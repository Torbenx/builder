#pragma once

#include "Instance.hpp"
#include "platform/Window.hpp"

namespace ui {

class RemoteInstance {
private:
    struct BasicAtlasAllocator : ui::AtlasAllocator {
        ui::IntVec nextAtlasSize(ui::IntVec size) override {
            if (size.x() == 0)
                size.x() = 4096;
            if (size.y() == 0)
                size.y() = 2048;
            else
                size.y() = size.y() * 2;
            return size;
        }
    };

    evt::Source<evt::CallbackEvent> m_callbackHandler;
    evt::Relay<evt::WindowStateUpdate, evt::ResizeEvent, evt::CallbackEvent> relay;
    BasicAtlasAllocator atlasAllocator;
    evt::ExecutorImpl executor;
    Instance instance {
        relay.output->output<evt::WindowStateUpdate>(),
        relay.output->output<evt::ResizeEvent>(),
        &executor,
        &atlasAllocator,
    };
    int_t nextTickToSchedule = 0;
    int_t nextTickToDo = 0;
    std::atomic<int_t> targetTick = -1;

    void schedule(evt::Scheduler scheduler, int_t tick) {
        scheduler.schedule({ *this, stx::CallMember<&RemoteInstance::doTick>() }, tick);
    }
    void doTick(evt::Scheduler scheduler) {
        log("doTick");
        int_t currentTick = nextTickToDo;
        nextTickToDo += 1;
        //log("doTick uiTick={}, depTick={}", currentTick, evt::tick());

        setInstance(&instance);
        executor.execute();
        instance.doTick();
        setInstance(nullptr);

        int_t expected = currentTick;
        if (!targetTick.compare_exchange_strong(expected, -1)) {
            //log("doTick expected {} got {}", currentTick, expected);
            VERIFY(Ui, expected > currentTick);
            schedule(scheduler, evt::tick() + 1);
        }
    }

public:
    RemoteInstance(const evt::Source<evt::WindowStateUpdate>& stateUpdate, const evt::Source<evt::ResizeEvent>& resizeEvent)
        : relay { evt::relayEvents(stateUpdate, resizeEvent, callbackHandler()) } {
        relay.output = relay.broadcastEvents(&executor);
    }

    evt::Source<evt::CallbackEvent>& callbackHandler() { return m_callbackHandler; };

    void scheduleTick(evt::Scheduler scheduler) {
        //log("scheduleTick uiTick={}, depTick={}", nextTickToSchedule, evt::tick());
        if (targetTick.exchange(nextTickToSchedule) == -1) {
            schedule(scheduler, evt::tick());
            //log("scheduled");
        }
        nextTickToSchedule += 1;
    }
};

}