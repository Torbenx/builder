#include "resource/JpegDecoder.hpp"
#include <turbojpeg.h>

void JpegDecoder::decode(std::istream& in) {
    width = 0;
    height = 0;
    in.seekg(0, in.end);
    int fileSize = in.tellg();
    in.seekg(0, in.beg);
    if (fileSize <= 0) {
        LOGE("failed: %d", fileSize);
        return;
    }
    unsigned char* fileBuf = new unsigned char[fileSize];
    in.read((char*)fileBuf, fileSize);

    tjhandle tjInstance = tjInitDecompress();

    int subsamp, colorspace;
    if (tjDecompressHeader3(tjInstance, fileBuf, fileSize, &width, &height, &subsamp, &colorspace) < 0) {
        LOGE("FAIL");
        return;
    }
    LOGD("size: %d x %d", width, height);
    int pixelFormat = TJPF_RGBX;
    int pixelSize = tjPixelSize[pixelFormat];
    unsigned char* imgBuf = new unsigned char[width * height * pixelSize];
    tjDecompress2(tjInstance, fileBuf, fileSize, imgBuf, width, 0, height, pixelFormat, 0);

    texture = Texture::color(imgBuf, width, height, Texture::Format::RGBA8888);
}
