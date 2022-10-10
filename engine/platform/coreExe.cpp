#include "Window.hpp"

namespace plt {

evt::ExecutorImpl* coreExe() {
    static evt::ExecutorImpl exe = {};
    return &exe;
}

}