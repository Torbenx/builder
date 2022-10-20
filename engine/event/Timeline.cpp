#include "Timeline.hpp"

namespace evt {

void Timeline::beginTick() {
    lastStartedTick += 1;
    VERIFY(Event, evt::currentTimeline() == nullptr);
    evt::setCurrentTimline(this);
}

void Timeline::endTick() {
    clearTick();
    int_t expected = lastStartedTick;
    if (!lastScheduledTick.compare_exchange_strong(expected, Timeline::INVALID_TICK, std::memory_order_relaxed, std::memory_order_acquire)) {
        VERIFY(Event, expected > lastStartedTick);
        async(asyncCallback, tickMapping[lastStartedTick + 1]);
    }
}

Timeline* mainTimeline() {
    static Timeline timeline = {};
    return &timeline;
}

RemoteTimeline::RemoteTimeline() {
    VERIFY(Event, evt::currentTimeline() == evt::mainTimeline());
    callbackRelay.output->startRelay(timeline()->executor());
}

void RemoteTimeline::asyncTick() {
    timeline()->tickMapping[nextTickToSchedule] = mainTick();
    if (timeline()->lastScheduledTick.exchange(nextTickToSchedule, std::memory_order_release) == Timeline::INVALID_TICK) {
        async(timeline()->asyncCallback, mainTick());
    }
    nextTickToSchedule += 1;
}

}