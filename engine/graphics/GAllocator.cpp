#include "GAllocator.hpp"

GAllocator::GAllocator(int_t dynamicSize, int_t staticSize, int_t stagingSize)
    : dynamicAllocator(dynamicSize), staticAllocator(staticSize), dynamicSize(dynamicSize), staticSize(staticSize), stagingSize(stagingSize) {

    staging.initializeWriteOnly(stagingSize);
    stagingMapped = (std::byte*)staging.mapWritable(0, stagingSize);

    dynamicCPU.initializeWriteOnly(dynamicSize);
    dynamicMapped = (std::byte*)dynamicCPU.mapWritable(0, dynamicSize);
    dynamicBuffer.initializeStatic(dynamicSize);

    staticBuffer.initializeStatic(staticSize);
}

void* GAllocator::queueForAllocation(GPtr* ptr, int_t alignment, int_t size, std::vector<AllocationInfo>& queue) {
    if (ptr->bufferId != 0) {
        free(ptr);
    }
    ptr->bufferId = 0xff;
    ptr->offset = 0;
    int_t offset = stagingOffset;
    stagingOffset += size;
    queue.push_back({ ptr, alignment, offset, size });
    return stagingMapped + offset;
}

void GAllocator::stage() {
    staging.bind(GL_COPY_READ_BUFFER);
    int_t stagedSize = 0;

    dynamicBuffer.bind(GL_COPY_WRITE_BUFFER);
    for (AllocationInfo alloc : dynamicAllocations) {
        int_t offset = dynamicAllocator.allocate(alloc.alignment, alloc.size);
        alloc.ptr->bufferId = 1;
        alloc.ptr->offset = offset;
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, alloc.offset, offset, alloc.size);

        stagedSize += alloc.size;
    }
    dynamicAllocations.clear();

    staticBuffer.bind(GL_COPY_WRITE_BUFFER);
    for (AllocationInfo alloc : staticAllocations) {
        int_t offset = staticAllocator.allocate(alloc.alignment, alloc.size);
        alloc.ptr->bufferId = 2;
        alloc.ptr->offset = offset;
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, alloc.offset, offset, alloc.size);

        stagedSize += alloc.size;
    }
    staticAllocations.clear();

    stagingOffset = 0;

    if (stagedSize != 0)
        log<Tag::Graphics>("staged {} bytes", stagedSize);
}

void GAllocator::pumpDynamicData() {
    dynamicCPU.bind(GL_COPY_READ_BUFFER);
    dynamicBuffer.bind(GL_COPY_READ_BUFFER);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, dynamicSize);
}

void GAllocator::free(GPtr* ptr) {
    if (ptr->bufferId == 1) {
        dynamicAllocator.free(ptr->offset);
    } else if (ptr->bufferId == 2) {
        staticAllocator.free(ptr->offset);
    } else {
        log<Tag::Graphics>("free with buffer id {}", (uint32_t)ptr->bufferId);
        VERIFY_NOT_REACHED(Graphics);
    }
    ptr->bufferId = 0;
    ptr->offset = 0;
}

void GAllocator::debug() {
    log<Tag::Graphics>("dynamic:");
    dynamicAllocator.debug();
    log<Tag::Graphics>("static:");
    staticAllocator.debug();
}

GAllocator::~GAllocator() {
    staging.unmap();
    staging.release();
    dynamicCPU.unmap();
    dynamicCPU.release();
    dynamicBuffer.release();
    dynamicBuffer.release();
    debug();
}
