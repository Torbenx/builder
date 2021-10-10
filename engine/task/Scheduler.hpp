#pragma once

#include "containers/Allocator32Bit.hpp"
#include "containers/AtomicArrayBuilder.hpp"
#include "containers/FlatSet.hpp"
#include "util/coroutines.hpp"
#include "util/log.hpp"

namespace Tag {
struct Task {
    static constexpr char tag[] = "task";
    static constexpr bool verifyEnabled = true;
};
}

namespace task {

struct TaskHandle {
    uint32_t id = 0;

    bool operator==(const TaskHandle& other) const { return id == other.id; }
    bool operator==(std::nullptr_t) const { return id == 0; }

    TaskHandle& operator=(std::nullptr_t) {
        id = 0;
        return *this;
    }
    TaskHandle(std::nullptr_t) { }
    TaskHandle() { }
    TaskHandle(uint32_t id)
        : id(id) { }

    explicit operator bool() { return id != 0; }
};

class Scheduler {
public:
    struct Root {
        uint32_t id = 0;
        bool operator==(const Root& other) const { return id == other.id; }
        explicit operator bool() const { return id != 0; }
        bool operator==(std::nullptr_t) const { return !(bool)*this; }
        Root& operator=(std::nullptr_t) {
            id = 0;
            return *this;
        }
    };
    struct Task {
        uint32_t id : 24 = 0;
        uint32_t rootId : 8 = 0;
        bool operator==(const Task& other) const { return id == other.id && rootId == other.rootId; }
        explicit operator bool() const { return id != 0 && rootId != 0; }
        bool operator==(std::nullptr_t) const { return !(bool)*this; }
        Task& operator=(std::nullptr_t) {
            id = 0;
            rootId = 0;
            return *this;
        }
        Root root() { return { rootId }; }
    };
    using tickdiff_t = int16_t;
    using refcnt_t = int32_t;
    using utick_t = uint32_t;

    Scheduler(int_t maxRoots)
        : maxRoots(maxRoots) {
        ticks.reset(new int_t[maxRoots * maxRoots]);
        std::fill(ticks.get(), ticks.get() + maxRoots * maxRoots, 0);
    }

    Root newRoot(int_t startTick = 0);
    void signalRoot(Root root, bool execute);
    Task newTask();

    void updateDependencyCount(Task target, Task source, tickdiff_t targetTicksDelay, refcnt_t delta);
    void signalDependencies(Task task, int_t tick);

protected:
    virtual void queue(Task task, int_t tick);

private:
    struct Dependency {
        Task task;
        tickdiff_t delay;
        refcnt_t refCount;

        bool operator==(const Dependency& other) const {
            return task == other.task && delay == other.delay;
        }
    };
    struct DependencyQueueItem {
        Task task;
        tickdiff_t delay;
        refcnt_t refCountDelta;

        Dependency asEmptyDependency() {
            return { task, delay, 0 };
        }
    };
    struct RootDependencyQueueItem : DependencyQueueItem {
        Task source;
    };
    struct RootAndTaskSyncHeader{
        uint32_t size;
        Ptr32<Dependency> ptr;
    };

    static constexpr Ptr32<RootAndTaskSyncHeader> SIGNALED_BY_ROOT_SENTINEL = { std::numeric_limits<uint32_t>::max() };
    int_t maxRoots;
    std::unique_ptr<int_t[]> ticks;

    AtomicArrayBuilder<DependencyQueueItem>& dependencyQueue(Task task);
    FlatSet<Dependency>& internalDependencies(Task task);
    FlatSet<Dependency>& externalDependencies(Task task);
    refcnt_t& signalCount(Task task);
    std::atomic<refcnt_t>& signal(Task task, utick_t tick);
    std::atomic<refcnt_t>& signalDelta(Task task, utick_t tick);
    int_t* ticksFor(Root root, utick_t tick);
    AtomicArrayBuilder<RootDependencyQueueItem> rootQueue(Root root, utick_t tick);
    std::atomic<Ptr32<RootAndTaskSyncHeader>>& rootAndTaskSync(Task task, utick_t tick);
    FlatSet<Task>& tasksWithExternalDependencies(Root root);

    void signalInternal(Task target, int_t targetTick, refcnt_t delta);
    void signalExternalDependencies(Root source, utick_t sourceTick, ArrayView<Dependency> externals);
};

}
