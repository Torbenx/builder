#include "../ui/text.hpp"
#include "engine.hpp"
#include "../util/log.hpp"
#include <d2d1.h>
#include <d2d1_1.h>
#include <Dwrite.h>
#include <math.h>
#include <locale>
#include <codecvt>

ID2D1Device *d2Device;
ID2D1DeviceContext *d2Context;
IDWriteFactory *writeFactory;

typedef struct{
    HANDLE sharedTexture;
    ID3D11Texture2D *dxTexture;
    ID2D1Bitmap1 *dxBitmap;
} DxTextImpl;

void text_init(){
    ID2D1Factory1 *d2Factroy;
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), (void**)&d2Factroy);
    IDXGIDevice* dxgiDevice;
    dxDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    d2Factroy->CreateDevice(dxgiDevice, &d2Device);
    d2Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2Context);
    dxgiDevice->Release();
    
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&writeFactory);
}

TextTexture::TextTexture(){
    internal = new DxTextImpl();
}

void TextTexture::init(int width, int height){
    DxTextImpl *impl = (DxTextImpl*)internal;
    texture.init();
    D3D11_TEXTURE2D_DESC desc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    HRESULT hr = dxDevice->CreateTexture2D(&desc, NULL, &impl->dxTexture);
    if(hr != S_OK)LOGW("failed to create direct3d texture: 0x%lx", hr);
    
    impl->sharedTexture = wglDXRegisterObjectNV(hDxDevice, impl->dxTexture, texture.glId, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
    
    IDXGISurface *dxSurface;
    hr = impl->dxTexture->QueryInterface(__uuidof(IDXGISurface), (void**)&dxSurface);
    if(hr != S_OK)LOGW("failed to get dxgi surface: 0x%lx", hr);
    
    D2D1_BITMAP_PROPERTIES1 bmProp = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 96.0f, 96.0f);
    hr = d2Context->CreateBitmapFromDxgiSurface(dxSurface, &bmProp, &impl->dxBitmap);
    if(hr != S_OK)LOGW("failed to create direct2d bitmap: 0x%lx", hr);
    dxSurface->Release();
    
    wglDXLockObjectsNV(hDxDevice, 1, &impl->sharedTexture);
    texture.applyTexParameter();
    texture.width = width;
    texture.height = height;
    texture.format = GL_RGBA;
    texture.type = GL_UNSIGNED_INT;
}

void TextTexture::drawText(std::string text, int maxWidth, float textSize, Vec4 color){
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wText = converter.from_bytes(text.data());
    
    DxTextImpl *impl = (DxTextImpl*)internal;
    wglDXUnlockObjectsNV(hDxDevice, 1, &impl->sharedTexture);
    d2Context->SetTarget(impl->dxBitmap);
    
    ID2D1SolidColorBrush *brush;
    d2Context->CreateSolidColorBrush(D2D1::ColorF(color.x, color.y, color.z, color.w), &brush);
    
    IDWriteTextFormat *textFormat;
    writeFactory->CreateTextFormat(L"Verdana", NULL, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, textSize, L"", &textFormat);
    
    d2Context->BeginDraw();
    
    d2Context->Clear();
    d2Context->DrawText(wText.data(), text.length(), textFormat, D2D1::RectF(0, 0, maxWidth < 0 ? INFINITY : maxWidth, texture.height), brush);
    
    d2Context->EndDraw();
    
    brush->Release();
    textFormat->Release();
    
    wglDXLockObjectsNV(hDxDevice, 1, &impl->sharedTexture);
}

TextTexture::~TextTexture(){
    DxTextImpl *impl = (DxTextImpl*)internal;
    wglDXUnregisterObjectNV(hDxDevice, impl->sharedTexture);
    impl->dxBitmap->Release();
    impl->dxTexture->Release();
    delete impl;
}