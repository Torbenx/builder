#pragma once

#include "GL/Shader.hpp"
#include "containers/RangeAllocator.hpp"

namespace Tag {
struct Graphics {
    static constexpr char tag[] = "graphic";
    static constexpr bool verifyEnabled = true;
};
}

struct GPtr {
    uint32_t bufferId : 8 = 0;
    uint32_t offset : 24 = 0; // in 16 bytes
};

template<typename T>
struct TypedGPtr : GPtr { };

class GAllocator {
public:
    struct AllocationInfo {
        GPtr* ptr;
        int_t alignment;
        int_t offset;
        int_t size;
    };

    GBuffer gBuffer;
    RangeAllocator allocator;
    std::vector<AllocationInfo> allocations;

    GBuffer staging;
    std::byte* mapped;
    int_t stagingOffset = 0;

    GLint uniformAlignment = 0;
    GLint attribAlignment = 16;

    GAllocator(int_t size = 1024 * 1024);
    GAllocator(const GAllocator&) = delete;
    ~GAllocator();

    void* staticUniform(GPtr* ptr, int_t size) { return static_(ptr, uniformAlignment, size); }
    void* staticAttrib(GPtr* ptr, int_t size) { return static_(ptr, attribAlignment, size); }

    void* static_(GPtr* ptr, int_t alignment, int_t size);

    void stage();

    UniformBuffer uniform(GPtr ptr, int_t size) { return { gBuffer, ptr.offset, size }; }
    VertexAttribBuffer attrib(GPtr ptr, int_t elementSize) { return { gBuffer, ptr.offset, elementSize }; }

    void free(GPtr* ptr);

    void debug();
};
