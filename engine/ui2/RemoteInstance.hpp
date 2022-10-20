#pragma once

#include "Instance.hpp"
#include "platform/Window.hpp"

namespace ui {

class RemoteInstance : public evt::RemoteTimeline {
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

    evt::Relay<evt::WindowStateUpdate, evt::ResizeEvent> relay;
    BasicAtlasAllocator atlasAllocator;
    Instance instance {
        timeline(),
        relay.output->output<evt::WindowStateUpdate>(),
        relay.output->output<evt::ResizeEvent>(),
        &atlasAllocator,
    };

    void doTick() {
        timeline()->beginTick();
        ui::setInstance(&instance);
        log("doTick");
        timeline()->executeScripts();
        instance.doTick();
        ui::setInstance(nullptr);
        timeline()->endTick();
    }

public:
    RemoteInstance(const evt::Source<evt::WindowStateUpdate>& stateUpdate, const evt::Source<evt::ResizeEvent>& resizeEvent)
        : relay { evt::relayEvents(stateUpdate, resizeEvent) } {
        relay.output->startRelay(timeline()->executor());
        timeline()->asyncCallback = { *this, stx::CallMember<&RemoteInstance::doTick>() };
    }
};

}