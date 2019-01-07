#include "../ui/text.hpp"
#include "engine.hpp"
#include "../util/log.hpp"
#include <d2d1.h>
#include <d2d1_1.h>

ID2D1Device *d2Device;
ID2D1DeviceContext *d2Context;

void text_init(){
    
    ID2D1Factory1 *d2Factroy;
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), (void**)&d2Factroy);
    IDXGIDevice* dxgiDevice;
    dxDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    d2Factroy->CreateDevice(dxgiDevice, &d2Device);
    d2Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2Context);
    dxgiDevice->Release();
}

void initalizeTextureForText(Texture& texture, int width, int height){
    D3D11_TEXTURE2D_DESC desc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    ID3D11Texture2D *dxTexture;
    HRESULT hr = dxDevice->CreateTexture2D(&desc, NULL, &dxTexture);
    if(hr != S_OK)LOGW("failed to create direct3d texture: 0x%lx", hr);
    
    HANDLE sharedTexture = wglDXRegisterObjectNV(hDxDevice, dxTexture, texture.glId, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
    
    IDXGISurface *dxSurface;
    hr = dxTexture->QueryInterface(__uuidof(IDXGISurface), (void**)&dxSurface);
    if(hr != S_OK)LOGW("failed to get dxgi surface: 0x%lx", hr);
    
    ID2D1Bitmap1 *dxBitmap;
    D2D1_BITMAP_PROPERTIES1 bmProp = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.0f, 96.0f);
    hr = d2Context->CreateBitmapFromDxgiSurface(dxSurface, &bmProp, &dxBitmap);
    if(hr != S_OK)LOGW("failed to create direct2d bitmap: 0x%lx", hr);
    
    d2Context->SetTarget(dxBitmap);
    
    ID2D1SolidColorBrush *brush;
    d2Context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &brush);
    d2Context->BeginDraw();
    d2Context->DrawRectangle(D2D1::RectF(100.0f, 100.0f, -100.0f, -100.0f),brush);
    LOGD("test: %lx",d2Context->EndDraw());
    
    wglDXLockObjectsNV(hDxDevice, 1, &sharedTexture);
    dxSurface->Release();
}