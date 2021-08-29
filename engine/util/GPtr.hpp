#pragma once

#include "GL/std140.hpp"
#include "GL/Shader.hpp"

struct GBufferCollection {
    GBuffer buffer(uint8_t bufferId) {
        return GBuffer { bufferId };
    }
};

struct GPtr {
    uint32_t bufferId : 8 = 0;
    uint32_t offset : 24 = 0; // in 16 bytes
};

class GAllocator {
public:
    void* allocateStatic(GPtr* ptr, int_t size) {
        GBuffer buffer;
        buffer.initializeWriteOnly(size);
        ptr.bufferId = buffer.glId;
        ptr.offset = 0;
        return buffer.mapWritable();
    }

    void allocateDynamic(GPtr* ptr, int_t size) {

    }

    void free(GPtr ptr) {
        GBuffer buffer { ptr.bufferId };
        buffer.unmap();
        buffer.release();
    }
};

template<std140::Struct T>
class GWritePtr {
public:
    GPtr ptr;

    UniformBuffer operator()(GBufferCollection& buffers) {
        return { buffers.buffer(ptr.id), ptr.offset * 16, sizeof(T) };
    }

    T& operator[](GAllocator& allocator) {
        T* r = (T*)allocator.allocateStatic(&ptr, sizeof(T));
        return *r;
    }

    void release(GAllocator& allocator) {
        allocator.free(ptr);
    }
};
