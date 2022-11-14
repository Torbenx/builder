#include "Buffer.hpp"

namespace gfx {

void Frame::growBufferAllocBump(uint32_t) { }

BufferMapping Frame::allocateBuffer(uint32_t id, BufferAllocKind kind, uint32_t gpuAlignment, uint32_t size, BufferMapping prevMapping) {
    uint32_t requiredSize = sizeof(BufferAllocHeader) + stx::alignmentCeil(BUFFER_CPU_ALLOCATION_ALIGNMENT, size);
    while (bufferAllocBump.offset + requiredSize >= bufferAllocBumpSize) {
        growBufferAllocBump(requiredSize);
    }
    new (get(bufferAllocBump)) BufferAllocHeader {
        .id = (uint16_t)id,
        .kind = kind,
        .alignment = (uint16_t)gpuAlignment,
        .size = size,
        .prevMapping = prevMapping,
    };
    BufferMapping ret = bufferAllocBump + sizeof(BufferAllocHeader);
    bufferAllocBump += requiredSize;
    return ret;
}

uint32_t Frame::allocateSWImage(int_t texelSize, iVec2 dimensions) {
    uint32_t id = toInstance(this)->nextSWImageId++;
    swImages[id] = sw::allocateImage(texelSize, dimensions);
    return id;
}

void Frame::complete() {
    toInstance(this)->frameComplete(toFrameIndex(this));
}

void Instance::allocateFrame(int_t index) {
    Frame& frame = frames[index];
    frame.swImages = new sw::ImageView<>[16];
    framesReadyMask |= (frame_mask_t)1 << index;
    framesAllocatedMask |= (frame_mask_t)1 << index;
}

bool Instance::beginFrame() {
    if (framesReadyMask != 0) {
    } else if (int_t frameCount = std::popcount(framesAllocatedMask); frameCount < MAX_CONCURRENT_FRAMES) {
        log("allocating frame {}", frameCount);
        allocateFrame(frameCount);
    } else {
        log("beginFrame() failed");
        return false;
    }
    int_t frameIndex = std::countr_zero(framesReadyMask);
    log("begining frame {}", frameIndex);
    framesReadyMask &= framesReadyMask - 1;
    setFrame(&frames[frameIndex]);
    return true;
}

void Instance::frameComplete(int_t index) {
    framesReadyMask |= (frame_mask_t)1 << index;
}

void Instance::clearFrame() {
    setFrame(nullptr);
}

}