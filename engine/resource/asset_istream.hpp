#ifndef HEADER_RESOURCE_ASSET_ISTREAM_H
#define HEADER_RESOURCE_ASSET_ISTREAM_H

#include <fstream>
#include <iostream>

#ifdef PLATFORM_ANDROID
#include "../android/assetbuf.hpp"
#endif

#ifdef PLATFORM_WINDOWS
class assetbuf : public std::filebuf{
    public:
        assetbuf(std::string file);
};
#endif

class asset_istream : public std::istream{
    private:
        assetbuf buf;
    public:
        asset_istream(std::string file);
};

#endif