#pragma once

#include "containers/ArrayView.hpp"
#include "util/log.hpp"
#include <atomic>
#include <mutex>

template<typename T>
class ArrayBuilder {
public:
    void pushBack(T item) {
        queue.push_back(item);
    }

    std::vector<T> get() {
        return std::move(queue);
    }

private:
    std::vector<T> queue;
};

template<typename T, int_t N>
class AtomicArrayBuilder {
public:
    void pushBack(T item) {
        int_t i = size.fetch_add(1, std::memory_order_relaxed);
        VERIFY(Containers, i < N);
        data[i] = item;
    }

    void getUnsafe() {
    }

private:
    T* data;
    std::atomic<int_t> size;
};
