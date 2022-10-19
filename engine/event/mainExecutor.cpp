#include "Scheduler.hpp"

namespace evt {

ExecutorImpl* mainExecutor() {
    static ExecutorImpl exe = {};
    return &exe;
}

}