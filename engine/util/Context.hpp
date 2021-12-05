#pragma once

#include "util/log.hpp"
#include <type_traits>

namespace Tag {
struct Context {
    static constexpr char tag[] = "context";
    static constexpr bool verifyEnabled = true;
};
}

namespace context_detail {
namespace {
    template<typename C>
    C current = {};
}

template<typename C>
struct RestoreHelper {
    const C ctx;
    RestoreHelper()
        : ctx(context_detail::current<C>) { }
    ~RestoreHelper() {
        context_detail::current<C> = ctx;
    }
};
}

template<typename C>
class Context {
public:
    Context(context_detail::RestoreHelper<C> = {})
        : ctx(context_detail::current<C>) {
        VERIFY(Context, isInitialized(ctx));
    }

    const C& operator*() requires(!std::is_pointer_v<C>) { return ctx; }
    const C* operator->() requires(!std::is_pointer_v<C>) { return &ctx; }

    std::remove_pointer_t<C>& operator*() requires std::is_pointer_v<C> { return *ctx; }
    C operator->() requires std::is_pointer_v<C> { return ctx; }

    void forward() { set(ctx); }

    static void set(const C& ctx) {
        context_detail::current<C> = ctx;
    }
    static void clear() {
        context_detail::current<C> = {};
    }

private:
    const C ctx;

    static bool isInitialized(C ctx) requires std::is_pointer_v<C> {
        return ctx != nullptr;
    }
};
