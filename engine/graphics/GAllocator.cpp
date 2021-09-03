#include "GAllocator.hpp"

GAllocator::GAllocator(int_t size)
    : allocator(size) {

    staging.initializeWriteOnly(size);
    mapped = (std::byte*)staging.mapWritable(0, size);

    gBuffer.initializeStatic(size);

    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformAlignment);
    log<Tag::Graphics>("uniform alignment: {}", uniformAlignment);
}

void* GAllocator::static_(GPtr* ptr, int_t alignment, int_t size) {
    if (ptr->bufferId != 0) {
        free(ptr);
    }
    ptr->bufferId = 1;
    ptr->offset = 0;
    int_t offset = (stagingOffset + alignment - 1) & ~(alignment - 1);
    stagingOffset = offset + size;
    allocations.push_back({ ptr, alignment, offset, size });
    return mapped + offset;
}

void GAllocator::stage() {
    staging.bind(GL_COPY_READ_BUFFER);
    gBuffer.bind(GL_COPY_WRITE_BUFFER);
    //glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, 1024 * 1024);
    int_t stagedSize = 0;
    for (AllocationInfo alloc : allocations) {
        int_t offset = allocator.allocate(alloc.alignment, alloc.size);
        alloc.ptr->bufferId = 2;
        alloc.ptr->offset = offset;
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, alloc.offset, offset, alloc.size);

        stagedSize += alloc.size;
    }
    stagingOffset = 0;
    allocations.clear();

    if (stagedSize != 0)
        log<Tag::Graphics>("staged {} bytes", stagedSize);
}

void GAllocator::free(GPtr* ptr) {
    EXPECT(Graphics, ptr->bufferId, (uint32_t)2);
    allocator.free(ptr->offset);
    ptr->bufferId = 0;
    ptr->offset = 0;
}

void GAllocator::debug() {
    allocator.debug();
}

GAllocator::~GAllocator() {
    staging.unmap();
    staging.release();
    gBuffer.release();
    debug();
}
