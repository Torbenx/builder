#ifndef HEADER_ANDROID_ASSETBUG_H
#define HEADER_ANDROID_ASSETBUG_H

#include <iostream>
#include "engine.hpp"

class assetbuf : public std::streambuf{
    private:
        AAsset *asset;
        int_type underflow();
        char *buffer;
        int bufferSize;
    public:
        assetbuf(std::string file);
        ~assetbuf();
};

#endif