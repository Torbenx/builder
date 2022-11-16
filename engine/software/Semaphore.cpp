#include "Semaphore.hpp"
#include "event/Timeline.hpp"

namespace sw {

Semaphore Semaphore::create() {
    return Semaphore { new Storage { .waitCount = 1 } };
}

Semaphore Semaphore::merge(ArrayView<Semaphore> in) {
    Semaphore out = create();
    for (Semaphore& semaphore : in)
        semaphore.thenSync({
            out.fork().asPtr(),
            [](void* ptr) { fromPtr(ptr).signal(); },
        });
    return out;
}

void Semaphore::signal() {
    VERIFY(Software, (bool)storage);
    int_t i = storage->waitCount.fetch_sub(1);
    i -= 1;
    if (i != 0) {
        storage = nullptr;
        return;
    }
    stx::BasicCallback copy = storage->callback;
    VERIFY(Software, (bool)copy);
    bool async = storage->asyncCallback;
    int_t mainTick = storage->mainTick;
    delete storage;
    storage = nullptr;
    if (async) {
        evt::async(copy, mainTick);
    } else {
        int_t restoreTick = evt::mainTick();
        evt::setMainTick(mainTick);
        copy();
        evt::setMainTick(restoreTick);
    }
}

void Semaphore::destroy() {
    VERIFY(Software, (bool)storage);
    int_t count = storage->waitCount.load();
    VERIFY(Software, count == 1);
    delete storage;
    storage = nullptr;
}

bool Semaphore::destroyWhenReady() {
    VERIFY(Software, (bool)storage);
    int_t count = storage->waitCount.load();
    if (count > 1)
        return false;
    VERIFY(Software, count == 1);
    delete storage;
    storage = nullptr;
    return true;
}

void Mutex::Unlock::handoverToNext() {
    Mutex::Link* current = mutex->current;
    mutex->current = nullptr;
    Mutex::Link* expected = current;
    if (mutex->last.compare_exchange_strong(expected, nullptr)) {
        delete current;
        return;
    }
    Mutex::Link* next;
    while ((next = current->next.load()) == nullptr) {
        VERIFY_NOT_REACHED(Software);
    }
    delete current;
    mutex->current = next;
    next->callback(*this);
}

void Mutex::Unlock::deallocate() {
    if (!mutex)
        return;
    handoverToNext();
    mutex = nullptr;
}

Mutex::LockRequest::LockRequest(Mutex* mutex)
    : m_mutex(mutex), link(new Link), prevOwner(mutex->last.exchange(link)) {
    if (prevOwner == nullptr)
        mutex->current = link;
}

stx::Owned<Mutex::Unlock> Mutex::LockRequest::lockIfReady() {
    if (prevOwner)
        return {};
    return Unlock { mutex() };
}

void Mutex::LockRequest::wait(stx::BasicCallback<stx::Owned<Unlock>> callback) {
    if (prevOwner == nullptr) {
        callback({ mutex() });
        return;
    }
    link->callback = callback;
    prevOwner->next.store(link);
}

}