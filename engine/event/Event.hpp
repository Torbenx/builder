#pragma once

#include "CompiledCondition.hpp"

namespace evt {

template<typename E>
struct EventReceiver {
    std::vector<E> events;
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
    struct NodeType : TmplLeafNode<NodeType> {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return UNBOUND_EVENT_COUNT; }

        EventSource* source = nullptr;
        EventReceiver<E>* receiver = nullptr;

        void commit(Signal signal) const {
            source->signals.push_back({ signal, receiver });
        }
        void cancel(ScriptHandle script) const {
            auto it = std::remove(source->signals.begin(), source->signals.end(), script);
            source->signals.erase(it, source->signals.end());
        }
    };
    std::vector<EventSignal<E>> signals;
    constexpr NodeType operator>(EventReceiver<E>& rec) {
        return { {}, this, &rec };
    }
};

}