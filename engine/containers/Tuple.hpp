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

    template<typename S, typename... Types>
    struct TupleImpl;
    template<int_t... I, typename... Types>
    struct TupleImpl<stx::int_sequence<I...>, Types...> : tuple_storage_indexed<Types, I>... { };
}

template<typename... Types>
using Tuple = detail::TupleImpl<stx::int_sequence_for<Types...>, Types...>;

template<int_t i, typename T>
auto& get(detail::tuple_storage_indexed<T, i>& s) {
    return s.t;
}
template<typename T>
auto& get(detail::tuple_storage_base<T>& s) {
    return s.t;
}

}
