#pragma once

#include "Script.hpp"
#include "util/target.hpp"

namespace evt {

struct Anything {
    bool await_ready() { return false; }
    void await_suspend(ScriptHandle handle) {
        handle->key = -1;
        handle->state = 0;
    }
    void await_resume() { }
};

struct InlineCallback {
#if defined(COMPILER_INTERNALS_CLANG)
    static constexpr int_t INLINE_CALLBACK_STORAGE_SIZE = 48;
#elif defined(COMPILER_INTERNALS_GCC)
    static constexpr int_t INLINE_CALLBACK_STORAGE_SIZE = 64;
#else
// MSVC can currently not compile this
// hence its does not get a size
#error "InlineCallback not supported"
#endif
    std::byte storage alignas(8)[INLINE_CALLBACK_STORAGE_SIZE];

    template<typename Base, void (Base::*callback)()>
    InlineCallback(std::integral_constant<void (Base::*)(), callback>) {
        struct CoroHandle {
            struct promise_type : ScriptPromise {
                CoroHandle get_return_object() { return {}; }
                void* operator new(size_t size, InlineCallback* ptr) {
                    EXPECT(Event, size, sizeof(ptr->storage));
                    return ptr->storage;
                }
                void operator delete(void*) { }
                promise_type(InlineCallback*& thizPtr) {
                    auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
                    InlineCallback* newThizPtr = (InlineCallback*)handle.address();
                    // clang's optimizer optimizes this condition to false even when it is true???
                    // VERIFY(Event, thizPtr == newThizPtr);
                    // so we have to make it more complicated
                    VERIFY(Event, (((uintptr_t)newThizPtr - (uintptr_t)thizPtr) & 0xffff'ffff) == 0);
                    thizPtr = newThizPtr;
                }
            };

            static CoroHandle coro(InlineCallback* thiz) {
                while (true) {
                    co_await Anything();
                    (static_cast<Base*>(thiz)->*callback)();
                }
            }
        };
        CoroHandle::coro(this);
    }

    ~InlineCallback() {
        inlineScript().handle.destroy();
    }

    ScriptHandle inlineScript() const { return { stdx::coroutine_handle<ScriptPromise>::from_address((void*)storage) }; }
    Signal inlineCallback() const { return { inlineScript(), 1 }; }
};

}