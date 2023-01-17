#pragma once

#include "containers/Tuple.hpp"

namespace stx {

template<typename Target, typename... Args>
concept BasicCallable = requires(const Target& target, Args... args) {
    target(std::forward<Args>(args)...);
};
template<typename Target, typename Source, typename... Args>
concept Callable = BasicCallable<Target, Args...> || BasicCallable<Target, Source&, Args...>;

template<typename... Args, typename Target, typename Source>
constexpr void callWithSource(const Target& target, Source&, Args... args) requires BasicCallable<Target, Args...> {
    return target(std::forward<Args>(args)...);
}
template<typename... Args, typename Target, typename Source>
constexpr void callWithSource(const Target& target, Source& source, Args... args) requires BasicCallable<Target, Source&, Args...> {
    return target(source, std::forward<Args>(args)...);
}

template<typename T, T t>
struct CallImpl;
template<typename Ret, typename... Args, Ret (*func)(Args...)>
struct CallImpl<Ret (*)(Args...), func> {
    Ret operator()(Args... args) const {
        return func(std::forward<Args>(args)...);
    }
};
template<auto func>
using Call = CallImpl<decltype(func), func>;

template<typename Derived, typename Callback>
struct WithDerived : Callback {
    WithDerived() = default;
    constexpr WithDerived(const Callback& callback)
        : Callback(callback) { }
    template<typename Base, typename... Args>
    constexpr decltype(auto) operator()(Base& base, Args&&... args) const requires std::derived_from<Derived, Base> {
        return Callback::operator()(static_cast<Derived&>(base), std::forward<Args>(args)...);
    }
};
template<typename Derived, typename Callback>
constexpr WithDerived<Derived, Callback> withDerived(const Callback& callback) { return { callback }; }

template<auto memberPtr, typename Callback>
struct WithContaining : Callback {
    using FieldType = typename stx::member_pointer<memberPtr>::FieldType;
    using ContType = typename stx::member_pointer<memberPtr>::BaseType;
    WithContaining() = default;
    WithContaining(const Callback& callback)
        : Callback(callback) { }
    template<typename... Args>
    constexpr decltype(auto) operator()(FieldType& field, Args&&... args) const {
        ContType* contP = reinterpret_cast<ContType*>(1); // FIXME: Why 1?
        FieldType* fieldP = &(contP->*memberPtr);
        int_t offset = (std::byte*)fieldP - (std::byte*)contP;
        ContType* cont = (ContType*)((std::byte*)&field - offset);
        return Callback::operator()(*cont, std::forward<Args>(args)...);
    }
};
template<auto memberPtr, typename Callback>
constexpr WithContaining<memberPtr, Callback> withContaining(const Callback& callback) { return { callback }; }

template<typename T, T t>
struct CallMemberImpl;
template<typename Class, typename Ret, typename... Args, Ret (Class::*funcPtr)(Args...)>
struct CallMemberImpl<Ret (Class::*)(Args...), funcPtr> {
    constexpr Ret operator()(Class& thiz, Args... args) const {
        return (thiz.*funcPtr)(std::forward<Args>(args)...);
    }
};
template<auto funcPtr>
using CallMember = CallMemberImpl<decltype(funcPtr), funcPtr>;
template<auto funcPtr>
using CallDerivedMember = WithDerived<typename stx::member_pointer<funcPtr>::BaseType, CallMember<funcPtr>>;

template<typename... Args>
struct BasicCallback {
    void (*callback)(void*, Args...) = nullptr;
    void* data = nullptr;

    BasicCallback() = default;
    template<typename Data, BasicCallable<Data*, Args...> Callback>
    BasicCallback(Data* data, const Callback& callback) {
        setCallback(data, callback);
    }
    template<typename Data, BasicCallable<Data&, Args...> Callback>
    BasicCallback(Data& data, const Callback& callback) {
        setCallback(data, callback);
    }
    template<BasicCallable<Args...> Callback>
    BasicCallback(const Callback& callback) {
        setCallback(callback);
    }
    explicit operator bool() const { return (bool)callback; }
    void operator()(Args... args) const {
        callback(data, std::forward<Args>(args)...);
    }
    template<typename Data, BasicCallable<Data*, Args...> Callback>
    void setCallback(Data* data, const Callback&) {
        this->data = data;
        this->callback = [](void* data, Args... args) {
            Callback {}((Data*)data, std::forward<Args>(args)...);
        };
    }
    template<typename Data, BasicCallable<Data&, Args...> Callback>
    void setCallback(Data& data, const Callback&) {
        this->data = &data;
        this->callback = [](void* data, Args... args) {
            Callback {}(*(Data*)data, std::forward<Args...>(args)...);
        };
    }
    template<BasicCallable<Args...> Callback>
    void setCallback(const Callback&) {
        this->data = nullptr;
        this->callback = [](void*, Args... args) {
            Callback {}(std::forward<Args>(args)...);
        };
    }
};

}