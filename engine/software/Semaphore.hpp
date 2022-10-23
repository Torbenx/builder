#pragma once

#include "event/Script.hpp"
#include "util/Callable.hpp"
#include "util/log.hpp"
#include <atomic>

namespace sw {

namespace Tag {
    struct Software {
        static constexpr char tag[] = "software";
        static constexpr bool verifyEnabled = true;
    };
}

class Semaphore {
private:
    struct Storage {
        std::atomic<int> waitCount = 0;
        bool asyncCallback = false;
        stx::BasicCallback<> callback;
        int_t mainTick = 0;
    };

    Storage* storage = nullptr;
    Semaphore(Storage* storage)
        : storage(storage) { }

public:
    Semaphore() = default;
    Semaphore(Semaphore&& other)
        : storage(other.storage) {
        other.storage = nullptr;
    }
    ~Semaphore() {
        VERIFY(Software, !(bool)storage);
    }

    Semaphore& operator=(Semaphore&& other) {
        VERIFY(Software, !(bool)storage);
        storage = other.storage;
        other.storage = nullptr;
        return *this;
    }

    Semaphore fork() {
        VERIFY(Software, (bool)storage);
        storage->waitCount.fetch_add(1);
        return { storage };
    }

    static Semaphore create();
    void signal();
    void destroy();

    void setSyncCallback(stx::BasicCallback<> callback) {
        VERIFY(Software, (bool)storage);
        storage->callback = callback;
        storage->asyncCallback = false;
        storage->mainTick = evt::mainTick();
    }
    void setAsyncCallback(stx::BasicCallback<> callback) {
        VERIFY(Software, (bool)storage);
        storage->callback = callback;
        storage->asyncCallback = true;
        storage->mainTick = evt::mainTick();
    }
    void thenSync(stx::BasicCallback<> callback) {
        setSyncCallback(callback);
        signal();
    }
    void thenAsync(stx::BasicCallback<> callback) {
        setAsyncCallback(callback);
        signal();
    }

    explicit operator bool() const { return (bool)storage; }
};

}