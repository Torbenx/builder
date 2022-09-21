#pragma once

#include "Ref.hpp"

namespace ui {

template<typename Key>
struct SmallStyleTableStorage {
    wrapper<typename Key::Type> wrapper;
};
template<typename... Keys>
struct SmallStyleTableImpl : SmallStyleTableStorage<Keys>... {
    template<typename Key>
    typename Key::Type& get() { return SmallStyleTableStorage<Key>::wrapper.value; }
    template<typename Key>
    const typename Key::Type& get() const { return SmallStyleTableStorage<Key>::wrapper.value; }
};

template<typename... Keys>
struct SmallStyleTableAccumulator {
    using type = SmallStyleTableImpl<Keys...>;
};
template<typename... Keys1, typename... Keys2>
struct SmallStyleTableAccumulator<SmallStyleTableImpl<Keys1...>, Keys2...> {
    using type = typename SmallStyleTableAccumulator<Keys2..., Keys1...>::type;
};
template<typename... Keys>
using SmallStyleTable = typename SmallStyleTableAccumulator<Keys...>::type;

template<typename Callback, typename... Keys>
void forEachKey(SmallStyleTableImpl<Keys...>& style, Callback&& callback) {
    (callback.template operator()<Keys>(get<Keys>(style)), ...);
}

template<typename Key>
struct SmallStyleTableRefStorage {
    Ref<typename Key::Type> value;
    SmallStyleTableRefStorage() = default;
    SmallStyleTableRefStorage(const MakeSelfReference& selfRef)
        : value(selfRef) { }
};
template<typename... Keys>
struct Ref<SmallStyleTableImpl<Keys...>> : SmallStyleTableRefStorage<Keys>... {
    Ref() = default;
    Ref(const MakeSelfReference& selfRef)
        : SmallStyleTableRefStorage<Keys>(selfRef)... { }

    template<typename Key>
    Ref<typename Key::Type>& get() { return SmallStyleTableRefStorage<Key>::value; }
    template<typename Key>
    const Ref<typename Key::Type>& get() const { return SmallStyleTableRefStorage<Key>::value; }

    void clear() {
        (get<Keys>().clear(), ...);
    }
    SmallStyleTableImpl<Keys...> resolve(const Resolver& resolver, const SmallStyleTableImpl<Keys...>& base) const {
        return { { get<Keys>().resolve(resolver, base.template get<Keys>()) }... };
    }
    SmallStyleTableImpl<Keys...> substitute(const Substituter& substituter, const SmallStyleTableImpl<Keys...>& base) const {
        return { { get<Keys>().substitute(substituter, base.template get<Keys>()) }... };
    }
};
}