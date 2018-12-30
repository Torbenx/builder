#ifndef HEADER_ANDROID_ASSETBUF_H
#define HEADER_ANDROID_ASSETBUF_H

#include <iostream>
#include "engine.hpp"

class assetbuf : public std::streambuf{
    private:
        AAsset *asset;
        char *buffer;
        int bufferSize;
        int_type underflow();
    public:
        assetbuf(std::string file);
        ~assetbuf();
};

#endif