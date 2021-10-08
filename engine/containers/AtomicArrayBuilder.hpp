#pragma once

#include "containers/ArrayView.hpp"
#include <atomic>
#include <mutex>

template<typename T>
class AtomicArrayBuilder {
public:
    uint32_t pushBack(T item) {
        while (true) {
            uint64_t enter = this->enter.fetch_add(1, std::memory_order_acquire);
            uint16_t offset = enter;
            uint16_t capacity = enter >> 16;
            if (offset < capacity) {
                storage[offset] = item;
                exit.fetch_add(1, std::memory_order_release);
                return enter >> 32;
            }
            allocate(capacity, enter >> 32);
        }
    }

    ArrayView<T> take(uint32_t payload = 0) {
        std::lock_guard<std::mutex> g(allocationLock);
        uint64_t enter = this->enter.exchange((uint64_t)payload << 32, std::memory_order_relaxed);

        uint16_t offset = enter;
        uint16_t capacity = enter >> 16;
        uint16_t size = std::min(offset, capacity);

        waitFor(size);

        exit.store(0, std::memory_order_relaxed);

        ArrayView<T> ret = { storage, size };
        storage = nullptr;
        return ret;
    }

    void clear(uint32_t payload = 0) { free(take(payload)); }

    static void free(ArrayView<T> view) { delete[] view.data(); }

    ~AtomicArrayBuilder() { delete[] storage; }

private:
    T* storage = nullptr;
    std::atomic<uint16_t> exit = 0;
    std::atomic<uint64_t> enter = 0;
    std::mutex allocationLock;

    void waitFor(uint16_t waitFor) {
        while (exit.load(std::memory_order_acquire) < waitFor) { }
    }

    void allocate(uint16_t oldCapacity, uint32_t payload) {
        std::lock_guard<std::mutex> g(allocationLock);

        uint16_t newCapacity = oldCapacity == 0 ? (uint16_t)4 : oldCapacity * 2;
        T* newStorage = new T[newCapacity];

        waitFor(oldCapacity);

        std::memcpy(newStorage, storage, oldCapacity * sizeof(T));
        delete[] storage;
        storage = newStorage;

        enter.store((uint64_t)payload << 32 | (uint64_t)newCapacity << 16 | (uint64_t)oldCapacity, std::memory_order_release);
    }
};