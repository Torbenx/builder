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

void Frame::allocationComplete() {
    Instance* instance = toInstance(this);
    int_t nextIndex = toFrameIndex(this) + 1;
    if (nextIndex == instance->concurrentFrames)
        nextIndex = 0;
    VERIFY(GFX, nextIndex == instance->nextFrameIndex);
    Frame* nextFrame = &instance->frames[nextIndex];
    for (int_t i = instance->concurrentFrames - 1; i >= 0; i--) {
        for (auto alloc : swImageAllocations.past(i)) {
            nextFrame->swImages[alloc.id] = sw::allocateImage(alloc.format, alloc.dimensions);
        }
    }
    swImageAllocations.shift(instance->concurrentFrames);
    nextFrame->swImageAllocations = std::move(swImageAllocations);
    instance->allocationOngoing = false;
}

void Frame::complete() {
    toInstance(this)->framesReadyMask |= 1u << toFrameIndex(this);
    
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

bool Instance::beginFrame() {
    int_t frameIndex = nextFrameIndex;
    if ((framesReadyMask & (1u << frameIndex)) == 0 || allocationOngoing)
        return false;

    nextFrameIndex += 1;
    if (nextFrameIndex == concurrentFrames)
        nextFrameIndex = 0;
    allocationOngoing = true;

    log("begining frame {}", frameIndex);
    setFrame(&frames[frameIndex]);
    return true;
}

void Instance::clearFrame() {
    setFrame(nullptr);
}

}