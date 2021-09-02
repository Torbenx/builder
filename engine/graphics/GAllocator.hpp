#pragma once

#include "GL/Shader.hpp"
#include "containers/RangeAllocator.hpp"

struct GPtr {
    uint32_t bufferId : 8 = 0;
    uint32_t offset : 24 = 0; // in 16 bytes
};

class GAllocator {
public:
    GBuffer gBuffer;
    RangeAllocator allocator;
    std::byte* mapped;
    GLint alignment = 0;

    GAllocator(int_t size = 1024 * 1024)
        : allocator(size) {
        gBuffer.initializeWriteOnly(size);
        mapped = (std::byte*)gBuffer.mapWritable(0, size);
        allocator.allocate(1, 63);
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
        log<Tag::GL>("uniform alignment: {}", alignment);
    }
    GAllocator(const GAllocator&) = delete;

    void* staticData(GPtr* ptr, int_t size) {
        if (ptr->bufferId != 0)
            free(ptr);
        ptr->bufferId = 1;
        ptr->offset = allocator.allocate(alignment, size);
        return mapped + ptr->offset;
    }

    void dynamicData(GPtr*, int_t) { }

    UniformBuffer buffer(GPtr ptr, int_t size) {
        return { gBuffer, ptr.offset, size };
    }

    void free(GPtr* ptr) {
        EXPECT(GL, ptr->bufferId, (uint32_t)1);
        allocator.free(ptr->offset);
    }

    ~GAllocator() {
        gBuffer.unmap();
        gBuffer.release();
    }
};
