#ifndef HEADER_UTIL_PTR_H
#define HEADER_UTIL_PTR_H

#include <memory>
#include "log.hpp"

template< class T >
class return_ptr{
public:
    return_ptr(T* ptr) : ptr(ptr) {};

    T&& move(){
        return std::move(*ptr);
    }

    T* release(){
        T* p = ptr;
        ptr = nullptr;
        return p;
    }

    std::unique_ptr<T> unique(){
        auto p = std::unique_ptr<T>();
        p.reset(release());
        return p;
    }

    std::shared_ptr<T> shared(){
        auto p = std::shared_ptr<T>();
        p.reset(release());
        return p;
    }

    ~return_ptr(){
        if(ptr){
            delete ptr;
        }
    }
private:
    T *ptr;
};

#endif
