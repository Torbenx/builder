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

template<typename Key, typename Style>
concept ContainedIn = requires(const Style* ptr) {
    static_cast<const SmallStyleTableStorage<Key>*>(ptr);
};

template<typename... Keys>
struct SmallStyleTableAccumulator {
    using type = SmallStyleTableImpl<Keys...>;
};
template<typename... Keys1, typename... Keys2>
struct SmallStyleTableAccumulator<SmallStyleTableImpl<Keys1...>, Keys2...> {
    using type = typename SmallStyleTableAccumulator<Keys2..., Keys1...>::type;
};
template<typename Key>
struct MaybeWrapWithTable {
    using type = SmallStyleTableImpl<Key>;
};
template<typename... Keys>
struct MaybeWrapWithTable<SmallStyleTableImpl<Keys...>> {
    using type = SmallStyleTableImpl<Keys...>;
};
template<typename... Keys>
using SmallStyleTable = typename SmallStyleTableAccumulator<typename MaybeWrapWithTable<Keys>::type...>::type;

template<typename Callback, typename... Keys>
void forEachKey(SmallStyleTableImpl<Keys...>& style, Callback&& callback) {
    (callback.template operator()<Keys>(get<Keys>(style)), ...);
}

namespace detail {
    template<typename... Ts>
    struct Pack;

    template<typename P1, typename P2>
    struct beginsWith;
    template<typename T1, typename... Ts1, typename T2, typename... Ts2>
    struct beginsWith<Pack<T1, Ts1...>, Pack<T2, Ts2...>> {
        using type = std::conditional_t<
            std::is_same_v<T1, T2>,
            typename beginsWith<Pack<Ts1...>, Pack<Ts2...>>::type,
            std::false_type>;
    };
    template<typename... Ts2>
    struct beginsWith<Pack<>, Pack<Ts2...>> {
        using type = std::true_type;
    };
    template<typename... Ts1>
    struct beginsWith<Pack<Ts1...>, Pack<>> {
        using type = std::false_type;
    };
    template<>
    struct beginsWith<Pack<>, Pack<>> {
        using type = std::true_type;
    };

    template<typename P1, typename P2>
    struct subpackOf;
    template<typename T1, typename... Ts1, typename T2, typename... Ts2>
    struct subpackOf<Pack<T1, Ts1...>, Pack<T2, Ts2...>> {
        using type = std::conditional_t<
            std::is_same_v<T1, T2>,
            typename beginsWith<Pack<Ts1...>, Pack<Ts2...>>::type,
            typename subpackOf<Pack<T1, Ts1...>, Pack<Ts2...>>::type>;
    };
    template<typename... Ts1>
    struct subpackOf<Pack<Ts1...>, Pack<>> {
        using type = std::false_type;
    };
    template<typename... Ts2>
    struct subpackOf<Pack<>, Pack<Ts2...>> {
        using type = std::true_type;
    };
    template<>
    struct subpackOf<Pack<>, Pack<>> {
        using type = std::true_type;
    };

    template<typename T, typename... Ts>
    struct first {
        using type = T;
    };
}
template<typename... Keys>
struct Ptr<SmallStyleTableImpl<Keys...>> : PtrBase {
    using PtrBase::PtrBase;

    template<typename... Keys2>
    static int_t computeByteOffset() {
        using FirstKey = typename detail::first<Keys...>::type;
        const auto* inObject = reinterpret_cast<SmallStyleTableImpl<Keys2...>*>(1); // FIXME: Why 1?
        const auto* outObject = static_cast<const SmallStyleTableStorage<FirstKey>*>(inObject);
        return (const std::byte*)outObject - (const std::byte*)inObject;
    }

    template<typename... Keys2>
    Ptr(const Ptr<SmallStyleTableImpl<Keys2...>>& in) requires(detail::subpackOf<detail::Pack<Keys...>, detail::Pack<Keys2...>>::type::value)
        : PtrBase(in.info().offsetBy(Ptr::computeByteOffset<Keys2...>())) { }

    template<typename Key>
    const Ptr<typename Key::Type> get() const {
        const auto* inObject = reinterpret_cast<SmallStyleTableImpl<Keys...>*>(1); // FIXME: Why 1?
        const auto* outObject = static_cast<const SmallStyleTableStorage<Key>*>(inObject);
        return Ptr<typename Key::Type> { byteOffsetBy((const std::byte*)outObject - (const std::byte*)inObject) };
    }
};

template<typename Key>
struct SmallStyleTableRefStorage {
    Ref<typename Key::Type> value;
    SmallStyleTableRefStorage() = default;
    SmallStyleTableRefStorage(Ptr<typename Key::Type> ptr)
        : value(ptr) { }
};
template<typename... Keys>
struct Ref<SmallStyleTableImpl<Keys...>> : SmallStyleTableRefStorage<Keys>... {
    Ref() = default;
    Ref(Ptr<SmallStyleTableImpl<Keys...>> ptr)
        : SmallStyleTableRefStorage<Keys>(ptr.template get<Keys>())... { }

    template<typename Key>
    Ref<typename Key::Type>& get() { return SmallStyleTableRefStorage<Key>::value; }
    template<typename Key>
    const Ref<typename Key::Type>& get() const { return SmallStyleTableRefStorage<Key>::value; }

    void clear() {
        (get<Keys>().clear(), ...);
    }
    SmallStyleTableImpl<Keys...> resolve(const Resolver& resolver) const {
        return { { get<Keys>().resolve(resolver) }... };
    }
    SmallStyleTableImpl<Keys...> substitute(const Substituter& substituter, const SmallStyleTableImpl<Keys...>& base) const {
        return { { get<Keys>().substitute(substituter, base.template get<Keys>()) }... };
    }
};

}