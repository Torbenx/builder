#pragma once

#include "ArrayView.hpp"
#include <atomic>
#include <optional>

template<typename T>
class AtomicQueue {
private:
    struct Item {
        std::atomic<Item*> next = nullptr;
        T value;
    };

public:
    AtomicQueue() {
        Item* initialItem = new Item {};
        head.store(initialItem, std::memory_order_relaxed);
        tail.store(initialItem, std::memory_order_relaxed);
    }
    ~AtomicQueue() {
        Item* head = this->head.load(std::memory_order_relaxed);
        Item* tail = this->tail.load(std::memory_order_relaxed);
        VERIFY(Containers, head == tail);
        delete head;
    }

private:
    std::optional<T> tryReplaceHead(Item*& oldHead) {
        Item* item = oldHead->next.load(std::memory_order_seq_cst);
        if (!item || head.load(std::memory_order_relaxed) != oldHead)
            return {};
        T copy = item->value;
        // FIXME: This still has ABA problems, that can only be solved via special allocation behaviour
        if (head.compare_exchange_weak(oldHead, item, std::memory_order_relaxed)) {
            delete oldHead;
            return copy;
        }
        return {};
    }

public:
    T popFront() {
        Item* oldHead = head.load(std::memory_order_relaxed);
        while (true) {
            if (std::optional<T> ret = tryReplaceHead(oldHead))
                return ret.value();
        }
    }
    std::optional<T> tryPopFront() {
        Item* oldHead = head.load(std::memory_order_relaxed);
        return tryReplaceHead(oldHead);
    }

    void pushBack(T value) {
        Item* item = new Item { nullptr, std::move(value) };
        Item* oldTail = tail.exchange(item, std::memory_order_relaxed);
        oldTail->next.store(item, std::memory_order_seq_cst);
    }

private:
    std::atomic<Item*> head = nullptr;
    std::atomic<Item*> tail = nullptr;
};
