#pragma once

#include <3d/Scene.hpp>
#include <GL/Shader.hpp>
#include <GL/glwrapper.hpp>
#include <Renderer.hpp>
#include <cmath>
#include <input/Look3d.hpp>
#include <lgraphic/DepthMap.hpp>
#include <math/Mat4x4.hpp>
#include <math/Vec2.hpp>
#include <math/Vec3.hpp>
#include <ui/DepthCubeMapComponent.hpp>
#include <ui/DepthMapComponent.hpp>
#include <ui/TestComponent.hpp>
#include <ui/TextComponent.hpp>
#include <ui/UiScene.hpp>
#include <util/text.hpp>

class DebugRenderable : public Scene::Object {
public:
    Buffer<Vec3>* source;
    Buffer<Vec3>* normals;
    Vec3 color;
    Mat4x4 model;
    int num;
    DrawMode mode = DrawMode::Triangles;

    DebugRenderable(Buffer<Vec3>* s, Buffer<Vec3>* nor, Vec3 c, int n, Mat4x4 m = Mat4x4::Id) {
        source = s;
        normals = nor;
        color = c;
        num = n;
        model = m;
    }

    Scene::ColorSource getColorSource() {
        return Scene::ColorSource::Uniform;
    }

    void draw(Framebuffer& framebuffer, Shader& shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform) {
        shader.uniformMat4x4(modelUniform, model);
        shader.attribPointer(positionAttrib, *source);
        framebuffer.draw(mode, num);
    }

    void draw2(Framebuffer& framebuffer, LShader& shader, const Scene::DrawData& data) {
        shader.uniformMat4x4(data.modelUniform, model);
        shader.uniformVec3(data.colorSource.uniform, color);
        shader.attribPointer(data.positionAttrib, *source);
        shader.attribPointer(data.normalAttrib, *normals);
        framebuffer.draw(mode, num);
    }
};

class ObjectRenderable : public Scene::Object {
public:
    Buffer<float>* source;
    Buffer<float>* texCoords;
    Buffer<float>* normals;
    Buffer<GLuint>* indices;
    Texture* texture;
    Mat4x4 model;
    int num;

    ObjectRenderable(Buffer<float>* s, Buffer<GLuint>* i, Buffer<float>* t, Buffer<float>* nor, Texture* tex, int n, Mat4x4 m = Mat4x4::Id) {
        source = s;
        indices = i;
        texCoords = t;
        normals = nor;
        texture = tex;
        num = n;
        model = m;
    }

    Scene::ColorSource getColorSource() {
        return Scene::ColorSource::Texture;
    }

    void draw(Framebuffer& framebuffer, Shader& shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform) {
        shader.uniformMat4x4(modelUniform, model);
        shader.attribPointer(positionAttrib, 3, *source);
        if (indices)
            framebuffer.draw(DrawMode::Triangles, *indices);
        else
            framebuffer.draw(DrawMode::Triangles, num);
    }

    void draw2(Framebuffer& framebuffer, LShader& shader, const Scene::DrawData& data) {
        shader.uniformMat4x4(data.modelUniform, model);
        shader.attribPointer(data.positionAttrib, 3, *source);
        shader.attribPointer(data.normalAttrib, 3, *normals);
        shader.uniformTexture(data.colorSource.texture.uniform, *texture, 10);
        shader.attribPointer(data.colorSource.texture.positionAttrib, 2, *texCoords);
        if (indices)
            framebuffer.draw(DrawMode::Triangles, *indices);
        else
            framebuffer.draw(DrawMode::Triangles, num);
    }
};

class ReflectionDebugRenderable : public DebugRenderable {
public:
    Reflection* reflection;

    ReflectionDebugRenderable(Buffer<Vec3>* s, Buffer<Vec3>* nor, Vec3 c, int n, Reflection* r, Mat4x4 m = Mat4x4::Id)
        : DebugRenderable(s, nor, c, n, m) {
        reflection = r;
    }

    Reflection* getReflection() {
        return reflection;
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
    std::unique_ptr<Buffer<Vec3>> f1;
    std::unique_ptr<Buffer<Vec3>> f2;
    std::unique_ptr<Buffer<Vec3>> f3;
    std::unique_ptr<Buffer<Vec3>> f4;
    std::unique_ptr<Buffer<Vec3>> f5;
    std::unique_ptr<Buffer<Vec3>> fNormal;
    std::unique_ptr<Buffer<float>> pawnVertex;
    std::unique_ptr<Buffer<GLuint>> pawnIndices;
    std::unique_ptr<Buffer<float>> pawnTexCoords;
    std::unique_ptr<Buffer<float>> pawnNormals;
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
