#include "Scheduler.hpp"
#include "containers/AtomicQueue.hpp"
#include "platform/Window.hpp"
#include "util/time.hpp"
#include <atomic>

namespace evt {
namespace {

    constinit inline std::atomic<int_t> lowestActiveTick = 1;

    using Item = Task;

    class Block {
    private:
        Item* allocBegin = nullptr;
        Item* m_begin = nullptr;
        Item* m_end = 0;
        Item* allocEnd = 0;

        void clearFields() {
            allocBegin = nullptr;
            m_begin = nullptr;
            m_end = nullptr;
            allocEnd = nullptr;
        }
        void stealFieldsFrom(Block& other) {
            allocBegin = other.allocBegin;
            m_begin = other.m_begin;
            m_end = other.m_end;
            allocEnd = other.allocEnd;
            other.clearFields();
        }
        void maybeDeallocate() {
            std::allocator<Item>().deallocate(allocBegin, allocSize());
        }
        int_t allocSize() const { return allocEnd - allocBegin; }

    public:
        Item* begin() { return m_begin; }
        Item* end() { return m_end; }
        int_t size() { return end() - begin(); }
        Item* data() { return m_begin; }
        bool empty() { return size() == 0; }
        int_t backSpace() { return allocEnd - end(); }
        int_t frontSpace() { return begin() - allocBegin; }
        int_t totalCapacity() { return allocEnd - allocBegin; }
        Item popFront() { return *(m_begin++); }
        Item& operator[](int_t i) { return data()[i]; }
        Item* acquireBack(int_t size = 1) {
            Item* ret = end();
            m_end += size;
            return ret;
        }
        Block() = default;
        Block(const Block&) = delete;
        Block(Block&& other) {
            stealFieldsFrom(other);
        }
        Block(int_t capacity) {
            allocBegin = std::allocator<Item>().allocate(capacity);
            allocEnd = allocBegin + capacity;
            m_begin = allocBegin;
            m_end = allocBegin;
        }
        Block& operator=(Block&& other) {
            maybeDeallocate();
            stealFieldsFrom(other);
            return *this;
        }
        ~Block() {
            VERIFY(Event, empty());
            maybeDeallocate();
        }
    };

    struct LocalQueue {
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
        void queue(Item item) {
            if (block.backSpace() >= 1) {
                *block.acquireBack() = item;
                return;
            }
            Block newBlock = Block { nextBlockCapacity(block.totalCapacity()) };
            std::copy(block.begin(), block.end(), newBlock.acquireBack(block.size()));
            *newBlock.acquireBack() = item;
            block = std::move(newBlock);
        }
        Item pop() {
            if (block.empty())
                return {};
            return block.popFront();
        }
    };

    struct GlobalQueue {
        struct QueueItem : AtomicQueueItem<QueueItem> {
            QueueItem(Block&& block)
                : block(std::move(block)) { }
            Block block;
        };
        AtomicQueue<QueueItem> m_queue;
        Block pop() {
            QueueItem* qItem = m_queue.popFront();
            Block ret = std::move(qItem->block);
            delete qItem;
            return ret;
        }
        void queue(Block&& block) {
            QueueItem* qItem = new QueueItem { std::move(block) };
            m_queue.pushBack(qItem);
        }
    };
}

struct Scheduler {
    static inline GlobalQueue globalQueues[MAX_CONCURRENT_TICKS] = {};
    LocalQueue localQueues[MAX_CONCURRENT_TICKS] = {};
    LocalQueue& localQueue(int_t tick) {
        return localQueues[(size_t)tick % MAX_CONCURRENT_TICKS];
    }
    GlobalQueue& globalQueue(int_t tick) {
        return globalQueues[(size_t)tick & MAX_CONCURRENT_TICKS];
    }

    int_t activeTick = 1;

    bool checkForBetter() {
        int_t lowestTick = lowestActiveTick.load(std::memory_order_relaxed);
        for (int_t i = lowestTick; i < activeTick; i++) {
            Block newBlock = globalQueue(i).pop();
            if (newBlock.empty())
                continue;

            VERIFY(Event, localQueue(i).empty());
            localQueue(i).setBlock(std::move(newBlock));

            Block oldBlock = localQueue(activeTick).takeBlock();
            if (!oldBlock.empty())
                globalQueue(activeTick).queue(std::move(oldBlock));

            activeTick = i;
            evt::setTick(activeTick);
            return true;
        }
        return false;
    }

    void volunteerNonActive() {
        for (int_t i = activeTick; i < activeTick + MAX_CONCURRENT_TICKS - 1; i++) {
            Block block = localQueue(i).takeBlock();
            if (!block.empty())
                globalQueue(i).queue(std::move(block));
        }
    }

    void schedule(Item item, int_t tick) {
        VERIFY(Event, tick <= activeTick);
        localQueue(tick).queue(item);
    }

    void run() {
        const int_t ticksPerSecond = 100;
        const int_t nanosPerSecond = 1'000'000'000;
        const int_t nanosPerTick = nanosPerSecond / ticksPerSecond;
        uint64_t lastTickAbsolute = getNanos() / nanosPerTick;
        while (true) {
            while (Item item = localQueue(activeTick).pop()) {
                item.execute(this);
            }
            uint64_t nowTickAbsolute = getNanos() / nanosPerTick;
            while (nowTickAbsolute > lastTickAbsolute) {
                evt::setTick(evt::tick() + 1);
                plt::processEvents([]() {});
                plt::coreExe()->execute();
            }
        }
    }
};

void SchedulerHandle::schedule(Item item, int_t tick) {
    scheduler->schedule(item, tick);
}

void runScheduler(Item item, int_t startTick) {
    Scheduler scheduler { .activeTick = startTick };
    evt::setTick(startTick);
    item.execute(&scheduler);
    scheduler.run();
}

void Item::execute(Scheduler* scheduler) const {
    function(scheduler, data);
}

}