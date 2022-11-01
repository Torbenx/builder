#pragma once

#include "GL/Shader.hpp"
#include "containers/RangeAllocator.hpp"
#include "containers/ArrayView.hpp"

namespace Tag {
struct Graphics {
    static constexpr char tag[] = "graphic";
    static constexpr bool verifyEnabled = true;
};
}

struct GPtr {
    uint32_t bufferId : 8 = 0;
    uint32_t offset : 24 = 0;
    explicit operator bool() const { return offset != 0; }
};

template<typename T>
struct TypedGPtr : GPtr { };

class GAllocator {
public:
    static constexpr int_t DYNAMIC_BUFFER_ID = 1;
    static constexpr int_t STATIC_BUFFER_ID = 2;
    static constexpr int_t STAGING_BUFFER_ID = 3;

    static constexpr int_t dynamicQueueDepth = 4;

    struct AllocationInfo {
        GPtr* ptr;
        int_t alignment;
        int_t offset;
        int_t size;
    };

    GBuffer staging, staticBuffer;
    GBuffer dynamicBuffers[dynamicQueueDepth];
    GBuffer dynamicCPU[dynamicQueueDepth];
    RangeAllocator dynamicAllocator, staticAllocator;
    std::vector<AllocationInfo> dynamicAllocations, staticAllocations;

    std::byte* dynamicMapped[dynamicQueueDepth];
    std::byte* stagingMapped;
    int_t stagingOffset = 0;

    int_t dynamicSize, staticSize, stagingSize;
    int_t pumpCount = 0;

    GAllocator(int_t dynamicSize = 200 * 1024, int_t staticSize = 16 * 1024 * 1024, int_t stagingSize = 16 * 1024 * 1024);
    GAllocator(const GAllocator&) = delete;
    ~GAllocator();

    void* allocateDynamic(GPtr* ptr, int_t alignment, int_t size) { return queueForAllocation(ptr, alignment, size, dynamicAllocations); }
    void* allocateStatic(GPtr* ptr, int_t alignment, int_t size) { return queueForAllocation(ptr, alignment, size, staticAllocations); }

    void stage();
    void pumpDynamicData();

    std::pair<GBuffer, int_t> buffer(GPtr ptr) {
        VERIFY(Graphics, ptr.bufferId == DYNAMIC_BUFFER_ID || ptr.bufferId == STATIC_BUFFER_ID);
        if (ptr.bufferId == DYNAMIC_BUFFER_ID) {
            return { dynamicBuffers[readableDynamicIndex()], (uint32_t)ptr.offset };
        }
        return { staticBuffer, (uint32_t)ptr.offset };
    }
    void* mapping(GPtr ptr) {
        VERIFY(Graphics, ptr.bufferId == DYNAMIC_BUFFER_ID || ptr.bufferId == STAGING_BUFFER_ID);
        if (ptr.bufferId == DYNAMIC_BUFFER_ID) {
            return dynamicMapped[writableDynamicIndex()] + ptr.offset;
        }
        return stagingMapped + ptr.offset;
    }

    void free(GPtr* ptr);

    void debug();

private:
    void* queueForAllocation(GPtr* ptr, int_t alignment, int_t size, std::vector<AllocationInfo>& queue);
    int_t writableDynamicIndex() { return pumpCount % dynamicQueueDepth; }
    int_t readableDynamicIndex() { return (pumpCount + 2) % dynamicQueueDepth; }
    int_t uploadDynamicIndex() { return writableDynamicIndex(); }
};

struct GAllocator2 {
    GPtr allocateDynamic(size_t alignment, int_t size);
    void* firstDynamicUpdate(GPtr);
    GPtr receivePermanentPtr(GPtr);
    void* dynamicUpdate(GPtr);

    GPtr allocateStatic(size_t alignment, int_t size);
    void* staticUpdate(GPtr);
};
