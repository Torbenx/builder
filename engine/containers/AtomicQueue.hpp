#pragma once

#include <atomic>
#include <concepts>

template<class T>
struct AtomicQueueItem {
    std::atomic<T*> next = nullptr;
};

template<class T>
concept AtomicQueueable = std::derived_from<T, AtomicListItem<T>>;

template<AtomicQueueable T>
class AtomicList {
public:
    AtomicList()
        : initial(nullptr), head(&initial), tail(&initial) { }

    T* popFront() {
        std::atomic<T*>* oldHead = head.load(std::memory_order_relaxed);
        while (true) {
            T* t = oldHead->load(std::memory_order_seq_cst);
            if (t && head.compare_exchange_weak(oldHead, &(t->next), std::memory_order_relaxed)) {
                return t;
            }
        }
    }

    void pushBack(T* t) {
        t->next.store(nullptr, std::memory_order_relaxed);
        std::atomic<T*>* oldTail = tail.exchange(&(t->next), std::memory_order_relaxed);
        oldTail->store(t, std::memory_order_seq_cst);
    }

    void pushBack(AtomicList<T>* l) {
        std::atomic<T*>* oldTail = tail.exchange(l->tail.load(std::memory_order_relaxed), std::memory_order_relaxed);
        oldTail->store(l->head.load(std::memory_order_relaxed)->load(std::memory_order_acquire), std::memory_order_release);
    }

    void clear() {
        initial.store(nullptr, std::memory_order_relaxed);
        tail.store(&initial);
        head.store(&initial);
    }

private:
    std::atomic<T*> initial;
    std::atomic<std::atomic<T*>*> head;
    std::atomic<std::atomic<T*>*> tail;
};
