#pragma once

#include <3d/CommonEffects.hpp>
#include <3d/PointLight.hpp>
#include <3d/Reflection.hpp>
#include <3d/Renderpass.hpp>
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
#include <ui/renderer/Renderer.hpp>

#include <graphics/render3d/renderer.hpp>

template<Gapi G>
class TestRenderer : public Renderer<G> {
public:
    GAllocator allocator;
    uint64_t lframe;
    uint64_t lsecond;
    int_t frameCounter = 0;
    Vec3 obj5_coords[4];
    Vec3 up;

    PointLight pointLight;
    SpotLight spotLight;
    AmbientLight ambientLight;
    Reflection reflection;
    UniformColor col1, col2, col3, col4, col5;
    TextureColor pawnCol;
    GAllocationHelper<> pawnTexCoords;

    GraphicsObject f1;
    GraphicsObject f2;
    GraphicsObject f3;
    GraphicsObject f4;
    GraphicsObject f5;
    GraphicsObject pawn;
    Resource<IndexBuffer> pawnIndices;
    Resource<Texture> pawnTexture;
    Resource<Texture> spotLightDepthMap;
    Resource<CubeMap> pointLightDepthMap;
    Resource<Texture> reflectionTexture;
    Resource<Texture> reflectionDepthBuffer;

    Framebuffer spotLightFB;
    DepthRenderpass spotLightDepthPass;
    Framebuffer pointLightFBs[6];
    DepthRenderpass pointLightDepthPasses[6];
    Framebuffer reflectionFB;
    Renderpass reflectionPass;

    GLsync lastSync;
    Vec3 pointLightPosition;
    Mat4x4 projection;
    Look3d look;
    uint32_t w = -1, a = -1, s = -1, d = -1, fullscreen = -1, exitKey = -1, upKey = -1, downKey = -1;
    Vec3 pos;
    bool wd = false, ad = false, sd = false, dd = false, upKeyDown = false, downKeyDown = false;
    int_t lookPointerId = POINTER_ID_INVALID, navPointerId = POINTER_ID_INVALID;
    int_t navBaseX, navBaseY;
    int_t navDeltaX = 0, navDeltaY = 0;
    TextComponent textComp;
    TestComponent textBackground;
    DepthCubeMapComponent cubeComp;
    TextureComponent refComp;
    UiScene uiScene;
    Renderpass renderpass;
    GraphicsObjectCollection objects;

    //std::unique_ptr<ui_renderer::Renderer> uiRenderer;
    Resource<Texture> uiTexture;

    using SG = render3d::SG;
    render3d::Renderpass* renderpass3d;
    render3d::StaticNodeSequence<SG::BoundingNode, SG::ObjectNode> o1;
    render3d::StaticNodeSequence<SG::ObjectNode> o2, o3, o4, o5;
    render3d::StaticNodeSequence<SG::CompoundNode> reflectedItems, unreflectedItems;
    render3d::StaticNodeSequence<SG::PortalNode, SG::TransformNode, SG::WorldCoordinatesMarkerNode, SG::CompoundNode> cameraSeq;
    render3d::StaticNodeSequence<SG::PortalNode, SG::TransformNode, SG::CompoundNode> reflectionSeq;

    TestRenderer();
    void change(int_t width, int_t height);
    void pointer(PointerEvent event);
    void rawPointer(int_t dx, int_t dy);
    void rawKey(uint32_t keycode, bool down);
    void drawFrame();
    void updatePointLightData();
    ~TestRenderer();
};
