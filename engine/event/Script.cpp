#include "conditions.hpp"
#include "platform/Window.hpp"

namespace evt {

void Signal<>::send(int_t count) const {
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

ScriptPromise::ScriptPromise()
    : executor(nullptr), completion(new Source<ScriptCompletionEvent>) { }

ScriptPromise::~ScriptPromise() {
    completion->sendEvent({});
    delete completion;
}

void Ping::commit(Signal<> signal, ScratchSpace& scratch) const {
    scratch.signal = signal;
    scratch.sourceHandler = &sourceHandler;
    targetHandler.sendEvent({
        &scratch,
        [](ScratchSpace* scratch) {
            postCallback(
                *scratch->sourceHandler,
                {
                    scratch,
                    [](ScratchSpace* scratch) {
                        Signal<> signal = scratch->signal;
                        scratch->signal = {};
                        signal.send();
                    },
                });
            scratch->sourceHandler = nullptr;
        },
    });
}

void Ping::cancel(Script, ScratchSpace& scratch) const {
    VERIFY(Event, scratch.signal.script == Script {});
}

}