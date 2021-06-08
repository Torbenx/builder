#pragma once

//T should have a field next of type std::atomic<T*>
template<class T>
class AtomicList {
public:
    AtomicList()
        : initial(nullptr), head(&initial), tail(&initial) { }

    T* popFront() {
        while (true) {
            std::atomic<T*>* oldHead = head.load(std::memory_order_relaxed);
            T* t = oldHead->load(std::memory_order_acquire);
            if (t && head.compare_exchange_weak(oldHead, &(t->next), std::memory_order_relaxed))
                return t;
        }
    }

    void pushBack(T* t) {
        t->next.store(nullptr, std::memory_order_relaxed);
        std::atomic<T*>* oldTail = tail.exchange(&(t->next), std::memory_order_relaxed);
        oldTail->store(t, std::memory_order_release);
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
