#pragma once

#include "Script.hpp"
#include "containers/Owned.hpp"
#include "containers/Tuple.hpp"
#include <optional>

namespace evt {

template<typename E>
struct receiver_traits;
template<typename E>
using Receiver = typename receiver_traits<E>::type;
template<typename E>
struct source_traits;
template<typename E>
using Source = typename source_traits<E>::type;

template<typename E>
struct ListReceiver {
    std::vector<E> events;
    void receiveEvent(const E& e) {
        events.push_back(e);
    }
    void receiveEvents(const ListReceiver<E>& other) {
        events.insert(events.end(), other.events.begin(), other.events.end());
    }
    bool hasEvent() const {
        return events.size() > 0;
    }
    int_t count() const {
        return events.size();
    }
    void reset() {
        events.clear();
    }
    template<stx::BasicCallable<E> Callback>
    void dispatchTo(const Callback& callback) {
        for (const auto& event : events)
            callback(event);
    }
};
template<typename E>
struct KeepLastReceiver {
    std::optional<E> event;
    KeepLastReceiver() = default;
    KeepLastReceiver(const KeepLastReceiver&) = default;
    KeepLastReceiver(KeepLastReceiver&& other)
        : event(std::move(other.event)) { other.event.reset(); }
    KeepLastReceiver& operator=(const KeepLastReceiver&) = default;
    KeepLastReceiver& operator=(KeepLastReceiver&& other) {
        event = std::move(other.event);
        other.event.reset();
        return *this;
    }
    void receiveEvent(const E& e) {
        event = e;
    }
    void receiveEvents(const KeepLastReceiver<E>& other) {
        VERIFY(Event, other.event.has_value());
        event = other.event.value();
    }
    bool hasEvent() const { return event.has_value(); }
    int_t count() const { return event.has_value(); }
    void reset() { event.reset(); }
    template<stx::BasicCallable<E> Callback>
    void dispatchTo(const Callback& callback) {
        if (hasEvent())
            callback(const_cast<const E&>(event.value()));
    }
};

template<typename E>
struct Signal {
    Signal<> signal;
    Receiver<E>* receiver = nullptr;
    void send(const E& e) const {
        receiver->receiveEvent(e);
        signal.send();
    }
    void send(const Receiver<E>& events) const {
        receiver->receiveEvents(events);
        signal.send(events.count());
    }
    bool operator==(const Script& script) const { return signal == script; }
};

// used by SciptCompletionEvent to avoid use after free
template<typename E>
concept SkipCleanupInCancel = E::NO_CLEANUP_IN_CANCEL;

template<typename E>
struct SourceImpl {
    using event_type = E;
    struct IgnoreContentNode : TmplLeafNode {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return UNBOUND_EVENT_COUNT; }

        struct ScratchSpace {
            Receiver<E> receiver;
        };

        const SourceImpl& source;

        void commit(Signal<> signal, ScratchSpace& scratch) const {
            source.signals.push_back({ signal, &scratch.receiver });
        }
        void cancel(Script script, ScratchSpace&) const {
            if constexpr (!SkipCleanupInCancel<E>) {
                auto it = std::remove(source.signals.begin(), source.signals.end(), script);
                source.signals.erase(it, source.signals.end());
            }
        }
    };
    struct MoveContentNode : TmplLeafNode {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return UNBOUND_EVENT_COUNT; }

        const SourceImpl& source;
        Receiver<E>& receiver;

        void commit(Signal<> signal) const {
            source.signals.push_back({ signal, &receiver });
        }
        void cancel(Script script) const {
            if constexpr (!SkipCleanupInCancel<E>) {
                auto it = std::remove(source.signals.begin(), source.signals.end(), script);
                source.signals.erase(it, source.signals.end());
            }
        }
    };
    template<typename Callback>
    struct DispatchContentNode : IgnoreContentNode {
        Callback callback;
        void cancel(Script script, typename IgnoreContentNode::ScratchSpace& scratch) const {
            IgnoreContentNode::cancel(script, scratch);
            scratch.receiver.dispatchTo(callback);
        }
    };
    using NodeType = IgnoreContentNode;

    mutable std::vector<Signal<E>> signals;
    SourceImpl() = default;
    SourceImpl(const SourceImpl&) = delete;
    void sendEvent(const E& event) {
        for (const Signal<E>& signal : signals)
            signal.send(event);
    }
    void sendEvents(const Receiver<E>& events) {
        if (!events.hasEvent())
            return;
        for (const Signal<E>& signal : signals)
            signal.send(events);
    }
    constexpr operator NodeType() const {
        return { {}, *this };
    }
};

template<typename S>
concept IsSource = std::derived_from<S, SourceImpl<typename S::event_type>>;

}

template<typename E>
constexpr typename evt::SourceImpl<E>::MoveContentNode operator>(const evt::SourceImpl<E>& source, evt::Receiver<E>& receiver) {
    return { {}, source, receiver };
}
template<typename E, typename Callback>
constexpr typename evt::SourceImpl<E>::template DispatchContentNode<Callback> operator>(const evt::SourceImpl<E>& source, const Callback& callback) requires requires(Callback callback, evt::Receiver<E> receiver) { receiver.dispatchTo(callback); }
{
    return { { {}, source }, callback };
}

namespace evt {

template<typename E>
concept HasCustomReceiver = requires { typename E::EventReceiver; };
template<typename E>
struct receiver_traits {
    using type = ListReceiver<E>;
};
template<HasCustomReceiver E>
struct receiver_traits<E> {
    using type = typename E::EventReceiver;
};
template<typename E>
concept HasCustomSource = requires { typename E::EventSource; };
template<typename E>
struct source_traits {
    using type = SourceImpl<E>;
};
template<HasCustomSource E>
struct source_traits<E> {
    using type = typename E::EventSource;
};

struct CallbackEvent {
    struct EventSource : SourceImpl<CallbackEvent> {
        void sendEvent(stx::BasicCallback<> callback) {
            sendEvent(CallbackEvent { callback });
        }
        void sendEvent(const CallbackEvent& event) {
            if (signals.size() > 0)
                return SourceImpl::sendEvent(event);
            event.callback();
        }
        void sendEvents(const ListReceiver<CallbackEvent>& events) {
            if (signals.size() > 0)
                return SourceImpl::sendEvents(events);
            for (const CallbackEvent& ev : events.events)
                ev.callback();
        }
    };
    stx::BasicCallback<> callback;
};

}