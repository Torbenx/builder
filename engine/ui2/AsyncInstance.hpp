#pragma once

#include "Instance.hpp"
#include "event/Window.hpp"
#include "event/AsyncTimeline.hpp"

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

    void asyncPaint(sw::TextureView output, sw::Semaphore waitFor, sw::Semaphore signal);
};

}