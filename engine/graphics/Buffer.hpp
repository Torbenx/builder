#pragma once

#include "platform/thread_local_storage.hpp"
#include "software/Image.hpp"
#include "util/Callable.hpp"
#include "util/types.hpp"
#include <bit>
#include <mutex>

namespace Tag {
struct GFX {
    static constexpr char tag[] = "graphics";
    static constexpr bool verifyEnabled = true;
};
}

namespace gfx {

using iVec2 = sw::iVec2;

inline constexpr size_t BUFFER_STAGING_ALLOCATION_ALIGNMENT = 16;

template<typename T = void>
class Buffer;

struct BufferInfo {
    Buffer<>* buffer = nullptr;
};
struct BufferMapping {
    uint32_t location = 0;
    uint32_t offset = 0;

    std::byte* mapping() const;
    BufferMapping& operator+=(int32_t shift) {
        offset += shift;
        return *this;
    }
    BufferMapping& operator-=(int32_t shift) {
        offset -= shift;
        return *this;
    }
};
inline BufferMapping operator+(const BufferMapping& m, int32_t shift) {
    return { m.location, m.offset + shift };
}
inline BufferMapping operator-(const BufferMapping& m, int32_t shift) {
    return { m.location, m.offset - shift };
}

enum class BufferAllocKind : uint16_t {
    Dynamic,
    Static,
    Freeze,
    Destroy,
};
struct BufferAllocInfo {
    uint16_t id = 0;
    BufferAllocKind kind : 2 = (BufferAllocKind)0;
    uint16_t alignment : 14 = 0;
    BufferMapping stagingMapping;
    uint32_t size = 0;
    BufferMapping prevMapping;
};

struct SWImageAllocInfo {
    TexelLayout format = (TexelLayout)0;
    uint32_t id = 0;
    iVec2 dimensions;
};

struct HWObject {
    uint64_t handle = 0;
    HWObject() = default;
    HWObject(uint64_t handle)
        : handle(handle) { }
};
struct HWBuffer : HWObject {
    uint32_t offset = 0;
    uint32_t size = 0;
};

inline constexpr int_t MAX_CONCURRENT_FRAMES = 8;

struct Frame {
    std::byte** bufferMappingLocations = nullptr;
    std::byte* get(BufferMapping buffer) const {
        return bufferMappingLocations[buffer.location] + buffer.offset;
    }

    BufferMapping bufferAllocBump;
    uint32_t bufferAllocBumpEnd = 0;
    std::vector<BufferAllocInfo> bufferAllocations;
    void growBufferAllocBump(uint32_t requiredSize);
    BufferMapping allocateBuffer(uint32_t id, BufferAllocKind kind, uint32_t gpuAlignment, uint32_t size, BufferMapping prevMapping);

    sw::ImageView<>* swImages = nullptr;
    std::vector<SWImageAllocInfo> swImageAllocations;
    uint32_t allocateSWImage(TexelLayout format, iVec2 dimensions);

    HWBuffer* hwBuffers = nullptr;
    HWObject* hwObjects = nullptr;
};
DECLARE_THREAD_LOCAL_GETSET(Frame*, frame, setFrame, GRAPHICS_FRAME)
inline constexpr size_t INSTANCE_ALIGNMENT = std::bit_ceil(sizeof(Frame) * MAX_CONCURRENT_FRAMES);

struct alignas(INSTANCE_ALIGNMENT) Instance {
    Frame frames[MAX_CONCURRENT_FRAMES] = {};

    BufferInfo* bufferInfos = nullptr;

    uint32_t nextSWImageId = 1;

    int_t nextFrameIndex = 0;
    int_t concurrentFrames = 4;
    std::atomic<uint32_t> framesReadyMask = (1u << concurrentFrames) - 1;
    Instance();
    ~Instance();
    bool nextFrameReady();
    void beginFrame(sw::Semaphore frameComplete);
    void clearFrame();
    void allocateFrame(int_t index);
    template<typename Callback>
    void iteratePrevFrames(int_t index, Callback&& callback) {
        for (int_t i = index - concurrentFrames + 1; i < index; i++) {
            int_t j = i >= 0 ? i : i + concurrentFrames;
            Frame* frame = &frames[j];
            callback(frame);
        }
    }
};
static_assert(offsetof(Instance, frames[0]) == 0);

inline Instance* toInstance(Frame* frame) {
    return (Instance*)stx::alignmentFloor(INSTANCE_ALIGNMENT, (uintptr_t)frame);
}
inline int_t toFrameIndex(Frame* frame) {
    return frame - (Frame*)toInstance(frame);
}
inline Instance* instance() { return toInstance(frame()); }

template<typename T = void>
struct BufferHandle {
    uint32_t id = 0;
};

template<>
class Buffer<void> {
protected:
    uint16_t m_id = 0;
    uint16_t m_loc = 0;
    uint32_t m_offset = 0;

    uint32_t id() const { return m_id; }
    uint32_t loc() const { return m_loc; }
    uint32_t offset() const { return m_offset; }

    void copyFields(const Buffer& other) {
        m_id = other.id();
        m_loc = other.loc();
        m_offset = other.offset();
    }
    void clearFields() {
        m_id = 0;
        m_loc = 0;
        m_offset = 0;
    }

    BufferInfo* info(Frame* frame) const {
        return toInstance(frame)->bufferInfos + id();
    }

    BufferMapping bufferMapping() const { return { loc(), offset() }; }
    void setMapping(BufferMapping mapping) {
        m_loc = mapping.location;
        m_offset = mapping.offset;
    }

    void moveFrom(Buffer&& other, Frame* frame) {
        copyFields(other);
        if (other) {
            BufferInfo* info = other.info(frame);
            VERIFY(GFX, info->buffer == &other);
            info->buffer = this;
        }
        other.clearFields();
    }

    void* allocate(BufferAllocKind kind, uint32_t alignment, uint32_t size, Frame* frame) {
        BufferMapping mapping = frame->allocateBuffer(id(), kind, alignment, size, bufferMapping());
        setMapping(mapping);
        return frame->get(mapping);
    }
    void deallocateInternal(Frame* frame) {
        allocate(BufferAllocKind::Destroy, 0, 0, frame);
    }

public:
    Buffer() = default;
    Buffer(const Buffer&) = delete;

    Buffer(Buffer&& other, Frame* frame = gfx::frame()) {
        moveFrom(std::move(other), frame);
    }
    Buffer& operator=(Buffer&& other) {
        if (*this)
            deallocateInternal(frame());
        moveFrom(std::move(other), frame());
        return *this;
    }

    void* mapping(Frame* frame = gfx::frame()) const {
        return frame->get(bufferMapping());
    }

    ~Buffer() {
        if (*this)
            deallocateInternal(frame());
    }
    void deallocate(Frame* frame = gfx::frame()) {
        if (*this) {
            deallocateInternal(frame);
            clearFields();
        }
    }

    void* staticUpdate(uint32_t alignment, uint32_t size, Frame* frame = gfx::frame()) {
        return allocate(BufferAllocKind::Static, alignment, size, frame);
    }
    void* beginDynamicUpdates(uint32_t alignment, uint32_t size, Frame* frame = gfx::frame()) {
        return allocate(BufferAllocKind::Dynamic, alignment, size, frame);
    }
    void endDynamicUpdates(Frame* frame = gfx::frame()) {
        allocate(BufferAllocKind::Freeze, 0, 0, frame);
    }

    explicit operator bool() const { return id() != 0; }
    operator BufferHandle<>() const { return { id() }; }
};

template<typename T>
class Buffer : public Buffer<> {
    using Base = Buffer<>;

public:
    operator BufferHandle<T>() const { return { id() }; }
    void* staticUpdate(Frame* frame = gfx::frame()) {
        return Base::staticUpdate(alignof(T), sizeof(T), frame);
    }
    void* beginDynamicUpdates(Frame* frame = gfx::frame()) {
        return Base::beginDynamicUpdates(alignof(T), sizeof(T), frame);
    }
    T* mapping(Frame* frame = gfx::frame()) const {
        return static_cast<T*>(Base::mapping(frame));
    }
};
template<typename T>
class Buffer<T[]> : public Buffer<> {
    using Base = Buffer<>;

public:
    operator BufferHandle<T[]>() const { return { id() }; }
    void* staticUpdate(int_t size, Frame* frame = gfx::frame()) {
        return Base::staticUpdate(alignof(T), sizeof(T) * size, frame);
    }
    void* beginDynamicUpdates(int_t size, Frame* frame = gfx::frame()) {
        return Base::beginDynamicUpdates(alignof(T), sizeof(T) * size, frame);
    }
    T* mapping(Frame* frame = gfx::frame()) const {
        return static_cast<T*>(Base::mapping(frame));
    }
};

template<TexelLayout layout = TexelLayout::Unknown>
class SWImageHandle {
protected:
    uint32_t id = 0;
    SWImageHandle(uint32_t id)
        : id(id) { }

public:
    SWImageHandle() = default;
    static SWImageHandle createDynamic(iVec2 dimensions, Frame* frame = gfx::frame()) {
        return { frame->allocateSWImage(layout, dimensions) };
    }

    sw::ImageView<layout> get(Frame* frame = gfx::frame()) const {
        return (sw::ImageView<layout>)frame->swImages[id];
    }
    void deallocate() {
        VERIFY_NOT_REACHED(GFX);
        *this = {};
    }
    explicit operator bool() const { return id != 0; }
};

enum class TexelComponents {
    D16,
    RGBA8,
};

enum class ImageType : uint32_t {
    Texture,
    CubeFacePosX,
    CubeFaceNegX,
    CubeFacePosY,
    CubeFaceNegY,
    CubeFacePosZ,
    CubeFaceNegZ,
    COUNT,
};

template<TexelLayout layout = TexelLayout::Unknown>
struct Image;

template<>
struct Image<TexelLayout::Unknown> {
    uint32_t baseId : 24 = 0;
    uint32_t mipLevel : 4 = 0;
    ImageType type : 4 = (ImageType)0;
};

template<TexelLayout layout>
struct Image : Image<> {
    using Base = Image<>;
};

template<TexelComponents comps>
struct ImageHandle : Image<> { };

template<TexelLayout layout>
struct Texture {
    uint32_t id = 0;
    Image<layout> image(int_t level = 0) const {
        return {
            .baseId = id,
            .mipLevel = (uint32_t)level,
            .type = ImageType::Texture,
        };
    }
    operator Image<layout>() const { return image(); }
};

template<typename S>
class Shader {
private:
public:
    const S& shader();
    S& mutate();
};

}