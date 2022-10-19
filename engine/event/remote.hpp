#pragma once

#include "platform/Window.hpp"

namespace evt {

namespace detail {
    template<typename... Args>
    using RemoteInvTpl = stx::Tuple<Source<CallbackEvent>&, Script, Script (*)(Args...), Args...>;
    template<typename... Args, int_t... I>
    Script remoteRunInvoke(RemoteInvTpl<Args...>* tuple, stx::int_sequence<I...>) {
        Source<CallbackEvent>& sourceHandler = stx::get<0>(*tuple);
        Script sourceScript = stx::get<1>(*tuple);
        Script targetScript = stx::get<2>(*tuple)(std::forward<Args>(stx::get<I + 3>(*tuple))...);
        delete tuple;
        await { *targetScript->completion };
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
template<typename... Args>
Script remoteRun(Script (*func)(Args...), Source<CallbackEvent>& sourceHandler, Source<CallbackEvent>& targetHandler, Args... args) {
    co_await SuspendAnd([&](Script sourceScript) {
        auto* tuple = new detail::RemoteInvTpl<Args...> { {
            { sourceHandler }, { sourceScript }, { func }, { std::forward<Args>(args) }...
        } };
        targetHandler.sendEvent({
            tuple,
            [](detail::RemoteInvTpl<Args...>* tuple) {
                detail::remoteRunInvoke(tuple, stx::int_sequence_for<Args...>());
            },
        });
    });
}

}