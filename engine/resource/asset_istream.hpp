#pragma once

#include <iostream>

class asset_istream : public std::istream{
    public:
        asset_istream(std::string file);
        ~asset_istream();
    private:
        asset_istream(const asset_istream& other) = delete;
};
