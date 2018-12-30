#ifndef HEADER_UTIL_HANDLE_H
#define HEADER_UTIL_HANDLE_H

#include "log.hpp"

template<typename H>
class Handle{
    private:
        unsigned int checkRefCount = 0;
    public:
        H value;
        unsigned int refCount = 0;
        Handle(H value){
            this->value = value;
            refCount++;
            checkRefCount++;
        }
        Handle(const Handle<H>& other){
            value = other.value;
            refCount++;
            checkRefCount++;
        }
        ~Handle(){
            if(--checkRefCount != refCount){
                LOGW("unref not called");
            }
        }
        bool unref(){
            return --refCount == 0;
        }
};

#endif