#ifndef HEADER_UTIL_SOURCEGETTER_H
#define HEADER_UTIL_SOURCEGETTER_H

//#include "SourceParser.hpp"
//#include "SourceDescription.hpp"

template <typename S>
class SourceGetter{
    public:
        virtual S getSource() = 0;
        virtual ~SourceGetter(){};
};

template <typename S>
class DirectSourceGetter : public SourceGetter<S>{
    public:
        S source;
        
        DirectSourceGetter(S source){
            this->source = source;
        }
        
        S getSource(){
            return source;
        }
};

template <typename S>
class NullSourceGetter : public SourceGetter<S>{
    public:
        S getSource(){
            return nullptr;
        }
};

#endif
