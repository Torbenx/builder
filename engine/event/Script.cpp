#include "Awaitable.hpp"
#include "conditions.hpp"
#include "platform/Window.hpp"

namespace evt {

void Signal<>::send(int_t count) const {
    ExecutorImpl* exeImpl = currentTimeline()->executor();
    VERIFY(Event, exeImpl);
    Executor* exe = script->executeOnNextTick ? &exeImpl->nextTick : exeImpl;

    uint64_t oldState = script->state.state;
    uint64_t newState = oldState + mask * count;
    script->state.state = newState;
    uint64_t key = script->state.key;
    bool wasReady = (oldState & key) != 0;
    bool isReady = (newState & key) != 0;
    if (isReady && !wasReady)
        exe->schedule(script);
    else if (wasReady && !isReady)
        exe->unschedule(script);
}

ScriptPromise::~ScriptPromise() {
    completion.sendEvent({});
}

void Ping::commit(Signal<> signal, ScratchSpace& scratch) const {
    scratch.signal = signal;
    scratch.sourceHandler = &currentTimeline()->callbackHandler();
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

Script detail::asyncInvoke(AsyncInvokeData* data) {
    Source<CallbackEvent>& sourceHandler = data->sourceHandler;
    Script sourceScript = data->sourceScript;
    Script targetScript = data->func(data);
    await { targetScript->completion };
    postCallback(
        sourceHandler,
        {
            sourceScript.address(),
            [](void* address) {
                Script::fromAddress(address).handle.destroy();
            },
        });
}

}