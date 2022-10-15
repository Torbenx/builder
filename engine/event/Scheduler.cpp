#include "Scheduler.hpp"
#include "containers/AtomicQueue.hpp"
#include "platform/Window.hpp"
#include "util/time.hpp"
#include <atomic>
#include <thread>

namespace evt {
namespace {

    using Item = SchedulingItem;

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
}

struct SchedulerImpl {
    static constinit inline std::atomic<int_t> lowestActiveTick = 1;
    static constinit inline std::atomic<int_t> nonIdleThreads = 0;
    static inline TickRingBuffer<GlobalQueue> globalQueues = {};
    static int_t lowestTick() { return lowestActiveTick.load(std::memory_order_relaxed); }

    TickRingBuffer<LocalStack> localStackes = {};

    void schedule(Item item, int_t tick) {
        VERIFY(Event, tick >= evt::tick());
        localStackes[tick].push(item);
    }

    void start(Item item, int_t startTick) {
        lowestActiveTick.store(startTick, std::memory_order_relaxed);
        evt::setTick(startTick);
        if (item)
            item(this);
        runWithPlatformCheck();
    }

    void enter() {
        runWithNoOpCheck();
    }

    template<typename AdditionalCheck>
    void run(AdditionalCheck additionalCheck) {
        // localStackes valid for [lowestTick, lowestTick + MAX_CONCURRENT_TICKS)
        // and empty for [lowestTick, activeTick)
        int_t activeTick = lowestTick();
        auto checkForBetter = [&]() {
            for (int_t i = lowestTick(); i < activeTick; i++) {
                Block newBlock = globalQueues[i].tryPop();
                if (newBlock.empty())
                    continue;

                VERIFY(Event, localStackes[i].empty());
                localStackes[i].setBlock(std::move(newBlock));
                activeTick = i;
                break;
            }
        };
        auto volunteerNonActive = [&]() {
            int_t lowest = lowestTick();
            for (int_t i = activeTick + 1; i < lowest + MAX_CONCURRENT_TICKS; i++) {
                Block block = localStackes[i].takeBlock();
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
                    log<Tag::Event>("{}: {} item(s) waiting", i, localStackes[i].size());
            }*/
            Item item = localStackes[activeTick].tryPop();
            if (item) {
                nothingCounter = 0;
                updateIdle(false);
                do {
                    evt::setTick(activeTick);
                    item(this);
                    volunteerNonActive();
                } while ((item = localStackes[activeTick].tryPop()));
            } else {
                if (++nothingCounter >= nothingsBeforeIdle) {
                    int_t nonIdle = updateIdle(true);
                    if (nonIdle == 0) {
                        // check exit condition here...
                    }
                    using namespace std::chrono_literals;
                    std::this_thread::sleep_for(1ms);
                }
            }
            // having activeTick = lowestTick + MAX_CONCURRENT_TICKS should be fine since
            // the ring buffer has an extra element that should always be empty
            activeTick = std::min(activeTick + 1, lowestTick() + MAX_CONCURRENT_TICKS);
            activeTick = std::min(activeTick, additionalCheck(this));
            checkForBetter();
            volunteerNonActive();
        }
    }

    void runWithPlatformCheck() {
        struct Check {
            const int_t ticksPerSecond = 100;
            const int_t nanosPerSecond = 1'000'000'000;
            const int_t nanosPerTick = nanosPerSecond / ticksPerSecond;
            uint64_t startTickAbsolute = getNanos() / nanosPerTick;
            uint64_t absoluteTickShift = startTickAbsolute - evt::tick();
            uint64_t lastTickAbsolute = startTickAbsolute - 1;

            int_t operator()(Scheduler scheduler) {
                uint64_t nowTickAbsolute = getNanos() / nanosPerTick;
                int_t lowest = lowestTick();
                int_t returnTick = std::numeric_limits<int_t>::max();
                [[maybe_unused]] int_t shouldBeTick = nowTickAbsolute - absoluteTickShift;
                while (nowTickAbsolute > lastTickAbsolute) {
                    int_t tickToStart = lastTickAbsolute + 1 - absoluteTickShift;
                    if (tickToStart - lowest >= MAX_CONCURRENT_TICKS) {
                        // log<Tag::Event>("failed to start tick {}, lowest is {}, should be at {}", tickToStart, lowest, shouldBeTick);
                        break;
                    }
                    // log("starting tick {}, lowest is {}, should be at {}", tickToStart, lowest, shouldBeTick);

                    evt::setTick(tickToStart);
                    plt::processEvents(scheduler, []() {});

                    lastTickAbsolute += 1;
                    returnTick = std::min(tickToStart, returnTick);
                }
                return returnTick;
            }
        };
        run<Check>({});
    }

    void runWithNoOpCheck() {
        run([](Scheduler) -> int_t {
            return std::numeric_limits<int_t>::max();
        });
    }

    void tickComplete(int_t tick) {
        VERIFY(Event, localStackes[tick].empty());
        int_t prevLowest = lowestActiveTick.fetch_add(1, std::memory_order_relaxed);
        EXPECT(Event, prevLowest, tick);
    }
};

void Scheduler::schedule(Item item, int_t tick) {
    scheduler->schedule(item, tick);
}
void Scheduler::tickComplete(int_t tick) {
    scheduler->tickComplete(tick);
}

void startScheduler(Item item, int_t startTick) {
    auto scheduler = SchedulerImpl {};
    scheduler.start(item, startTick);
}
void enterScheduler() {
    auto scheduler = SchedulerImpl {};
    scheduler.enter();
}

}

namespace evt::dep {

void Target::completeTick(Scheduler scheduler) {
    PerTick& thisTick = perTick[evt::tick()];
    VERIFY(Event, thisTick.counter.load(std::memory_order_relaxed) == READY_COUNT);
    dependencyCount += thisTick.countDiff.load(std::memory_order_relaxed);
    thisTick.counter.store(0, std::memory_order_relaxed);
    thisTick.countDiff.store(0, std::memory_order_relaxed);

    if (dependencyCount > 0) {
        unlock(scheduler, evt::tick() + 1);
    } else {
        VERIFY(Event, dependencyCount == 0);
        // block it
        applySignal({}, READY_COUNT, evt::tick());
    }
}

void Dependency::destroy(Scheduler scheduler) {
    if (source) {
        source->removeDependency(scheduler, *signal.target, signal.tickShift);
        source = nullptr;
        signal = {};
    }
}

stx::Owned<Dependency> Source::createDependency(Target& target, int_t tickShift) {
    Signal signal { &target, tickShift };
    targets.push_back(signal);
    target.incrementDependencyCount(1, evt::tick() + tickShift);
    return { { this, signal } };
}

void Source::removeDependency(Scheduler scheduler, Target& target, int_t tickShift) {
    Signal signal { &target, tickShift };
    auto it = std::find(targets.begin(), targets.end(), signal);
    VERIFY(Event, it != targets.end());
    targets.erase(it);
    target.decrementDependencyCount(scheduler, 1, evt::tick() + tickShift);
}

}