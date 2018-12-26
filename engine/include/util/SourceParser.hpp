#ifndef HEADER_UTIL_SOURCEPARSER_H
#define HEADER_UTIL_SOURCEPARSER_H

#include <iostream>

template<typename O>
class SourceParser{
    public:
        virtual O parse(std::istream in) = 0;
};

#endif
