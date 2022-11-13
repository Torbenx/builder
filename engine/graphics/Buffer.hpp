#pragma once

#include "platform/thread_local_storage.hpp"
#include "software/Image.hpp"
#include "util/Callable.hpp"
#include "util/types.hpp"
#include <bit>

namespace gfx {

inline constexpr size_t BUFFER_CPU_ALLOCATION_ALIGNMENT = 16;

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
struct alignas(BUFFER_CPU_ALLOCATION_ALIGNMENT) BufferAllocHeader {
    uint16_t id = 0;
    BufferAllocKind kind : 2 = (BufferAllocKind)0;
    uint16_t alignment : 14 = 0;
    uint32_t size = 0;
    BufferMapping prevMapping;
};

struct Frame {
    std::byte** bufferMappingLocations = nullptr;
    std::byte* get(BufferMapping buffer) const {
        return bufferMappingLocations[buffer.location] + buffer.offset;
    }

    BufferMapping bufferAllocBump;
    uint32_t bufferAllocBumpSize = 0;
    void growBufferAllocBump(uint32_t requiredSize);
    BufferMapping allocateBuffer(uint32_t id, BufferAllocKind kind, uint32_t gpuAlignment, uint32_t size, BufferMapping prevMapping);

    sw::ImageView<>* softwareImages = nullptr;
};

inline constexpr int_t MAX_CONCURRENT_FRAMES = 4;
inline constexpr size_t INSTANCE_ALIGNMENT = std::bit_ceil(sizeof(Frame) * MAX_CONCURRENT_FRAMES);

struct alignas(INSTANCE_ALIGNMENT) Instance {
    Frame frames[MAX_CONCURRENT_FRAMES] = {};

    BufferInfo* bufferInfos = nullptr;
};
static_assert(offsetof(Instance, frames[0]) == 0);

DECLARE_THREAD_LOCAL_GETSET(Frame*, frame, setFrame, GRAPHICS_FRAME)
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

    BufferInfo* info() const {
        return instance()->bufferInfos + id();
    }

    BufferMapping bufferMapping() const { return { loc(), offset() }; }
    void setMapping(BufferMapping mapping) {
        m_loc = mapping.location;
        m_offset = mapping.offset;
    }

    void moveFrom(Buffer&& other) {
        copyFields(other);
        if (other) {
            BufferInfo* info = other.info();
            // VERIFY(Graphics, info->buffer == &other);
            info->buffer = this;
        }
        other.clearFields();
    }

    void* allocate(BufferAllocKind kind, uint32_t alignment, uint32_t size) {
        Frame* frame = gfx::frame();
        BufferMapping mapping = frame->allocateBuffer(id(), kind, alignment, size, bufferMapping());
        setMapping(mapping);
        return frame->get(mapping);
    }
    void deallocateInternal() {
        allocate(BufferAllocKind::Destroy, 0, 0);
    }

public:
    Buffer() = default;
    Buffer(const Buffer&) = delete;

    Buffer(Buffer&& other) {
        moveFrom(std::move(other));
    }
    Buffer& operator=(Buffer&& other) {
        if (*this)
            deallocateInternal();
        moveFrom(std::move(other));
        return *this;
    }

    void* mapping() const {
        return frame()->get(bufferMapping());
    }

    ~Buffer() {
        if (*this)
            deallocateInternal();
    }
    void deallocate() {
        if (*this) {
            deallocateInternal();
            clearFields();
        }
    }

    void* staticUpdate(uint32_t alignment, uint32_t size) {
        return allocate(BufferAllocKind::Static, alignment, size);
    }
    void* beginDynamicUpdates(uint32_t alignment, uint32_t size) {
        return allocate(BufferAllocKind::Dynamic, alignment, size);
    }
    void endDynamicUpdates() {
        allocate(BufferAllocKind::Freeze, 0, 0);
    }

    explicit operator bool() const { return id() != 0; }
    operator BufferHandle<>() const { return { id() }; }
};

template<typename T>
class Buffer : public Buffer<> {
    using Base = Buffer<>;

public:
    operator BufferHandle<T>() const { return { id() }; }
    void* staticUpdate() {
        return Base::staticUpdate(alignof(T), sizeof(T));
    }
    void* beginDynamicUpdates() {
        return Base::beginDynamicUpdates(alignof(T), sizeof(T));
    }
    T* mapping() const {
        return static_cast<T*>(Base::mapping());
    }
};
template<typename T>
class Buffer<T[]> : public Buffer<> {
    using Base = Buffer<>;

public:
    operator BufferHandle<T[]>() const { return { id() }; }
    void* staticUpdate(int_t size) {
        return Base::staticUpdate(alignof(T), sizeof(T) * size);
    }
    void* beginDynamicUpdates(int_t size) {
        return Base::beginDynamicUpdates(alignof(T), sizeof(T) * size);
    }
    T* mapping() const {
        return static_cast<T*>(Base::mapping());
    }
};

template<TexelLayout layout>
class SWImageHandle {
protected:
    uint32_t id = 0;

public:
    sw::ImageView<layout> get() const {
        return (sw::ImageView<layout>)frame()->softwareImages[id];
    }
};

enum class TexelComponents {
    D16,
    RGBA8,
};

enum class ImageType : uint32_t {
    Texture,
    Renderbuffer,
    CubeFacePosX,
    CubeFaceNegX,
    CubeFacePosY,
    CubeFaceNegY,
    CubeFacePosZ,
    CubeFaceNegZ,
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
class ShaderInstance {
private:
public:
    const S& shader();
    S& mutate();
};

}