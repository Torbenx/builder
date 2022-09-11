#pragma once

#include "Ref.hpp"

namespace ui {

template<typename Key>
struct SmallStyleStorage {
    typename Key::Type value;
};
namespace detail {
    template<typename Key>
    typename Key::Type& SmallStyle_get(SmallStyleStorage<Key>& storage) {
        return storage.value;
    }
    template<typename Key>
    const typename Key::Type& SmallStyle_get(const SmallStyleStorage<Key>& storage) {
        return storage.value;
    }
}
template<typename... Keys>
struct SmallStyleImpl : SmallStyleStorage<Keys>... {
    template<typename Key>
    typename Key::Type& get() { return detail::SmallStyle_get<Key>(*this); }
    template<typename Key>
    const typename Key::Type& get() const { return detail::SmallStyle_get<Key>(*this); }
};

template<typename... Keys>
struct SmallStyleAccumulator {
    using type = SmallStyleImpl<Keys...>;
};
template<typename... Keys1, typename... Keys2>
struct SmallStyleAccumulator<SmallStyleImpl<Keys1...>, Keys2...> {
    using type = typename SmallStyleAccumulator<Keys2..., Keys1...>::type;
};
template<typename... Keys>
using SmallStyle = typename SmallStyleAccumulator<Keys...>::type;

template<typename Callback, typename... Keys>
void forEachKey(SmallStyleImpl<Keys...>& style, Callback&& callback) {
    (callback.template operator()<Keys>(get<Keys>(style)), ...);
}

template<typename Key>
struct SmallStyleRefStorage {
    Ref<typename Key::Type> value;
    SmallStyleRefStorage() = default;
    SmallStyleRefStorage(const MakeSelfReference& selfRef)
        : value(selfRef) { }
};
namespace detail {
    template<typename Key>
    Ref<typename Key::Type>& SmallStyleRef_get(SmallStyleRefStorage<Key>& storage) {
        return storage.value;
    }
    template<typename Key>
    const Ref<typename Key::Type>& SmallStyleRef_get(const SmallStyleRefStorage<Key>& storage) {
        return storage.value;
    }
}
template<typename... Keys>
struct Ref<SmallStyleImpl<Keys...>> : SmallStyleRefStorage<Keys>... {
    Ref() = default;
    Ref(const MakeSelfReference& selfRef)
        : SmallStyleRefStorage<Keys>(selfRef)... { }

    template<typename Key>
    Ref<typename Key::Type>& get() { return detail::SmallStyleRef_get<Key>(*this); }
    template<typename Key>
    const Ref<typename Key::Type>& get() const { return detail::SmallStyleRef_get<Key>(*this); }

    SmallStyleImpl<Keys...> resolve(const Resolver& resolver) const {
        return { { get<Keys>(*this).resolve(resolver) }... };
    }
};
}