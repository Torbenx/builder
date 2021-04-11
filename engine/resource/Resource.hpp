#pragma once

template<typename T>
class Resource {
public:
    const T value;

    Resource() { }
    Resource(T value)
        : value(value), monitor(new int(1)) { }

    void set(const T& new_value) {
        unref();
        const_cast<T&>(value) = new_value;
        monitor = new int(1);
    }

    void clear() {
        unref();
        monitor = nullptr;
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

    void setEternal(const T& new_value) {
        clear();
        const_cast<T&>(value) = new_value;
    }

    ~Resource() {
        unref();
    }

private:
    int* monitor = nullptr;

    void ref() {
        if (monitor) {
            ++(*monitor);
        } else {
            LOGD("ref with no monitor!");
        }
    }

    //always override monitor after unref()
    void unref() {
        if (monitor) {
            if (--(*monitor) == 0) {
                const_cast<T&>(value).release();
                delete monitor;
            }
        }
    }
};
