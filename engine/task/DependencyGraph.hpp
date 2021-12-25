#pragma once

template<typename T>
class UpdateQueue : public FlatSet<T> {
public:
    template<typename Callback, typename Condition>
    void apply(Callback&& callback, Condition&& cond) {
        removeIf([](auto update) {
            if (cond(update)) {
                callback(update);
                return true;
            }
            return false;
        });
    }
};

template<typename T>
class TickUpdateQueue : public UpdateQueue<T> {
    template<typename Callback>
    void apply(Callback&& callback, utick_t utick, TickExpander expander) {
        auto tick = expander.tick(utick);
        apply(callback,
            [=](auto update) {
                return expander.tick(update.startTick) <= tick;
            });
    }
};

using utick_t = uint32_t;
using urefcnt_t = uint32_t;
using refcnt_t = std::make_signed_t<urefcnt_t>;
using tickdiff_t = int16_t;

struct Group {
    uint8_t id = 0;

    int_t index() { return id; }
};

struct Task {
    uint16_t id = 0;
};

struct Target {
    struct Update {
        Group sourceGroup;
        Target target;
        refcnt_t delta;
        utick_t startTick;
    };

    Target(Task task)
        : id(task.id), mapped(false) { }

    uint32_t id : 31 = 0;
    bool mapped : 1 = 0;

    bool isMapping() { return mapped; }
    Task asTask() { return { id }; }
};

struct Dependency {
    Target target;
    tickdiff_t delay;
    refcnt_t refCount = 0;

    bool operator==(Dependency& other) {
        return target == other.target && delay == other.delay;
    }
};

struct MappedDependecy {
    struct GroupInfo {
        Group group;
        Target mapping;
        refcnt_t refCount = 0;

        bool operator==(GroupInfo other) {
            // mapping follows from the group
            return group == other.group;
        }
    };

    Target target;
    FlatSet<GroupInfo> referencingGroups;

    bool operator==(Target other) {
        return target == other;
    }
};

struct Source {
    struct Update {
        Target target;
        tickdiff_t delay;
        refcnt_t delta;
        utick_t startTick;

        Dependency asEmptyDependency() { return { target, delay, 0 }; }
        RootDependency asEmptyMappedDependecy() {
            // delay should always be 0 and delta always +1
            return { target };
        }
    };

    static constexpr uint16_t ROOT_ID { 1 };
    static constexpr Source ROOT { ROOT_ID };

    uint16_t id = 0;

    bool isRoot() {
        return task == ROOT_ID;
    }
};

struct Promise {
    // stuff that depends on us
    FlatSet<Dependency> dependencies;
    TickUpdateQueue<Source::Update> sourceUpdates;

    // stuff we depend on
    urefcnt_t refCount = 0;
    TickUpdateQueue<Target::Update> targetUpdates;
};

struct GroupHeader {
    FlatSet<MappedDependecy> dependencies;
    TickUpdateQueue<Source::Update> sourceUpdates;
    TickUpdateQueue<Target::Update> targetUpdates;
};

struct DependencyGraph {

    /* ==============> TODO <============== */
    AtomicArrayBuilder<Target::Update>& updateQueue(Group, Target);
    AtomicArrayBuilder<Source::Update>& updateQueue(Group, Source);
    std::atomic<urefcnt_t>& signal(Group, Target, utick_t);
    TickExpander tickExpander(Group);
    std::pair<Group, Target> translateMapping(Group, Target);
    // valid after inializeGroupTick(sourceGroup, sourceTick)
    utick_t translateTick(Group sourceGroup, Group targetGroup, utick_t sourceTick);
    // Makes it possible to send signals across group boundaries.
    // Returns a target in sourceGroup that is linked to the provided target.
    std::pair<Target, bool> findOrCreateMapping(Group sourceGroup, Group targetGroup, Task target, utick_t sourceTick);

    /* ==============> Scheduler <============== */
    void scheduleTask(Group, Task, utick_t);

    /* ==============> Concurrency <============== */
    static constexpr urefcnt_t TARGET_UNLOCKED_FLAG = (urefcnt_t)1 << (std::numeric_limits<urefcnt_t>::digits - 1);

    void signalTarget(Group, Target, utick_t, urefcnt_t delta);
    void unlockTarget(Group, Target, utick_t, refcnt_t signalCount);
    void updateDependency(Group group, Source source, Target target, utick_t tick, tickdiff_t delay, refcnt_t delta);

    /* ==============> ? <============== */
    static constexpr refcnt_t EXTERNAL_REFERENCE_REF_COUNT = 1;
    static constexpr size_t MAX_GROUPS = 256;

    void onTaskTickFinish(Promise&, Group, Task, utick_t);
    void onDepedenciesFulfilled(Group, Target, utick_t);
    Target updateDependency(Group sourceG, Task source, Group targetG, Task target, utick_t sourceTick, tickdiff_t targetTickDelay, refcnt_t delta);
    void beginGroupTick(GroupHeader&, Group, utick_t, std::bitset<MAX_GROUPS> firstTimeUsingTick);
    void initializeGroupTick();
};
