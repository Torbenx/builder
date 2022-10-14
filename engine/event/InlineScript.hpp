#pragma once

#include "Awaitable.hpp"
#include "Script.hpp"
#include "conditions.hpp"
#include "util/target.hpp"
#include "util/Callable.hpp"

namespace evt {

struct InlineScriptBase {
#if defined(COMPILER_INTERNALS_CLANG)
    static constexpr int_t INLINE_SCRIPT_STORAGE_SIZE = 48;
#elif defined(COMPILER_INTERNALS_GCC)
    static constexpr int_t INLINE_SCRIPT_STORAGE_SIZE = 64;
#else
// MSVC can currently not compile this
// hence its does not get a size
#error "InlineScript not supported"
#endif
    std::byte storage alignas(8)[INLINE_SCRIPT_STORAGE_SIZE];

    struct CoroHandle {
        struct promise_type : ScriptPromise {
            CoroHandle get_return_object() { return {}; }
            void* operator new(size_t size, InlineScriptBase* ptr) {
                EXPECT(Event, size, sizeof(ptr->storage));
                return ptr->storage;
            }
            void operator delete(void*) { }
            promise_type(InlineScriptBase*& thizPtr) {
                auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
                InlineScriptBase* newThizPtr = (InlineScriptBase*)handle.address();
                // clang's optimizer optimizes this condition to false even when it is true???
                // VERIFY(Event, thizPtr == newThizPtr);
                // so we have to make it more complicated
                VERIFY(Event, (((uintptr_t)newThizPtr - (uintptr_t)thizPtr) & 0xffff'ffff) == 0);
                thizPtr = newThizPtr;
            }
        };
    };

    Script inlineScript() const { return { stdx::coroutine_handle<ScriptPromise>::from_address((void*)storage) }; }

    ~InlineScriptBase() {
        inlineScript().handle.destroy();
    }
};

template<typename Target = void, typename ConditionNode = void>
struct InlineScript : InlineScriptBase {
    using Condition = CompiledCondition<ConditionNode>;
    Target target;
    Condition condition;
    InlineScript(const Target& target, const Condition& condition)
        : target(target), condition(condition) {
        static_assert(stx::Callable<Target, InlineScript>);
        [](InlineScriptBase* base) -> CoroHandle {
            InlineScript* thiz = static_cast<InlineScript*>(base);
            while (true) {
                await { thiz->condition };
                stx::callWithSource(thiz->target, *thiz);
            }
        }(this);
    }
};

template<typename ConditionNode>
struct InlineScript<void, ConditionNode> : InlineScriptBase {
    using Condition = CompiledCondition<ConditionNode>;
    Condition condition;
    template<stx::Callable<InlineScript> Target>
    InlineScript(const Target&, const Condition& condition)
        : condition(condition) {
        [](InlineScriptBase* base) -> CoroHandle {
            InlineScript* thiz = static_cast<InlineScript*>(base);
            while (true) {
                await { thiz->condition };
                stx::callWithSource(Target {}, *thiz);
            }
        }(this);
    }
};

template<typename Target>
struct InlineScript<Target, void> : InlineScriptBase {
    Target target;
    template<typename ConditionNode = MonoCondition::RootNode>
    InlineScript(const Target& target, const CompiledCondition<ConditionNode>& = monoCondition)
        : target(target) {
        static_assert(stx::Callable<Target, InlineScript>);
        [](InlineScriptBase* base) -> CoroHandle {
            InlineScript* thiz = static_cast<InlineScript*>(base);
            while (true) {
                await { CompiledCondition<ConditionNode>() };
                stx::callWithSource(thiz->target, *thiz);
            }
        }(this);
    }
};

template<>
struct InlineScript<void, void> : InlineScriptBase {
    template<stx::Callable<InlineScript> Target, typename ConditionNode = MonoCondition::RootNode>
    InlineScript(const Target&, const CompiledCondition<ConditionNode>& = monoCondition) {
        [](InlineScriptBase* base) -> CoroHandle {
            InlineScript* thiz = static_cast<InlineScript*>(base);
            while (true) {
                await { CompiledCondition<ConditionNode>() };
                stx::callWithSource(Target {}, *thiz);
            }
        }(this);
    }
};

}