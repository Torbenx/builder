#pragma once

#include "containers/ArrayView.hpp"
#include "resource/Owned.hpp"
#include <atomic>
#include <cstring>
#include <mutex>

template<typename T>
class AtomicArrayBuilder {
public:
    template<typename Callback, typename Conditional>
    uint32_t pushBack(T item, Callback&& callback, Conditional&& conditional) {
        while (true) {
            uint64_t enter = this->enter.fetch_add(1, std::memory_order_acquire);
            uint16_t offset = enter;
            uint16_t capacity = enter >> 16;
            uint32_t payload = enter >> 32;
            if (!conditional(payload)) {
                return payload;
            }
            if (offset < capacity) {
                storage[offset] = item;
                callback(payload);
                exit.fetch_add(1, std::memory_order_release);
                return payload;
            }
            allocate(capacity, payload);
        }
    }

    uint32_t pushBack(T item) {
        return pushBack(
            item, [](uint32_t) {}, [](uint32_t) { return true; });
    }

    template<typename Callback>
    uint32_t pushBack(T item, Callback&& callback) {
        return pushBack(
            item, std::forward<Callback>(callback), [](uint32_t) { return true; });
    }

    template<typename Conditional>
    uint32_t pushBackIf(T item, Conditional&& conditional) {
        return pushBack(
            item, [](uint32_t) {}, std::forward<Conditional>(conditional));
    }

    Owned<ArrayView<T>> take(uint32_t payload = 0) {
        std::lock_guard<std::mutex> g(allocationLock);
        uint64_t enter = this->enter.exchange((uint64_t)payload << 32, std::memory_order_relaxed);

        uint16_t offset = enter;
        uint16_t capacity = enter >> 16;
        uint16_t size = std::min(offset, capacity);

        waitFor(size);

        exit.store(0, std::memory_order_relaxed);

        ArrayView<T> ret = { storage, size };
        storage = nullptr;
        return { ret };
    }

    void clear(uint32_t payload = 0) { take(payload).free(); }

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