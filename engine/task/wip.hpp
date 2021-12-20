#pragma once

template<typename Impl>
class DependencyGraph {
public:

    struct TargetData {
        struct Update {
            utick_t startTick;
            refcnt_t delta;
        };

        FlatSet<Update> pendingUpdates;
        refcnt_t refCount;
    };

    struct SourceData {
        struct Dependency {
            Target target;
            tickdiff_t delay;
            refcnt_t refCount;

            bool operator==(Dependency& other) {
                return target == other.target && delay == other.delay;
            }
        };
        struct Update {
            Target target;
            utick_t startTick;
            tickdiff_t delay;
            refcnt_t delta;

            Dependency asEmptyDependency() {
                return { target, delay, 0 };
            }
        };

        FlatSet<Update> pendingUpdates;
        FlatSet<Dependency> depedencies;
    };

    /*
     * Impl should have:
     *
     * AtomicArrayBuilder<TargetData::Update>& updateQueue(Target target);
     * AtomicArrayBuilder<SourceData::Update>& updateQueue(Source source);
     *
     * std::atomic<urefcnt_t>& signal(Target target, utick_t tick);
     *
     * void depedenciesFulfilled(Target target, utick_t tick);
     *
     * bool lessThan(utick_t a, utick_t b);
     */

    void signalSource(Impl::Source source, utick_t tick, SourceData& data) {
        auto queue = impl()->updateQueue(source).take(tick + 1);
        data.pendingUpdates.insert(*queue);
        queue.release();

        data.pendingUpdates.removeIf([&](auto item) {
            if (impl()->lessThan(item.startTick, tick)) {
                auto& dep = data.depedencies.getOrInsert(item.asEmptyDependency());
                dep.refCount += item.delta;
                return true;
            }
            return false;
        });

        data.depedencies.removeIf([](auto dep) { return dep.refCount == 0; });
        for (auto dep : data.depedencies)
            signalTarget(dep.target, tick + dep.delay, dep.refCount);
    }

    void unlockTarget(Impl::Target target, utick_t tick, TargetData& data) {
        auto queue = impl()->updateQueue(target).take();
        data.pendingUpdates.insert(*queue);
        queue.release();

        data.pendingUpdates.removeIf([&](auto item) {
            if ((impl()->lessThan(item.startTick, tick)) {
                data.refCount += delta;
                return true;
            }
            return false;
        });

        signalTarget(target, tick, TARGET_UNLOCKED_FLAG - data.refCount);
    }

    // must never be called after unlockTarget(target, tick + delay + 1, ...)
    void updateDependency(Impl::Source source, Impl::Target target, utick_t tick, tickdiff_t delay, refcnt_t delta) {
        utick_t firstEmittingTick, firstReceivingTick;
        impl()->updateQueue(source).pushBack({ target, tick, delay, delta },
            [&](utick_t currentSourceTick) {
                firstEmittingTick = std::max(currentSourceTick, tick);
                firstReceivingTick = firstEmittingTick + delay;
                impl()->signal(target, firstReceivingTick).fetch_sub(delta, std::memory_order_relaxed);
            });
        impl()->updateQueue(target).pushBack({ firstReceivingTick, delta });
    }

private:
    static constexpr urefcnt_t TARGET_UNLOCKED_FLAG = (urefcnt_t)1 << (std::numeric_limits<urefcnt_t>::digits - 1);

    Impl* impl() { return static_cast<Impl*>(this); }

    void signalTarget(Impl::Target target, utick_t tick, urefcnt_t delta) {
        urefcnt_t i = impl()->signal(target, tick).fetch_add(delta, std::memory_order_acq_rel);
        i += delta;
        if (i == TARGET_UNLOCKED_FLAG)
            impl()->depedenciesFulfilled(target, tick);
    }
};
