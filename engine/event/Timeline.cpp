#include "AsyncTimeline.hpp"

namespace evt {

void Timeline::beginTick() {
    lastStartedTick += 1;
    VERIFY(Event, evt::currentTimeline() == nullptr);
    evt::setCurrentTimeline(this);
}

void Timeline::clearTick() {
    evt::setCurrentTimeline(nullptr);
}

void Timeline::endTick() {
    clearTick();
    int_t expected = lastStartedTick;
    if (!lastScheduledTick.compare_exchange_strong(expected, Timeline::INVALID_TICK, std::memory_order_relaxed, std::memory_order_acquire)) {
        VERIFY(Event, expected > lastStartedTick);
        async(asyncCallback, tickMapping[lastStartedTick + 1]);
    }
}

void detail::AsyncTimelineBase::scheduleAsyncTick(Timeline* timeline) {
    timeline->tickMapping[nextTickToSchedule] = mainTick();
    if (timeline->lastScheduledTick.exchange(nextTickToSchedule, std::memory_order_release) == Timeline::INVALID_TICK) {
        async(timeline->asyncCallback, mainTick());
    }
    nextTickToSchedule += 1;
}

int_t detail::AsyncTimelineBase::computeTick(const Timeline* timeline) const {
    if (timeline->tickMapping[nextTickToSchedule - 1] == mainTick())
        return nextTickToSchedule - 1;
    return nextTickToSchedule;
}

}