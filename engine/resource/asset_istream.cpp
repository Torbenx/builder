#include "asset_istream.hpp"
#include <fstream>

#ifdef PLATFORM_WINDOWS

class assetbuf : public std::filebuf{
    public:
        assetbuf(std::string file);
};

assetbuf::assetbuf(std::string file) : std::filebuf(){
    open(file, std::ios_base::in);
}

asset_istream::asset_istream(std::string file) : std::istream(new assetbuf(file)){
    
}

asset_istream::~asset_istream(){
    delete rdbuf();
}

#endif

