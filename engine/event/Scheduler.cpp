#include "Timeline.hpp"
#include "containers/AtomicArrayBuilder.hpp"
#include "containers/AtomicQueue.hpp"
#include "platform/Window.hpp"
#include "util/time.hpp"
#include <atomic>
#include <thread>

namespace evt {
namespace {

    using Item = stx::BasicCallback<>;

    struct BlockData {
        Item* m_data = nullptr;
        uint32_t m_size = 0;
        uint32_t m_capacity = 0;
    };
    class Block : private BlockData {
    private:
        void setData(BlockData data) {
            *(BlockData*)this = data;
        }
        void stealDataFrom(Block& other) {
            setData(other);
            other.setData({});
        }
        void maybeDeallocate() {
            std::allocator<Item>().deallocate(data(), capacity());
        }
        Block(const BlockData& data)
            : BlockData(data) { }

    public:
        static Block adoptData(BlockData data) { return Block(data); }
        Item* begin() { return data(); }
        Item* end() { return data() + size(); }
        int_t size() { return m_size; }
        Item* data() { return m_data; }
        bool empty() { return size() == 0; }
        int_t capacity() { return m_capacity; }
        Item popBack() {
            Item ret = (*this)[size() - 1];
            m_size -= 1;
            return ret;
        }
        Item& operator[](int_t i) { return data()[i]; }
        Item* acquireBack(int_t size = 1) {
            Item* ret = end();
            m_size += size;
            return ret;
        }
        Block() = default;
        Block(const Block&) = delete;
        Block(Block&& other) {
            stealDataFrom(other);
        }
        Block(int_t capacity) {
            m_data = std::allocator<Item>().allocate(capacity);
            m_size = 0;
            m_capacity = capacity;
        }
        Block& operator=(Block&& other) {
            maybeDeallocate();
            stealDataFrom(other);
            return *this;
        }
        BlockData takeData() {
            BlockData data = *this;
            setData({});
            return data;
        }
        Block splitBack(int_t newSize) {
            Block newBlock(std::bit_ceil((size_t)newSize));
            int_t myNewSize = size() - newSize;
            std::copy_n(data() + myNewSize, newSize, newBlock.acquireBack(newSize));
            m_size = myNewSize;
            return newBlock;
        }
        ~Block() {
            VERIFY(Event, empty());
            maybeDeallocate();
        }
    };

    struct LocalStack {
        Block block;
        static constexpr int_t nextBlockCapacity(int_t cap) {
            if (cap == 0)
                return 4;
            else
                return cap * 2;
        }
        bool empty() { return block.empty(); }
        void setBlock(Block newBlock) {
            block = std::move(newBlock);
        }
        Block takeBlock() {
            return std::move(block);
        }
        void push(Item item) {
            if (block.capacity() - block.size() >= 1) {
                *block.acquireBack() = item;
                return;
            }
            Block newBlock { nextBlockCapacity(block.capacity()) };
            std::copy_n(block.data(), block.size(), newBlock.acquireBack(block.size()));
            *newBlock.acquireBack() = item;
            block = std::move(newBlock);
        }
        Item tryPop() {
            if (block.empty())
                return {};
            return block.popBack();
        }
        int_t size() { return block.size(); }
    };

    struct GlobalQueue {
        AtomicQueue<BlockData> m_queue;
        Block tryPop() {
            std::optional<BlockData> block = m_queue.tryPopFront();
            if (!block.has_value())
                return {};
            return Block::adoptData(block.value());
        }
        void queue(Block&& block) {
            m_queue.pushBack(block.takeData());
        }
    };

    struct CallbackItem {
        Source<CallbackEvent>* postTo = nullptr;
        CallbackEvent event;
    };
    using CallbackQueue = AtomicArrayBuilder<CallbackItem>;

    struct GlobalData {
        TickRingBuffer<GlobalQueue> queues = {};
        TickRingBuffer<std::atomic<int_t>> itemsAlive = {};
        std::atomic<int_t> lowestActiveTick = 0;
        TickRingBuffer<CallbackQueue> queuedCallbacks;
        std::atomic<bool> shouldExit = false;
    };

    struct Scheduler {
        GlobalData* global;
        TickRingBuffer<LocalStack> localStacks = {};
        int_t checkIndex = 0;
        int_t activeTick = lowestTick();
        std::vector<bool (*)(Scheduler*)> checks;
        bool hasNonActiveTicks = false;
        bool executingItem = false;
        bool positiveCheckPending = false;
        const int_t checksBeforeSleep = 30;
        int_t checksSinceLastSchedule = 0;
        bool shouldExit = false;
        uint64_t lastSecond = 0;
        uint64_t totalWorkTime = 0;
        int_t itemsExecuted = 0;
        int_t threadId = 0;

        Scheduler(int_t id, GlobalData* global)
            : global(global), threadId(id) {
            checks.push_back([](Scheduler* scheduler) { return scheduler->checkForBetter(); });
            checks.push_back([](Scheduler* scheduler) { return scheduler->checkForSleep(); });
        }
        int_t lowestTick() { return global->lowestActiveTick.load(std::memory_order_relaxed); }

        void schedule(Item item, int_t tick) {
            if (!item) {
                VERIFY_NOT_REACHED(Event);
                return;
            }
            VERIFY(Event, tick >= evt::mainTick());
            VERIFY(Event, tick < lowestTick() + MAX_CONCURRENT_TICKS);
            global->itemsAlive[tick].fetch_add(1, std::memory_order_relaxed);
            localStacks[tick].push(item);
            if (tick > activeTick)
                hasNonActiveTicks = true;
        }

        bool checkForBetter() {
            for (int_t i = lowestTick(); i < activeTick; i++) {
                Block newBlock = global->queues[i].tryPop();
                if (newBlock.empty())
                    continue;

                VERIFY(Event, localStacks[i].empty());
                localStacks[i].setBlock(std::move(newBlock));
                activeTick = i;
                hasNonActiveTicks = true;
                return true;
            }
            return false;
        }

        bool checkForSleep() {
            uint64_t now = getNanos();
            if (now - lastSecond > 1'000'000'000) {
                log("{} executed {} items and worked {}% of the time", threadId, itemsExecuted, totalWorkTime * 100.0f / (now - lastSecond));
                totalWorkTime = 0;
                itemsExecuted = 0;
                lastSecond = now;
            }
            if (localStacks[activeTick].size() == 0 && !hasNonActiveTicks && !executingItem)
                checksSinceLastSchedule += 1;
            else
                checksSinceLastSchedule = 0;

            if (checksSinceLastSchedule < checksBeforeSleep)
                return false;

            if (global->shouldExit.load()) {
                shouldExit = true;
                return false;
            }
            using namespace std::chrono_literals;
            // std::this_thread::sleep_for(1ms);

            checksSinceLastSchedule = 0;
            return false;
        }

        void volunteerNonActive() {
            int_t lowest = lowestTick();
            /*if (localStacks[activeTick].size() >= 6) {
                Block block = localStacks[activeTick].takeBlock();
                Block newBlock = block.splitBack(3);
                globalQueues[activeTick].queue(std::move(block));
                localStacks[activeTick].setBlock(std::move(newBlock));
            }*/
            for (int_t i = activeTick + 1; i < lowest + MAX_CONCURRENT_TICKS; i++) {
                Block block = localStacks[i].takeBlock();
                if (!block.empty()) {
                    global->queues[i].queue(std::move(block));
                }
            }
            hasNonActiveTicks = false;
        }

        bool check() {
            if (positiveCheckPending)
                return false;

            auto check = checks[checkIndex];
            checkIndex += 1;
            if (checkIndex == (int_t)checks.size())
                checkIndex = 0;

            bool ret = check(this);
            positiveCheckPending = ret;
            return ret;
        }

        void runOnce() {
            positiveCheckPending = false;
            if (Item item = localStacks[activeTick].tryPop(); item) {
                if (hasNonActiveTicks)
                    volunteerNonActive();
                evt::setMainTick(activeTick);
                executingItem = true;
                uint64_t workBegin = getNanos();
                item();
                totalWorkTime += getNanos() - workBegin;
                executingItem = false;
                global->itemsAlive[activeTick].fetch_sub(1, std::memory_order_relaxed);
                itemsExecuted += 1;
            } else {
                // having activeTick = lowestTick + MAX_CONCURRENT_TICKS should be fine since
                // the ring buffer has an extra element that should always be empty
                int_t lowest = lowestTick();
                activeTick = std::min(activeTick + 1, lowest + MAX_CONCURRENT_TICKS);
                if (activeTick == lowest + MAX_CONCURRENT_TICKS)
                    hasNonActiveTicks = false;
            }
            check();
        }

        void run() {
            while (!shouldExit) {
                runOnce();
            }
        }
    };
    DECLARE_THREAD_LOCAL_GETSET(Scheduler*, scheduler, setScheduler, SCHEDULER)

    struct MainScheduler : Scheduler {
        const int_t ticksPerSecond = 10000;
        const int_t nanosPerSecond = 1'000'000'000;
        const int_t nanosPerTick = nanosPerSecond / ticksPerSecond;
        int_t nextTickToStart = 0;
        int_t absoluteTickShift = getNanos() / nanosPerTick;
        bool shouldStartTick = false;
        std::vector<CallbackQueue::TakeReturnType> callbackQueues;
        Timeline timeline;

        MainScheduler(int_t id, GlobalData* global)
            : Scheduler(id, global) {
            checks.push_back([](Scheduler* scheduler) {
                return static_cast<MainScheduler*>(scheduler)->checkTickLifetime();
            });
        }

        bool checkTickLifetime() {
            // end ticks
            int_t currentLowestTick = lowestTick();
            int_t lowestTick = currentLowestTick;
            while (global->itemsAlive[lowestTick].load(std::memory_order_relaxed) == 0 && lowestTick < nextTickToStart) {
                callbackQueues.push_back(global->queuedCallbacks[lowestTick].take());
                lowestTick += 1;
            }
            if (lowestTick != currentLowestTick)
                global->lowestActiveTick.store(lowestTick, std::memory_order_relaxed);

            // start tick
            int_t nowTickAbsolute = getNanos() / nanosPerTick;
            [[maybe_unused]] int_t shouldBeTick = nowTickAbsolute - absoluteTickShift;
            if (nowTickAbsolute < nextTickToStart + absoluteTickShift)
                return false;

            if (nextTickToStart - lowestTick >= MAX_CONCURRENT_TICKS) {
                // log<Tag::Event>("failed to start tick {}, lowest is {} waiting for {} items, should be at {}",
                //     nextTickToStart, lowestTick, global->itemsAlive[lowestTick].load(std::memory_order_relaxed), shouldBeTick);
                return false;
            }

            // log<Tag::Event>("starting tick {}, lowest is {} waiting for {} items, should be at {}",
            //     nextTickToStart, lowestTick, global->itemsAlive[lowestTick].load(std::memory_order_relaxed), shouldBeTick);

            hasNonActiveTicks = true;
            bool result = false;
            if (nextTickToStart < activeTick) {
                activeTick = nextTickToStart;
                result = true;
            }

            shouldStartTick = true;
            return result;
        }

        bool run() {
            for (auto& queue : callbackQueues)
                for (auto callback : queue)
                    callback.postTo->sendEvent(callback.event);
            callbackQueues.clear();
            int_t scriptsExecuted = timeline.executeScripts();
            if (scriptsExecuted == 0 && lowestTick() == nextTickToStart)
                return false;
            timeline.clearTick();

            nextTickToStart += 1;
            shouldStartTick = false;
            itemsExecuted += 1;

            do {
                runOnce();
            } while (!shouldStartTick);

            evt::setMainTick(nextTickToStart);
            timeline.beginTick();
            EXPECT(Event, timeline.tick(), nextTickToStart);
            return true;
        }
    };

    struct EngineImpl {
        GlobalData global;
        MainScheduler mainScheduler { 0, &global };
        std::vector<std::thread> extraThreads;

        EngineImpl() {
            int_t tCount = 0;
            for (int_t i = 0; i < tCount; i++) {
                extraThreads.emplace_back(
                    [i](GlobalData* global) {
                        Scheduler scheduler { i + 1, global };
                        setScheduler(&scheduler);
                        scheduler.run();
                        setScheduler(nullptr);
                    },
                    &global);
            }
        }
        ~EngineImpl() {
            global.shouldExit.store(true);
            for (auto& thread : extraThreads)
                thread.join();
        }
    };
}

void async(Item item, int_t tick) {
    scheduler()->schedule(item, tick);
}

void postCallback(Source<CallbackEvent>& postTo, stx::BasicCallback<> callback) {
    scheduler()->global->queuedCallbacks[evt::mainTick()].pushBack({ &postTo, { callback } });
}

bool schedulerCheck() {
    return scheduler()->check();
}

EngineScope::EngineScope()
    : data(new EngineImpl {}) {
    EngineImpl* impl = (EngineImpl*)data;

    VERIFY(Event, scheduler() == nullptr);
    VERIFY(Event, mainTick() == 0);
    VERIFY(Event, currentTimeline() == nullptr);

    setScheduler(&impl->mainScheduler);
    setMainTick(0);
    impl->mainScheduler.timeline.beginTick();
}
bool EngineScope::run() {
    return static_cast<MainScheduler*>(scheduler())->run();
}
EngineScope::~EngineScope() {
    EngineImpl* impl = (EngineImpl*)data;

    VERIFY(Event, scheduler() == &impl->mainScheduler);
    VERIFY(Event, mainTick() == impl->mainScheduler.nextTickToStart);
    VERIFY(Event, currentTimeline() == &impl->mainScheduler.timeline);

    impl->mainScheduler.timeline.clearTick();
    setMainTick(0);
    setScheduler(nullptr);

    delete impl;
    data = nullptr;
}

}