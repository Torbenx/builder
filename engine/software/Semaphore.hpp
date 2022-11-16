#pragma once

#include "containers/Owned.hpp"
#include "event/Script.hpp"
#include "util/Callable.hpp"
#include "util/coroutines.hpp"
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

    bool destroyWhenReady();

    static Semaphore create();
    static Semaphore merge(ArrayView<Semaphore>);
    static Semaphore fromPtr(void* ptr) { return { (Storage*)ptr }; }
    void signal();
    void destroy();

    void* asPtr() {
        Storage* storage = this->storage;
        this->storage = nullptr;
        return storage;
    }

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

struct SemaphoreAwaitable {
    Semaphore& semaphore;
    bool await_ready() {
        return semaphore.destroyWhenReady();
    }
    void await_suspend(stdx::coroutine_handle<> handle) {
        semaphore.thenAsync({
            handle.address(),
            [](void* address) {
                stdx::coroutine_handle<>::from_address(address).resume();
            },
        });
    }
    void await_resume() { }
};

class Mutex {
private:
    struct Link;

public:
    class Unlock {
    private:
        Mutex* mutex = nullptr;
        void handoverToNext();

    public:
        Unlock() = default;
        Unlock(Mutex* mutex)
            : mutex(mutex) { }
        void deallocate();
        explicit operator bool() const { return (bool)mutex; }
    };

    class LockRequest {
    private:
        Mutex* m_mutex = nullptr;
        Mutex::Link* link = nullptr;
        Mutex::Link* prevOwner = nullptr;

    public:
        LockRequest(Mutex* mutex);
        Mutex* mutex() { return m_mutex; }
        stx::Owned<Unlock> lockIfReady();
        void wait(stx::BasicCallback<stx::Owned<Unlock>> callback);
    };

private:
    struct Link {
        stx::BasicCallback<stx::Owned<Unlock>> callback;
        std::atomic<Link*> next = nullptr;
    };
    Link* current = nullptr;
    std::atomic<Link*> last = nullptr;
};

struct MutexAwaitable {
    Mutex::LockRequest request;
    MutexAwaitable(Mutex* mutex)
        : request(mutex) { }
    bool await_ready() {
        auto lock = request.lockIfReady().take();
        return (bool)lock;
    }
    void await_suspend(stdx::coroutine_handle<> handle) {
        request.wait({
            handle.address(),
            [](void* address, stx::Owned<Mutex::Unlock> lock) {
                lock.take();
                stdx::coroutine_handle<>::from_address(address).resume();
            },
        });
    }
    stx::Owned<Mutex::Unlock> await_resume() {
        return { request.mutex() };
    }
};

}

inline sw::SemaphoreAwaitable operator co_await(sw::Semaphore& semaphore) {
    return { semaphore };
}

inline sw::MutexAwaitable operator co_await(sw::Mutex& mutex) {
    return { &mutex };
}