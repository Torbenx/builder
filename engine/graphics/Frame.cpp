#include "Buffer.hpp"
#include "opengl/core.hpp"

namespace gfx {

Allocator::Allocator(const FrameData& copyFrom, Frame* frame, RootAllocator* root)
    : FrameData(copyFrom), m_frame(frame), root(root) { }

Allocator::Allocator(RootAllocator* root)
    : Allocator(*root, root->frame(), root) {
    root->unfinishedAllocators.fetch_add(1, std::memory_order_relaxed);
}

RootAllocator::RootAllocator(Frame* frame)
    : Allocator(*frame, frame, this) { }

Allocator* RootAllocator::fork() {
    auto child = std::make_unique<Allocator>(this);
    Allocator* ret = child.get();
    children.emplace_back(std::move(child));
    return ret;
}
void Allocator::finish() {
    int_t remaining = root->unfinishedAllocators.fetch_sub(1, std::memory_order_seq_cst) - 1;
    if (remaining > 0)
        return;
    VERIFY(GFX, remaining == 0);
    root->doFinish();
}
void RootAllocator::doFinish() {
    auto handleAllocator = [&](Allocator* allocator) {
        frame()->swImageAllocations.insert(
            frame()->swImageAllocations.end(),
            allocator->swImageAllocations.begin(),
            allocator->swImageAllocations.end());
    };
    handleAllocator(this);
    for (auto& child : children)
        handleAllocator(child.get());
    delete this;
}

void Allocator::growBufferAllocBump(uint32_t) { }

BufferMapping Allocator::allocateBuffer(uint32_t id, BufferAllocKind kind, uint32_t gpuAlignment, uint32_t size, BufferMapping prevMapping) {
    uint32_t requiredSize = stx::alignmentCeil(BUFFER_STAGING_ALLOCATION_ALIGNMENT, size);
    while (bufferAllocBump.offset + requiredSize >= bufferAllocBumpEnd) {
        growBufferAllocBump(requiredSize);
    }
    bufferAllocations.push_back({
        .id = (uint16_t)id,
        .kind = kind,
        .alignment = (uint16_t)gpuAlignment,
        .stagingMapping = bufferAllocBump,
        .size = size,
        .prevMapping = prevMapping,
    });
    BufferMapping ret = bufferAllocBump;
    bufferAllocBump += requiredSize;
    return ret;
}

uint32_t Allocator::allocateSWImage(TexelLayout format, iVec2 dimensions) {
    uint32_t id = instance()->nextSWImageId++;
    swImages[id] = sw::allocateImage(format, dimensions);
    swImageAllocations.push_back({
        .format = format,
        .id = id,
        .dimensions = dimensions,
    });
    return id;
}

void finishAllocator() {
    Allocator* alloc = allocator();
    setAllocator(nullptr);
    alloc->finish();
}

void Instance::allocateFrame(int_t index) {
    Frame& frame = frames[index];
    frame.swImages = new sw::ImageView<>[16];
    frame.bufferMappingLocations = new std::byte*[16]();
    // auto [handle, mapping] = gl::allocateWriteBuffer(4096);
    // frame.bufferMappingLocations[0] = (std::byte*)mapping;
}

Instance::Instance() {
    for (int_t i = 0; i < concurrentFrames; i++)
        allocateFrame(i);
}

Instance::~Instance() {
    // TODO: cleanup
}

bool Instance::nextFrameReady() {
    return (framesReadyMask.load() & (1u << nextFrameIndex)) != 0;
}

void Instance::beginFrame(sw::Semaphore frameComplete) {
    VERIFY(GFX, nextFrameReady());
    int_t frameIndex = nextFrameIndex++;
    if (nextFrameIndex == concurrentFrames)
        nextFrameIndex = 0;

    log("begining frame {}", frameIndex);
    Frame* frame = &frames[frameIndex];

    frame->swImageAllocations.clear();
    iteratePrevFrames(frameIndex, [&](Frame* before) {
        for (auto alloc : before->swImageAllocations) {
            frame->swImages[alloc.id] = sw::allocateImage(alloc.format, alloc.dimensions);
        }
    });

    framesReadyMask.fetch_and(~(1u << frameIndex));
    frameComplete.thenSync({
        frame,
        [](Frame* frame) {
            toInstance(frame)->framesReadyMask.fetch_or(1u << toFrameIndex(frame));
        },
    });

    setAllocator(new RootAllocator { frame });
}

}