#pragma once

#include "containers/AtomicList.hpp"

#if __has_include(<coroutine>)

#include <coroutine>
namespace stdx = std;

#else

#include <experimental/coroutine>
namespace stdx = std::experimental;

#endif

struct ScheduleItem : AtomicListItem<ScheduleItem> {
    stdx::coroutine_handle<> handle;
};

class scheduling {
public:
    static inline AtomicList<ScheduleItem> schedulingQueue;
};
