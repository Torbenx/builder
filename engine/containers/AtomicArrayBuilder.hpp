#pragma once

#include "containers/ArrayView.hpp"
#include "resource/Owned.hpp"
#include <atomic>
#include <cstring>
#include <mutex>
#include <shared_mutex>

template<typename T>
class AtomicArrayBuilder {
public:
    template<typename Callback>
    uint32_t pushBack(const T& item, Callback&& callback) {
        while (true) {
            int_t capacity;
            {
                std::shared_lock<std::shared_mutex> g(lock);
                int_t offset = this->offset.fetch_add(1, std::memory_order_relaxed);
                capacity = this->capacity;
                if (offset < capacity) {
                    std::construct_at(storage + offset, item);
                    callback(payload);
                    return payload;
                }
            }
            allocate(capacity);
        }
    }

    uint32_t pushBack(const T& item) {
        return pushBack(
            item, [](uint32_t) {});
    }

    Owned<ArrayView<T>> take(uint32_t payload = 0) {
        std::unique_lock<std::shared_mutex> g(lock);
        int_t offset = this->offset.load(std::memory_order_relaxed);
        int_t size = std::min(offset, capacity);
        ArrayView<T> ret = { storage, size };

        storage = nullptr;
        this->offset.store(0, std::memory_order_relaxed);
        capacity = 0;
        this->payload = payload;

        return { ret };
    }

    void clear(uint32_t payload = 0) { take(payload).release(); }

    ~AtomicArrayBuilder() { delete[] storage; }

private:
    T* storage = nullptr;
    std::atomic<int_t> offset = 0;
    int_t capacity = 0;
    uint32_t payload = 0;
    std::shared_mutex lock;

    void allocate(int_t oldCapacity) {
        std::unique_lock<std::shared_mutex> g(lock);
        if (capacity != oldCapacity)
            return;

        int_t newCapacity = capacity == 0 ? 4 : capacity * 2;
        std::allocator<T> allocator;
        T* newStorage = allocator.allocate(newCapacity);

        std::copy(storage, storage + capacity, newStorage);

        allocator.deallocate(storage, capacity);

        storage = newStorage;
        offset.store(capacity, std::memory_order_relaxed);
        capacity = newCapacity;
    }
};
