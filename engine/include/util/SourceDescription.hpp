#ifndef HEADER_UTIL_SOURCEDESCRIPTION_H
#define HEADER_UTIL_SOURCEDESCRIPTION_H

#include <iostream>

class SourceDiscription{
    virtual std::istream open() = 0;
};

#endif
