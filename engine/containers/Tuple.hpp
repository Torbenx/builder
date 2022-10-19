#pragma once

#include "util/Callable.hpp"
#include "util/types.hpp"

namespace stx {

namespace detail {
    struct tuple_storage_untyped { };

    template<typename T, int_t i>
    struct tuple_storage_indexed {
        T t;
    };
    template<typename T, int_t i>
    requires std::is_empty_v<T>
    struct tuple_storage_indexed<T, i> { };

    template<typename... Storage>
    struct TupleImpl : Storage... { };

    template<typename S, typename... Types>
    struct TupleIndirect;
    template<int_t... I, typename... Types>
    struct TupleIndirect<stx::int_sequence<I...>, Types...> {
        using type = TupleImpl<tuple_storage_indexed<Types, I>...>;
    };
    template<typename... Types>
    using TupleBase = typename detail::TupleIndirect<stx::int_sequence_for<Types...>, Types...>::type;
}

template<typename... Types>
struct Tuple : detail::TupleBase<Types...> { };

template<int_t i, typename T>
constexpr T& get(detail::tuple_storage_indexed<T, i>& s) {
    return s.t;
}
template<int_t i, typename T>
constexpr const T& get(const detail::tuple_storage_indexed<T, i>& s) {
    return s.t;
}
template<typename T, int_t i>
constexpr T& get(detail::tuple_storage_indexed<T, i>& s) {
    return s.t;
}
template<typename T, int_t i>
constexpr const T& get(const detail::tuple_storage_indexed<T, i>& s) {
    return s.t;
}

template<typename... Types, int_t... I, typename Callback>
constexpr void forEachElement(detail::TupleImpl<detail::tuple_storage_indexed<Types, I>...>& tuple, Callback&& callback) {
    (callback(get<I>(tuple)), ...);
}
template<typename... Types, int_t... I, typename Callback>
constexpr void forEachElementIndexed(detail::TupleImpl<detail::tuple_storage_indexed<Types, I>...>& tuple, Callback&& callback) {
    (callback(get<I>(tuple), stx::constant<I>()), ...);
}

}
