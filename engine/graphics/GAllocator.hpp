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

    GBuffer staging, staticBuffer;
    GBuffer dynamicBuffers[2];
    GBuffer dynamicCPU[2];
    RangeAllocator dynamicAllocator, staticAllocator;
    std::vector<AllocationInfo> dynamicAllocations, staticAllocations;

    std::byte* dynamicMapped[2];
    std::byte* stagingMapped;
    int_t stagingOffset = 0;

    int_t dynamicSize, staticSize, stagingSize;
    int_t pumpCount = 0;

    GAllocator(int_t dynamicSize = 1024, int_t staticSize = 1024 * 1024, int_t stagingSize = 1024 * 1024);
    GAllocator(const GAllocator&) = delete;
    ~GAllocator();

    void* allocateDynamic(GPtr* ptr, int_t alignment, int_t size) { return queueForAllocation(ptr, alignment, size, dynamicAllocations); }
    void* allocateStatic(GPtr* ptr, int_t alignment, int_t size) { return queueForAllocation(ptr, alignment, size, staticAllocations); }

    void stage();
    void pumpDynamicData();

    std::pair<GBuffer, int_t> buffer(GPtr ptr) {
        VERIFY(Graphics, ptr.bufferId == 1 || ptr.bufferId == 2);
        if (ptr.bufferId == 1) {
            return { dynamicBuffers[readableDynamicIndex()], (uint32_t)ptr.offset };
        }
        return { staticBuffer, (uint32_t)ptr.offset };
    }
    void* dynamicMapping(GPtr ptr) {
        VERIFY(Graphics, ptr.bufferId == 1);
        return dynamicMapped[writableDynamicIndex()] + ptr.offset;
    }

    void free(GPtr* ptr);

    void debug();

private:
    void* queueForAllocation(GPtr* ptr, int_t alignment, int_t size, std::vector<AllocationInfo>& queue);
    int_t writableDynamicIndex() { return pumpCount & 1; }
    int_t readableDynamicIndex() { return (pumpCount + 1) & 1; }
};
