#include "assetbuf.hpp"
#include "../resource/asset_istream.hpp"
#include <string.h>
#include "../util/log.hpp"

asset_istream::asset_istream(std::string file) : std::istream(new assetbuf(file)){
    
}

asset_istream::~asset_istream(){
    delete rdbuf();
}

assetbuf::assetbuf(std::string file){
    asset = AAssetManager_open(assetManager, file.c_str(), AASSET_MODE_STREAMING);
    if(asset != nullptr){
        bufferSize = 2048;
        buffer = new char[bufferSize];
        setg(buffer, buffer, buffer);
    }
}

std::streambuf::int_type assetbuf::underflow(){
    if(asset == nullptr){
        return traits_type::eof();
    }
    if(gptr() == egptr()){
        int ret = AAsset_read(asset, buffer, bufferSize);
        if(ret < 0){
            LOGW("Error");
            return traits_type::eof();
        }else if(ret == 0){
            return traits_type::eof();
        }else{
            setg(buffer, buffer, buffer+ret);
        }
    }
    return traits_type::to_int_type(*gptr());
}

assetbuf::~assetbuf(){
    if(asset != nullptr){
        delete buffer;
        AAsset_close(asset);
    }
}