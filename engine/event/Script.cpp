#include "ScriptPromise.hpp"

namespace evt {

void Signal::send(int_t count) const {
    Executor* localExecutor = currentExecutor();
    uint64_t oldState = script->state.state;
    uint64_t newState = oldState + mask * count;
    script->state.state = newState;
    uint64_t key = script->state.key;
    bool wasReady = (oldState & key) != 0;
    bool isReady = (newState & key) != 0;
    Executor* exe = script->executor;
    exe = exe ? exe : localExecutor;
    VERIFY(Event, exe);
    if (isReady && !wasReady)
        exe->schedule(script);
    else if (wasReady && !isReady)
        exe->unschedule(script);
}

}