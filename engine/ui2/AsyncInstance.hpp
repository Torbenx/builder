#pragma once

#include "Instance.hpp"
#include "containers/Owned.hpp"
#include "event/AsyncTimeline.hpp"
#include "event/Window.hpp"

namespace ui {

class AsyncInstance : public evt::AsyncTimeline<Instance> {
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

    BasicAtlasAllocator atlasAllocator;

    void doTick();

public:
    AsyncInstance(const evt::Source<evt::WindowStateUpdate>& stateUpdate, const evt::Source<evt::ResizeEvent>& resizeEvent)
        : AsyncTimeline(stateUpdate, resizeEvent, &atlasAllocator) {
        timeline()->asyncCallback = { *this, stx::CallMember<&AsyncInstance::doTick>() };
    }

    stx::Owned<Instance::SWPainter*> createSWPainter() {
        return new Instance::SWPainter(this);
    }
    stx::Owned<Instance::HWPainter*> createHWPainter() {
        return new Instance::HWPainter(this);
    }

    std::vector<Instance::PaintEvent>& paintEvents() {
        return timeline()->paintRing[tick()];
    }
};

}