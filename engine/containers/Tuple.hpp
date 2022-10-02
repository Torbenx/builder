#pragma once

#include "util/types.hpp"

namespace stx {

namespace detail {
    struct tuple_storage_untyped { };

    template<typename T>
    struct tuple_storage_base /*: tuple_storage_untyped*/ {
        T t;
    };

    template<typename T, int_t i>
    struct tuple_storage_indexed : tuple_storage_base<T> {
        //tuple_storage_untyped* untyped_ptr() { return this; }
    };
    template<typename T, int_t i>
    requires std::is_empty_v<T>
    struct tuple_storage_indexed<T, i> {
    };

    template<typename... Storage>
    struct TupleImpl : Storage... { };
    template<typename S, typename... Types>
    struct TupleIndirect;
    template<int_t... I, typename... Types>
    struct TupleIndirect<stx::int_sequence<I...>, Types...> {
        using type = TupleImpl<tuple_storage_indexed<Types, I>...>;
    };
}

template<typename... Types>
using Tuple = typename detail::TupleIndirect<stx::int_sequence_for<Types...>, Types...>::type;

template<int_t i, typename T>
auto& get(detail::tuple_storage_indexed<T, i>& s) {
    return s.t;
}
template<typename T>
auto& get(detail::tuple_storage_base<T>& s) {
    return s.t;
}

template<typename... Types, int_t... I, typename Callback>
void forEachElement(detail::TupleImpl<detail::tuple_storage_indexed<Types, I>...>& tuple, Callback&& callback) {
    (callback(get<I>(tuple)), ...);
}

}
