#pragma once

#include "GL/Shader.hpp"

struct GPtr {
    uint32_t bufferId : 8 = 0;
    uint32_t offset : 24 = 0; // in 16 bytes
};

class GAllocator {
public:
    GAllocator() { }
    GAllocator(const GAllocator&) = delete;

    void* staticData(GPtr* ptr, int_t size) {
        GBuffer buffer { ptr->bufferId };
        if (buffer.glId != 0)
            buffer.release();
        buffer.initializeWriteOnly(size);
        ptr->bufferId = buffer.glId;
        ptr->offset = 0;
        return buffer.mapWritable(0, size);
    }

    void dynamicData(GPtr*, int_t) {
    }

    GBuffer buffer(uint8_t bufferId) {
        return GBuffer { bufferId };
    }

    UniformBuffer buffer(GPtr ptr, int_t size) {
        return { buffer(ptr.bufferId), ptr.offset, size };
    }

    void free(GPtr* ptr) {
        GBuffer buffer { ptr->bufferId };
        buffer.unmap();
        buffer.release();
        ptr->bufferId = 0;
        ptr->offset = 0;
    }
};

template<typename T>
class StaticGData {
public:
    GPtr ptr;

    GBuffer operator()(GAllocator& allocator) {
        return allocator.buffer(ptr.bufferId);
    }

    T& update(GAllocator& allocator) {
        T* r = (T*)allocator.staticData(&ptr, sizeof(T));
        return *r;
    }

    void release(GAllocator& allocator) {
        allocator.free(&ptr);
    }
};
