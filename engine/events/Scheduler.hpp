#pragma once

#include "containers/AtomicList.hpp"

#if __has_include(<coroutine>)

#include <coroutine>
namespace stdx = std;

#else

#include <experimental/coroutine>
namespace stdx = std::experimental;

#endif

namespace Tag {
struct Events {
    static constexpr char tag[] = "events";
    static constexpr bool verifyEnabled = true;
};
}

#define await co_await

struct ScheduleItem : AtomicListItem<ScheduleItem> {
    stdx::coroutine_handle<> handle;
};

class scheduling {
public:
    static inline AtomicList<ScheduleItem> schedulingQueue;
};
