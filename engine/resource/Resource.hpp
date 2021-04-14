#pragma once

#include "util/log.hpp"

#define DEBUG_RESOURCE(str) //LOGD(str " refCount=%d, weakRefCount=%d", refCount, weakRefCount)

struct Monitor {
    int refCount;
    int weakRefCount;

    // default construct with 1 reference
    Monitor()
        : refCount(1), weakRefCount(0) { }

    void ref() {
        DEBUG_RESOURCE("ref");
        ++refCount;
    }
    bool unref() {
        DEBUG_RESOURCE("unref");
        --refCount;
        return refCount == 0;
    }
    void weakRef() {
        DEBUG_RESOURCE("weakRef");
        ++weakRefCount;
    }
    void weakUnref() {
        DEBUG_RESOURCE("weakUnref");
        --weakRefCount;
    }
    bool dead() { return refCount == 0 && weakRefCount == 0; }

    ~Monitor() {
        DEBUG_RESOURCE("~");
    }
};

template<typename T>
class Resource {
public:
    class Weak {
    public:
        Weak()
            : value() { }

        Weak& operator=(const Weak& other) {
            unref();
            const_cast<T&>(value) = other.value;
            monitor = other.monitor;
            ref();
            return *this;
        }

        Resource<T> acquire() {
            if (expired()) {
                LOGD("acquire on expired resource");
                return Resource<T>();
            }
            return Resource<T>(value, monitor);
        }

        bool expired() {
            if (!monitor)
                return true;
            if (monitor->refCount == 0) {
                unref();
                return true;
            }
            return false;
        }

        ~Weak() {
            unref();
        }

    private:
        const T value;
        Monitor* monitor = nullptr;

        Weak(const T& value, Monitor* monitor)
            : value(value), monitor(monitor) { ref(); }

        void ref() {
            if (monitor) {
                monitor->weakRef();
            } else {
                LOGD("Weak::ref with no monitor!");
            }
        }

        void unref() {
            if (monitor) {
                monitor->weakUnref();
                if (monitor->dead()) {
                    delete monitor;
                }
            }
        }
        friend Resource;
    };

    const T value;

    Resource()
        : value() { }
    Resource(T value)
        : value(value), monitor(new Monitor()) { }
    Resource(const Resource<T>& res)
        : value(res.value), monitor(res.monitor) { ref(); }

    void set(const T& new_value) {
        unref();
        const_cast<T&>(value) = new_value;
        monitor = new Monitor();
    }

    void clear() {
        unref();
        monitor = nullptr;
    }

    template<typename U>
    Resource<T>& operator=(const Resource<U>& res) {
        unref();
        const_cast<T&>(value) = res.value;
        monitor = res.monitor;
        ref();
        return *this;
    }

    Resource<T>& operator=(const Resource<T>& res) {
        unref();
        const_cast<T&>(value) = res.value;
        monitor = res.monitor;
        ref();
        return *this;
    }

    Resource<T>& operator=(const T& new_value) {
        set(new_value);
        return *this;
    }

    const T* operator->() {
        return &value;
    }

    const T& operator*() {
        return value;
    }

    operator const T() {
        return value;
    }

    void setEternal(const T& new_value) {
        clear();
        const_cast<T&>(value) = new_value;
    }

    Weak weak() {
        return Weak(value, monitor);
    }

    ~Resource() {
        unref();
    }

    Monitor* monitor = nullptr;

private:
    Resource(const T& value, Monitor* monitor)
        : value(value), monitor(monitor) { ref(); }

    void ref() {
        if (monitor) {
            monitor->ref();
        } else {
            LOGD("ref with no monitor!");
        }
    }

    // always override monitor after unref()
    void unref() {
        if (monitor) {
            if (monitor->unref()) {
                const_cast<T&>(value).release();
                if (monitor->dead()) {
                    delete monitor;
                }
            }
        }
    }
};
