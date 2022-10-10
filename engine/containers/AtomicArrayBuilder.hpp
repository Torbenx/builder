#pragma once

#include "ArrayView.hpp"
#include "resource/Owned.hpp"
#include <atomic>
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

    struct TakeReturnTypeView : ArrayView<T> {
        int_t capacity = 0;
        void destroy() {
            std::destroy_n(this->data(), this->size());
            std::allocator<T>().deallocate(this->data(), capacity);
        }
    };
    using TakeReturnType = Owned<TakeReturnTypeView>;
    TakeReturnType take(uint32_t payload = 0) {
        std::unique_lock<std::shared_mutex> g(lock);
        int_t offset = this->offset.load(std::memory_order_relaxed);
        int_t size = std::min(offset, capacity);
        TakeReturnTypeView ret = { { storage, size }, capacity };

        storage = nullptr;
        this->offset.store(0, std::memory_order_relaxed);
        capacity = 0;
        this->payload = payload;

        return ret;
    }

    void clear(uint32_t payload = 0) { take(payload).release(); }

    ~AtomicArrayBuilder() {
        take();
    }

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

        std::uninitialized_copy_n(storage, capacity, newStorage);

        allocator.deallocate(storage, capacity);

        storage = newStorage;
        offset.store(capacity, std::memory_order_relaxed);
        capacity = newCapacity;
    }
};
