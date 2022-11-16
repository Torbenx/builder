#include "Buffer.hpp"

namespace gfx {

void Frame::growBufferAllocBump(uint32_t) { }

BufferMapping Frame::allocateBuffer(uint32_t id, BufferAllocKind kind, uint32_t gpuAlignment, uint32_t size, BufferMapping prevMapping) {
    uint32_t requiredSize = sizeof(BufferAllocInfo) + stx::alignmentCeil(BUFFER_CPU_ALLOCATION_ALIGNMENT, size);
    while (bufferAllocBump.offset + requiredSize >= bufferAllocBumpSize) {
        growBufferAllocBump(requiredSize);
    }
    new (get(bufferAllocBump)) BufferAllocInfo {
        .id = (uint16_t)id,
        .kind = kind,
        .alignment = (uint16_t)gpuAlignment,
        .size = size,
        .prevMapping = prevMapping,
    };
    BufferMapping ret = bufferAllocBump + sizeof(BufferAllocInfo);
    bufferAllocBump += requiredSize;
    return ret;
}

uint32_t Frame::allocateSWImage(TexelLayout format, iVec2 dimensions) {
    uint32_t id = toInstance(this)->nextSWImageId++;
    swImages[id] = sw::allocateImage(format, dimensions);
    swImageAllocations.current().push_back({
        .format = format,
        .id = id,
        .dimensions = dimensions,
    });
    return id;
}

void Instance::allocateFrame(int_t index) {
    Frame& frame = frames[index];
    frame.swImages = new sw::ImageView<>[16];
}

Instance::Instance() {
    for (int_t i = 0; i < concurrentFrames; i++)
        allocateFrame(i);
}

Instance::~Instance() {
    // TODO: cleanup
}

bool Instance::nextFrameReady() {
    return (framesReadyMask.load() & (1u << nextFrameIndex)) != 0 && !allocationOngoing.load();
}

void Instance::beginFrame(sw::Semaphore allocationComplete, sw::Semaphore frameComplete) {
    VERIFY(GFX, nextFrameReady());
    int_t frameIndex = nextFrameIndex++;
    if (nextFrameIndex == concurrentFrames)
        nextFrameIndex = 0;

    //log("begining frame {}", frameIndex);
    Frame* frame = &frames[frameIndex];

    allocationOngoing.store(true);
    allocationComplete.thenSync({
        frame,
        [](Frame* frame) {
            toInstance(frame)->allocationComplete(toFrameIndex(frame));
        },
    });
    framesReadyMask.fetch_and(~(1u << frameIndex)); 
    frameComplete.thenSync({
        frame,
        [](Frame* frame) {
            toInstance(frame)->framesReadyMask.fetch_or(1u << toFrameIndex(frame));
        },
    });

    setFrame(frame);
}

void Instance::allocationComplete(int_t index) {
    Frame* curFrame = &frames[index];
    int_t nextIndex = index + 1;
    if (nextIndex == concurrentFrames)
        nextIndex = 0;
    VERIFY(GFX, nextIndex == nextFrameIndex);
    Frame* nextFrame = &frames[nextIndex];

    for (int_t i = concurrentFrames - 1; i >= 0; i--) {
        for (auto alloc : curFrame->swImageAllocations.past(i)) {
            nextFrame->swImages[alloc.id] = sw::allocateImage(alloc.format, alloc.dimensions);
        }
    }
    curFrame->swImageAllocations.shift(concurrentFrames);
    nextFrame->swImageAllocations = std::move(curFrame->swImageAllocations);

    allocationOngoing.store(false);
}

void Instance::clearFrame() {
    setFrame(nullptr);
}

}