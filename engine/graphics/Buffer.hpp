#pragma once

#include "platform/thread_local_storage.hpp"
#include "util/types.hpp"

namespace gfx {

inline constexpr size_t BUFFER_CPU_ALLOCATION_ALIGNMENT = 16;

class Buffer;

struct BufferInfo {
    Buffer* buffer = nullptr;
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

struct BufferManager {
    enum class AllocationKind : uint16_t {
        Dynamic,
        Static,
        Freeze,
        Destroy,
    };
    struct alignas(BUFFER_CPU_ALLOCATION_ALIGNMENT) Header {
        uint16_t id = 0;
        AllocationKind kind : 2 = (AllocationKind)0;
        uint16_t alignment : 14 = 0;
        uint32_t size = 0;
        BufferMapping prevMapping;
    };

    std::byte* const* mappingLocations = nullptr;
    BufferInfo* infos = nullptr;

    BufferMapping allocBump;
    uint32_t allocBumpSize = 0;
    stx::BasicCallback<BufferManager&, uint32_t> allocBumpCallback;
    BufferMapping allocate(uint32_t id, AllocationKind kind, uint32_t gpuAlignment, uint32_t size, BufferMapping prevMapping) {
        uint32_t requiredSize = sizeof(Header) + stx::alignmentCeil(BUFFER_CPU_ALLOCATION_ALIGNMENT, size);
        while (allocBump.offset + requiredSize >= allocBumpSize) [[unlikely]] {
            allocBumpCallback(*this, requiredSize);
        }
        new (get(allocBump)) Header {
            .id = (uint16_t)id,
            .kind = kind,
            .alignment = (uint16_t)gpuAlignment,
            .size = size,
            .prevMapping = prevMapping,
        };
        BufferMapping ret = allocBump + sizeof(Header);
        allocBump += requiredSize;
        return ret;
    }

    std::byte* get(BufferMapping buffer) const {
        return mappingLocations[buffer.location] + buffer.offset;
    }
};
DECLARE_THREAD_LOCAL_GETSET(BufferManager*, bufferManager, setBufferManager, GRAPHICS_BUFFER_MANAGER)

class Buffer {
private:
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
        return bufferManager()->infos + id();
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

    using AllocationKind = BufferManager::AllocationKind;
    void* allocate(AllocationKind kind, uint32_t alignment, uint32_t size) {
        BufferManager* manager = bufferManager();
        BufferMapping mapping = manager->allocate(id(), kind, alignment, size, bufferMapping());
        setMapping(mapping);
        return manager->get(mapping);
    }
    void deallocateInternal() {
        allocate(AllocationKind::Destroy, 0, 0);
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
        return bufferManager()->get(bufferMapping());
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
        return allocate(AllocationKind::Static, alignment, size);
    }
    void* beginDynamicUpdates(uint32_t alignment, uint32_t size) {
        return allocate(AllocationKind::Dynamic, alignment, size);
    }
    void endDynamicUpdates() {
        allocate(AllocationKind::Freeze, 0, 0);
    }

    explicit operator bool() const { return id() != 0; }
};

}