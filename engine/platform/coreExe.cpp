#include "Window.hpp"

namespace plt {

evt::ExecutorImpl* coreExe() {
    static evt::ExecutorImpl exe = {};
    return &exe;
}

evt::Source<evt::CallbackEvent>& callbackHandler() {
    static evt::Source<evt::CallbackEvent> source = {};
    return source;
}

}