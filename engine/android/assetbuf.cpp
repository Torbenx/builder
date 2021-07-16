#include "android/assetbuf.hpp"
#include "resource/asset_istream.hpp"
#include "util/log.hpp"
#include <string.h>

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
            log<Tag::Android>("Error");
            return traits_type::eof();
        }else if(ret == 0){
            return traits_type::eof();
        }else{
            setg(buffer, buffer, buffer+ret);
        }
    }
    return traits_type::to_int_type(*gptr());
}

std::streampos assetbuf::seekpos(std::streampos, std::ios_base::openmode){
    log<Tag::Android>("assetbuf::seekpos not implemented");
    return std::streampos(std::streamoff(-1));
}

std::streampos assetbuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode){
    if(!asset){
        return std::streampos(-1);
    }
    int whence;
    switch(way){
        case std::ios_base::seekdir::beg:
            whence = SEEK_SET;
            break;
        case std::ios_base::seekdir::cur:
            whence = SEEK_CUR;
            break;
        case std::ios_base::seekdir::end:
            whence = SEEK_END;
            break;
    }
    return std::streampos(AAsset_seek(asset, off, whence));
}

assetbuf::~assetbuf(){
    if(asset != nullptr){
        delete buffer;
        AAsset_close(asset);
    }
}
