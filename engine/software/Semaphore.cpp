#include "Semaphore.hpp"
#include "event/Timeline.hpp"

namespace sw {

Semaphore Semaphore::create() {
    return Semaphore { new Storage { .waitCount = 1 } };
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
    bool async = storage->asyncCallback;
    delete storage;
    storage = nullptr;
    if (async)
        evt::async(copy);
    else
        copy();
}

void Semaphore::destroy() {
    VERIFY(Software, (bool)storage);
    int_t count = storage->waitCount.load(std::memory_order_relaxed);
    VERIFY(Software, count == 1);
    delete storage;
    storage = nullptr;
}

}