#pragma once

#include "util/log.hpp"
#include "util/types.hpp"

#define DEBUG_RESOURCE(str) //LOGD(str " refCount=%d", refCount)

template<typename T>
class Resource;

template<typename T>
concept Releasable = requires(T value) { value.release(); };

class ResourceMonitor {
public:
    bool expired() { return refCount == 0; }

    virtual ~ResourceMonitor() {
        DEBUG_RESOURCE("~");
    }

protected:
    virtual bool release() {
        delete this;
        return true;
    }

private:
    int_t refCount = 0;

    void ref() {
        DEBUG_RESOURCE("ref");
        ++refCount;
    }

    bool unref() {
        DEBUG_RESOURCE("unref");
        --refCount;
        if (refCount == 0) {
            return release();
        }
        return false;
    }

    template<typename T>
    friend class Resource;
};

template<typename T>
class Resource {
public:
    const T value;

    Resource()
        : value() { }
    Resource(T value)
        : value(value), monitor(new ResourceMonitor()) { ref(); }
    Resource(const Resource<T>& res)
        : value(res.value), monitor(res.monitor) { ref(); }

    void set(const T& new_value) {
        unref();
        const_cast<T&>(value) = new_value;
        monitor = new ResourceMonitor();
        ref();
    }

    void clear() {
        unref();
        monitor = nullptr;
    }

    void setEternal(const T& new_value) {
        clear();
        const_cast<T&>(value) = new_value;
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

    const T* operator->() { return &value; }
    const T& operator*() { return value; }
    operator const T() { return value; }
    bool hasMonitor() { return monitor != nullptr; }
    ~Resource() { unref(); }

    Resource(const T& value, ResourceMonitor* monitor)
        : value(value), monitor(monitor) { ref(); }

private:
    mutable ResourceMonitor* monitor = nullptr;

    void ref() {
        if (monitor) {
            monitor->ref();
        } else {
            LOGD("ref with no monitor!");
        }
    }

    void do_release() requires Releasable<T> {
        const_cast<T&>(value).release();
    }

    void do_release() requires (!Releasable<T>) {
        LOGD("monitor::release() returned true but no release() function is present on T, resource might be leaked!");
    }

    // always override monitor after unref()
    void unref() {
        if (monitor) {
            if (monitor->unref()) {
                do_release();
            }
        }
    }

    template<typename U>
    friend class Resource;
};
