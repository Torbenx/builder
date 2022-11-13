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

}