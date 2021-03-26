#ifndef HEADER_ANDROID_ASSETBUF_H
#define HEADER_ANDROID_ASSETBUF_H

#include <iostream>
#include "engine.hpp"

class assetbuf : public std::streambuf{
    protected:
        AAsset *asset;
        char *buffer;
        int bufferSize;
        virtual int_type underflow() override;
        virtual std::streampos seekpos(std::streampos, std::ios_base::openmode) override;
        virtual std::streampos seekoff(std::streamoff, std::ios_base::seekdir, std::ios_base::openmode) override;

    public:
        assetbuf(std::string file);
        ~assetbuf();
};

#endif
