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
    GLint uniformAlignment = 0;
    GLint attribAlignment = 16;

    GAllocator(int_t size = 1024 * 1024)
        : allocator(size) {
        gBuffer.initializeWriteOnly(size);
        mapped = (std::byte*)gBuffer.mapWritable(0, size);
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformAlignment);
        log<Tag::GL>("uniform alignment: {}", uniformAlignment);
    }
    GAllocator(const GAllocator&) = delete;

    void* staticUniform(GPtr* ptr, int_t size) { return static_(ptr, uniformAlignment, size); }

    void* staticAttrib(GPtr* ptr, int_t size) { return static_(ptr, attribAlignment, size); }

    void* static_(GPtr* ptr, int_t alignment, int_t size) {
        if (ptr->bufferId != 0)
            free(ptr);
        ptr->bufferId = 1;
        ptr->offset = allocator.allocate(alignment, size);
        return mapped + ptr->offset;
    }

    UniformBuffer uniform(GPtr ptr, int_t size) {
        return { gBuffer, ptr.offset, size };
    }

    VertexAttribBuffer attrib(GPtr ptr, int_t elementSize) {
        return { gBuffer, ptr.offset, elementSize };
    }

    void free(GPtr* ptr) {
        EXPECT(GL, ptr->bufferId, (uint32_t)1);
        allocator.free(ptr->offset);
        ptr->bufferId = 0;
        ptr->offset = 0;
    }

    ~GAllocator() {
        gBuffer.unmap();
        gBuffer.release();
    }
};
