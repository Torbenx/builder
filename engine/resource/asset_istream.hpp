#ifndef HEADER_RESOURCE_ASSET_ISTREAM_H
#define HEADER_RESOURCE_ASSET_ISTREAM_H

#include <iostream>

class asset_istream : public std::istream{
    public:
        asset_istream(std::string file);
        ~asset_istream();
    private:
        asset_istream(const asset_istream& other) = delete;
};

#endif