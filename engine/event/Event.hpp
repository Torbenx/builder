#pragma once

#include "Script.hpp"

namespace evt {

template<typename E>
struct EventReceiver {
    std::vector<E> events;
};

template<typename E>
struct EventDispatcher : EventReceiver<E> {

};

template<typename E>
struct EventSignal {
    Signal signal;
    EventReceiver<E>* receiver = nullptr;
    void send(const E& e) const {
        receiver->events.emplace_back(e);
        signal.send();
    }
};

template<typename E>
struct EventSource {
    std::vector<EventSignal<E>> signals;
};

}