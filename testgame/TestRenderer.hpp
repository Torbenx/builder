#pragma once

#include <3d/Scene.hpp>
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

class DebugRenderable : public SceneRenderable {
public:
    VertexBuffer* source;
    VertexBuffer* normals;
    Vec3 color;
    int num;
    DrawMode mode = DrawMode::Triangles;

    DebugRenderable(VertexBuffer* s, VertexBuffer* nor, Vec3 c, int n, Mat4x4 m = Mat4x4::Id) {
        source = s;
        normals = nor;
        color = c;
        num = n;
    }

    void depthDraw(Framebuffer& framebuffer, Shader& shader, Shader::LocationHandle positionAttrib) {
        shader.attribPointer(positionAttrib, *source);
        framebuffer.draw(mode, num);
    }

    std::vector<EffectBindings> addEffects(EffectShaderBuilder& builder) {
        EffectBindings data;
        data.handles[0] = builder.addFragmentUniform("vec3", "u_color");
        builder.fMain << Shader::FRAGMENT_COLOR << " = vec4(u_color * " << EffectShader::LIGHT_INTENSITY << ", 1.0);\n";
        return { data };
    }

    void draw(Framebuffer& framebuffer, EffectShader& shader, std::vector<EffectBindings>& data) {
        shader.uniformVec3(data[0].handles[0], color);
        shader.attribPointer(shader.positionAttrib, *source);
        shader.attribPointer(shader.normalAttrib, *normals);
        framebuffer.draw(mode, num);
    }
};

class ObjectRenderable : public SceneRenderable {
public:
    VertexBuffer* source;
    VertexBuffer* texCoords;
    VertexBuffer* normals;
    IndexBuffer* indices;
    Texture* texture;
    int num;

    ObjectRenderable(VertexBuffer* s, IndexBuffer* i, VertexBuffer* t, VertexBuffer* nor, Texture* tex, int n) {
        source = s;
        indices = i;
        texCoords = t;
        normals = nor;
        texture = tex;
        num = n;
    }

    std::vector<EffectBindings> addEffects(EffectShaderBuilder& builder) {
        //EffectBindings data;
        return {};
    }

    void depthDraw(Framebuffer& framebuffer, Shader& shader, Shader::LocationHandle positionAttrib) {
        shader.attribPointer(positionAttrib, *source);
        if (indices)
            framebuffer.draw(DrawMode::Triangles, *indices);
        else
            framebuffer.draw(DrawMode::Triangles, num);
    }

    void draw(Framebuffer& framebuffer, EffectShader& shader, std::vector<EffectBindings>& data) {
        /*shader.attribPointer(shader.positionAttrib, *source);
        shader.attribPointer(shader.normalAttrib, *normals);
        shader.attribPointer(data[0].handles[0], *texCoords);
        shader.uniformTexture(data[0].handles[1], *texture, data[0].textureSlot);
        if (indices)
            framebuffer.draw(DrawMode::Triangles, *indices);
        else
            framebuffer.draw(DrawMode::Triangles, num);*/
    }
};

class ReflectionDebugRenderable : public DebugRenderable {
public:
    Reflection* reflection;

    ReflectionDebugRenderable(VertexBuffer* s, VertexBuffer* nor, Vec3 c, int n, Reflection* r)
        : DebugRenderable(s, nor, c, n) {
        reflection = r;
    }
};

class TestRenderer : public Renderer {
public:
    DebugRenderable* obj1;
    DebugRenderable* obj2;
    ReflectionDebugRenderable* obj3;
    DebugRenderable* obj4;
    DebugRenderable* obj5;
    ObjectRenderable* pawn;
    uint64_t lframe;
    uint64_t lsecond;
    int frameCounter = 0;
    Vec3 obj5_coords[4];
    Vec3 up;
    PointLight pointLight;
    SpotLight spotLight;
    AmbientLight ambientLight;
    Reflection reflection;
    std::unique_ptr<VertexBuffer> f1;
    std::unique_ptr<VertexBuffer> f2;
    std::unique_ptr<VertexBuffer> f3;
    std::unique_ptr<VertexBuffer> f4;
    std::unique_ptr<VertexBuffer> f5;
    std::unique_ptr<VertexBuffer> fNormal;
    std::unique_ptr<VertexBuffer> pawnVertex;
    std::unique_ptr<VertexBuffer> pawnTexCoords;
    std::unique_ptr<VertexBuffer> pawnNormals;
    std::unique_ptr<IndexBuffer> pawnIndices;
    std::unique_ptr<Texture> pawnTexture;
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
