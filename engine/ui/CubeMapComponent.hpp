#pragma once

#include "UiComponent.hpp"
#include "resource/Resource.hpp"
#include "util/Shaders.hpp"

class CubeMapComponent : public UiComponent {
public:
    Resource<SimpleCubeMapShader> shader;
    CubeMap* cubeMap;
    Resource<VertexBuffer> coords;
    Resource<VertexBuffer> texCoords;
    CubeMapComponent(bool createShader = true);
    ~CubeMapComponent([[context]] GAllocator* allocator) { }
    void draw(Framebuffer&, [[context]] GAllocator*) override;
    void updateRect(ScreenMetrics&, [[context]] GAllocator*) override;
    virtual void doUniforms() { }
    virtual void positiveX(Rect rect);
    virtual void negativeX(Rect rect);
    virtual void positiveY(Rect rect);
    virtual void negativeY(Rect rect);
    virtual void positiveZ(Rect rect);
    virtual void negativeZ(Rect rect);
};
