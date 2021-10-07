#pragma once

#include "util/log.hpp"
#include <tuple>

#ifdef interface
// apparently mingw's rcp.h defines this
#undef interface
#endif

namespace Tag {
struct Task {
    static constexpr char tag[] = "task";
    static constexpr bool verifyEnabled = true;
};
}

namespace task {

template<typename T>
struct interface_t { };
template<typename T>
constexpr interface_t<T> interface = {};

template<typename T>
struct is_interface {
    static constexpr bool value = false;
};
template<typename T>
struct is_interface<interface_t<T>> {
    static constexpr bool value = true;
};

template<typename T, typename... Types>
struct alias_t { };
template<typename T, typename... Types>
constexpr alias_t<T, Types...> alias;

template<typename T>
struct data_t { };
template<typename T>
constexpr data_t<T> data = {};
template<typename T>
struct is_data {
    static constexpr bool value = false;
};
template<typename T>
struct is_data<data_t<T>> {
    static constexpr bool value = true;
};

template<typename T>
struct InterfaceTuple {
    using type = std::tuple<>;
};
template<typename... Types>
struct InterfaceTuple<std::tuple<interface_t<Types>...>> {
    using type = std::tuple<Types...>;
};

DEFINE_LINK_COUNTER(task_interface)
template<typename T>
struct GlobalInterface {
    LINK_COUNTER_IMPL(task_interface)
};

template<typename T>
struct DataTuple {
    using type = std::tuple<>;
};
template<typename... Types>
struct DataTuple<std::tuple<data_t<Types>...>> {
    using type = std::tuple<Types...>;
};

template<typename... Types>
struct Description {
    static constexpr auto interfaces = std::tuple_cat(std::conditional_t<is_interface<Types>::value, std::tuple<Types>, std::tuple<>> {}...);
    using InterfaceTuple_t = typename InterfaceTuple<decltype(interfaces)>::type;

    static constexpr auto data = std::tuple_cat(std::conditional_t<is_data<Types>::value, std::tuple<Types>, std::tuple<>> {}...);
    using DataTuple_t = typename DataTuple<decltype(data)>::type;

    constexpr Description(Types...) { }

    struct Instance {
        InterfaceTuple_t interfaces;
        DataTuple_t data;
    };
};

}
