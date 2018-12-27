#include "asset_istream.hpp"

#ifdef PLATFORM_WINDOWS
assetbuf::assetbuf(std::string file) : std::filebuf(){
    open(file, std::ios_base::in);
}
#endif

asset_istream::asset_istream(std::string file) : std::istream(&buf), buf(file){
    //rdbuf(&buf);
}