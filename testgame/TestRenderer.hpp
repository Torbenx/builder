#pragma once

#include <3d/CommonEffects.hpp>
#include <3d/PointLight.hpp>
#include <3d/Reflection.hpp>
#include <3d/Scene.hpp>
#include <3d/SpotLight.hpp>
#include <GL/Shader.hpp>
#include <GL/glwrapper.hpp>
#include <Renderer.hpp>
#include <cmath>
#include <input/Look3d.hpp>
#include <math/Mat4x4.hpp>
#include <math/Vec2.hpp>
#include <math/Vec3.hpp>
#include <ui/DepthCubeMapComponent.hpp>
#include <ui/DepthMapComponent.hpp>
#include <ui/TestComponent.hpp>
#include <ui/TextComponent.hpp>
#include <ui/UiScene.hpp>
#include <util/text.hpp>

class TestRenderer : public Renderer {
public:
    uint64_t lframe;
    uint64_t lsecond;
    int frameCounter = 0;
    Vec3 obj5_coords[4];
    Vec3 up;
    PointLight pointLight;
    SpotLight spotLight;
    AmbientLight ambientLight;
    Reflection reflection;
    Resource<VertexBuffer> f1;
    Resource<VertexBuffer> f2;
    Resource<VertexBuffer> f3;
    Resource<VertexBuffer> f4;
    Resource<VertexBuffer> f5;
    Resource<VertexBuffer> fNormal;
    Resource<VertexBuffer> pawnVertex;
    Resource<VertexBuffer> pawnTexCoords;
    Resource<VertexBuffer> pawnNormals;
    Resource<IndexBuffer> pawnIndices;
    Resource<Texture> pawnTexture;
    Mat4x4 projection;
    Look3d look;
    uint32_t w = -1, a = -1, s = -1, d = -1, fullscreen = -1, exitKey = -1, upKey = -1, downKey = -1;
    Vec3 pos;
    bool wd = false, ad = false, sd = false, dd = false, upKeyDown = false, downKeyDown = false;
    int lookPointerId = POINTER_ID_INVALID, navPointerId = POINTER_ID_INVALID;
    int navBaseX, navBaseY;
    int navDeltaX = 0, navDeltaY = 0;
    TextComponent textComp;
    DepthMapComponent cubeComp;
    TextureComponent refComp;
    Scene scene;
    UiScene uiScene;

    TestRenderer();
    void change(int width, int height);
    void pointer(PointerEvent event);
    void rawPointer(int dx, int dy);
    void rawKey(uint32_t keycode, bool down);
    void drawFrame();
    ~TestRenderer();
};
