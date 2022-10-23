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

    TickRingBuffer<GlobalQueue> globalQueues = {};
    constinit TickRingBuffer<std::atomic<int_t>> globalItemsAlive = {};
    constinit std::atomic<int_t> lowestActiveTick = 1;
    constinit std::atomic<int_t> nonIdleThreads = 0;

    struct CallbackItem {
        Source<CallbackEvent>* postTo = nullptr;
        CallbackEvent event;
    };
    using CallbackQueue = AtomicArrayBuilder<CallbackItem>;
    TickRingBuffer<CallbackQueue> queuedCallbacks;

    struct Scheduler {
        static int_t lowestTick() { return lowestActiveTick.load(std::memory_order_relaxed); }

        TickRingBuffer<LocalStack> localStacks = {};

        void schedule(Item item, int_t tick) {
            if (!item) {
                VERIFY_NOT_REACHED(Event);
                return;
            }
            VERIFY(Event, tick >= evt::mainTick());
            VERIFY(Event, tick < lowestTick() + MAX_CONCURRENT_TICKS);
            globalItemsAlive[tick].fetch_add(1, std::memory_order_relaxed);
            localStacks[tick].push(item);
        }

        void start(stx::BasicCallback<> callback) {
            lowestActiveTick.store(0, std::memory_order_relaxed);
            evt::setMainTick(0);
            evt::setCurrentTimeline(mainTimeline());
            if (callback)
                callback();
            evt::setCurrentTimeline(nullptr);
            runWithPlatformCheck();
        }

        void enter() {
            runWithNoOpCheck();
        }

        template<typename AdditionalCheck>
        void run(AdditionalCheck additionalCheck) {
            // localStacks valid for [lowestTick, lowestTick + MAX_CONCURRENT_TICKS)
            // and empty for [lowestTick, activeTick)
            int_t activeTick = lowestTick();
            auto checkForBetter = [&]() {
                for (int_t i = lowestTick(); i < activeTick; i++) {
                    Block newBlock = globalQueues[i].tryPop();
                    if (newBlock.empty())
                        continue;

                    VERIFY(Event, localStacks[i].empty());
                    localStacks[i].setBlock(std::move(newBlock));
                    activeTick = i;
                    break;
                }
            };
            auto volunteerNonActive = [&]() {
                int_t lowest = lowestTick();
                if (localStacks[activeTick].size() >= 6) {
                    Block block = localStacks[activeTick].takeBlock();
                    Block newBlock = block.splitBack(3);
                    globalQueues[activeTick].queue(std::move(block));
                    localStacks[activeTick].setBlock(std::move(newBlock));
                }
                for (int_t i = activeTick + 1; i < lowest + MAX_CONCURRENT_TICKS; i++) {
                    Block block = localStacks[i].takeBlock();
                    if (!block.empty()) {
                        globalQueues[i].queue(std::move(block));
                    }
                }
            };
            auto updateIdle = [idleState = true](bool idle) mutable -> int_t {
                if (idle && !idleState) {
                    idleState = true;
                    return nonIdleThreads.fetch_sub(1, std::memory_order_relaxed) - 1;
                }
                if (!idle && idleState) {
                    idleState = false;
                    return nonIdleThreads.fetch_add(1, std::memory_order_relaxed) + 1;
                }
                return nonIdleThreads.load(std::memory_order_relaxed);
            };
            int_t nothingCounter = 0;
            static constexpr int_t nothingsBeforeIdle = 100;
            while (true) {
                /*{
                    int_t lowest = lowestTick();
                    log<Tag::Event>("lowest = {}, active = {}", lowest, activeTick);
                    for (int_t i = lowest; i < lowest + MAX_CONCURRENT_TICKS; i++)
                        log<Tag::Event>("{}: {} item(s) waiting", i, localStacks[i].size());
                }*/
                Item item = localStacks[activeTick].tryPop();
                if (item) {
                    nothingCounter = 0;
                    updateIdle(false);
                    do {
                        evt::setMainTick(activeTick);
                        item();
                        globalItemsAlive[activeTick].fetch_sub(1, std::memory_order_relaxed);
                        volunteerNonActive();
                    } while ((item = localStacks[activeTick].tryPop()));
                } else {
                    if (++nothingCounter >= nothingsBeforeIdle) {
                        int_t nonIdle = updateIdle(true);
                        if (nonIdle == 0) {
                            // check exit condition here...
                        }
                        using namespace std::chrono_literals;
                        std::this_thread::sleep_for(1ms);
                        nothingCounter = 0;
                    }
                }
                // having activeTick = lowestTick + MAX_CONCURRENT_TICKS should be fine since
                // the ring buffer has an extra element that should always be empty
                activeTick = std::min(activeTick + 1, lowestTick() + MAX_CONCURRENT_TICKS);
                activeTick = std::min(activeTick, additionalCheck());
                checkForBetter();
                volunteerNonActive();
            }
        }

        void runWithPlatformCheck() {
            struct Check {
                const int_t ticksPerSecond = 10000;
                const int_t nanosPerSecond = 1'000'000'000;
                const int_t nanosPerTick = nanosPerSecond / ticksPerSecond;
                int_t nextTickToStart = 0;
                int_t absoluteTickShift = getNanos() / nanosPerTick;

                int_t operator()() {

                    // end ticks
                    int_t currentLowestTick = lowestActiveTick.load(std::memory_order_relaxed);
                    int_t lowestTick = currentLowestTick;
                    std::vector<CallbackQueue::TakeReturnType> callbackQueues;
                    while (globalItemsAlive[lowestTick].load(std::memory_order_relaxed) == 0 && lowestTick < nextTickToStart) {
                        callbackQueues.push_back(queuedCallbacks[lowestTick].take());
                        lowestTick += 1;
                    }
                    if (lowestTick != currentLowestTick)
                        lowestActiveTick.store(lowestTick, std::memory_order_relaxed);

                    // start tick
                    int_t nowTickAbsolute = getNanos() / nanosPerTick;
                    [[maybe_unused]] int_t shouldBeTick = nowTickAbsolute - absoluteTickShift;
                    if (nowTickAbsolute < nextTickToStart + absoluteTickShift)
                        return std::numeric_limits<int_t>::max();

                    if (nextTickToStart - lowestTick >= MAX_CONCURRENT_TICKS) {
                        // log<Tag::Event>("failed to start tick {}, lowest is {} waiting for {} items, should be at {}",
                        //     nextTickToStart, lowestTick, globalItemsAlive[lowestTick].load(std::memory_order_relaxed), shouldBeTick);
                        return std::numeric_limits<int_t>::max();
                    }

                    // log<Tag::Event>("starting tick {}, lowest is {} waiting for {} items, should be at {}",
                    //     nextTickToStart, lowestTick, globalItemsAlive[lowestTick].load(std::memory_order_relaxed), shouldBeTick);
                    evt::setMainTick(nextTickToStart);
                    mainTimeline()->beginTick();
                    EXPECT(Event, mainTimeline()->tick(), nextTickToStart);
                    plt::processEventsImpl([]() {});
                    for (auto& queue : callbackQueues)
                        for (auto callback : queue)
                            callback.postTo->sendEvent(callback.event);
                    callbackQueues.clear();
                    mainTimeline()->executeScripts();
                    mainTimeline()->clearTick();

                    int_t returnTick = nextTickToStart;

                    nextTickToStart += 1;
                    return returnTick;
                }
            };
            run<Check>({});
        }

        void runWithNoOpCheck() {
            run([]() -> int_t {
                return std::numeric_limits<int_t>::max();
            });
        }
    };
    DECLARE_THREAD_LOCAL_GETSET(Scheduler*, scheduler, setScheduler, SCHEDULER)
}

void postCallback(Source<CallbackEvent>& postTo, stx::BasicCallback<> callback) {
    queuedCallbacks[evt::mainTick()].pushBack({ &postTo, { callback } });
}

void async(Item item, int_t tick) {
    scheduler()->schedule(item, tick);
}

void startScheduler(Item item) {
    auto scheduler = Scheduler {};
    setScheduler(&scheduler);
    scheduler.start(item);
}
void enterScheduler() {
    auto scheduler = Scheduler {};
    setScheduler(&scheduler);
    scheduler.enter();
}

void completeStartup(stx::BasicCallback<> callback) {
    int_t extraThreadCount = 1;
    std::vector<std::thread> threads;
    threads.reserve(extraThreadCount);
    for (int_t i = 0; i < extraThreadCount; i++)
        threads.emplace_back(evt::enterScheduler);

    evt::startScheduler(callback);

    for (std::thread& thread : threads)
        thread.join();
}

}