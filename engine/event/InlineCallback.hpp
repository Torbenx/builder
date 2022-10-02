#pragma once

#include "Awaitable.hpp"
#include "Script.hpp"
#include "conditions.hpp"
#include "util/target.hpp"

namespace evt {

template<typename Target, typename... Args, int_t... I>
auto callWithTuple_helper(const Target& target, const stx::Tuple<Args...>& tuple, stx::int_sequence<I...>) {
    return target(stx::get<I>(tuple)...);
}
template<typename Target, typename... Args>
auto callWithTuple(const Target& target, const stx::Tuple<Args...>& tuple) {
    return callWithTuple_helper<Target, Args...>(target, tuple, stx::int_sequence_for<Args...>());
}

template<typename Target>
concept BasicCallable = requires(const Target& target, const typename Target::ArgsTuple& tuple) {
    { callWithTuple(target, tuple) } -> std::same_as<typename Target::ReturnType>;
};
template<typename Target, typename Source>
concept SourceAwareCallable = requires(const Target& target, Source* source) {
    { target.withSource(source) } -> BasicCallable;
};
template<typename Target, typename Source>
concept Callable = BasicCallable<Target> || SourceAwareCallable<Target, Source>;

template<typename Target>
Target withCallSource(const Target& target, void*) requires BasicCallable<Target> {
    return target;
}
template<typename Target, typename Source>
auto withCallSource(const Target& target, Source* source) requires SourceAwareCallable<Target, Source> {
    return target.withSource(source);
}

template<typename T, T>
struct CallDerived;

template<typename Derived, typename R, typename... Args, R (Derived::*funcPtr)(Args...)>
struct CallDerived<R (Derived::*)(Args...), funcPtr> {
    struct WithSource {
        using ReturnType = R;
        using ArgsTuple = stx::Tuple<Args...>;
        Derived* derived = nullptr;
        constexpr R operator()(Args... args) const {
            return (derived->*funcPtr)(args...);
        }
    };
    template<typename Base>
    constexpr WithSource withSource(Base* base) const requires std::derived_from<Derived, Base> {
        return { static_cast<Derived*>(base) };
    }
};
template<auto funcPtr>
constexpr CallDerived<decltype(funcPtr), funcPtr> callDerived = {};

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

    ScriptHandle inlineScript() const { return { stdx::coroutine_handle<ScriptPromise>::from_address((void*)storage) }; }

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
        static_assert(Callable<Target, InlineScript>);
        [](InlineScriptBase* base) -> CoroHandle {
            InlineScript* thiz = static_cast<InlineScript*>(base);
            while (true) {
                co_await thiz->condition;
                withCallSource(thiz->target, thiz)();
            }
        }(this);
    }
};

template<typename ConditionNode>
struct InlineScript<void, ConditionNode> : InlineScriptBase {
    using Condition = CompiledCondition<ConditionNode>;
    Condition condition;
    template<Callable<InlineScript> Target>
    InlineScript(const Target&, const Condition& condition)
        : condition(condition) {
        [](InlineScriptBase* base) -> CoroHandle {
            InlineScript* thiz = static_cast<InlineScript*>(base);
            while (true) {
                co_await thiz->condition;
                withCallSource(Target {}, thiz)();
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
        static_assert(Callable<Target, InlineScript>);
        [](InlineScriptBase* base) -> CoroHandle {
            InlineScript* thiz = static_cast<InlineScript*>(base);
            while (true) {
                co_await CompiledCondition<ConditionNode>();
                withCallSource(thiz->target, thiz)();
            }
        }(this);
    }
};

template<>
struct InlineScript<void, void> : InlineScriptBase {
    template<Callable<InlineScript> Target, typename ConditionNode = MonoCondition::RootNode>
    InlineScript(const Target&, const CompiledCondition<ConditionNode>& = monoCondition) {
        [](InlineScriptBase* base) -> CoroHandle {
            InlineScript* thiz = static_cast<InlineScript*>(base);
            while (true) {
                co_await CompiledCondition<ConditionNode>();
                withCallSource(Target {}, thiz)();
            }
        }(this);
    }
};

}