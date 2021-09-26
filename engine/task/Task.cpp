#include "task/Task.hpp"

void init_script() {
    task::detail::g_allocator.initialize();
}