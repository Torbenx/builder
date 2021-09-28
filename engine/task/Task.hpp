#pragma once

#include "containers/MemoryAllocator32.hpp"
#include "util/coroutines.hpp"
#include "util/log.hpp"
#include "util/uninitialized.hpp"
#include <type_traits>

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

template<typename... Types>
struct description {
    static constexpr auto interfaces = std::tuple_cat(std::conditional_t<is_interface<Types>::value, std::tuple<Types>, std::tuple<>> {}...);
    static constexpr auto data = std::tuple_cat(std::conditional_t<is_data<Types>::value, std::tuple<Types>, std::tuple<>> {}...);

    constexpr description(Types...) { }
};

namespace detail {
    inline uninitialized<MemoryAllocator32> g_allocator;
    inline MemoryAllocator32& allocator() { return g_allocator.value; }
}

template<description desc = description {}>
class Promise;
template<description desc = description {}>
class PromiseBase;

template<description desc = description {}>
class Task {
public:
    using promise_type = Promise<desc>;

    ~Task() {
        VERIFY(Task, ptr == nullptr);
    }

private:
    Promise<desc>* ptr;

    Promise<desc>* take() {
        Promise<desc>* r = ptr;
        ptr = nullptr;
        return r;
    }

    template<description d>
    friend class Promise;
    template<description d>
    friend class PromiseBase;
};
}
