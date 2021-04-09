#pragma once

#include "lgraphic/AmbientLight.hpp"
#include "lgraphic/LShader.hpp"
#include "lgraphic/PointLight.hpp"
#include "lgraphic/Reflection.hpp"
#include "lgraphic/SpotLight.hpp"
#include <list>
#include <memory>
#include <vector>

class Scene {
public:
    enum ColorSource {
        Uniform,
        Texture,
    };
    struct DrawData {
        Shader::LocationHandle positionAttrib;
        Shader::LocationHandle normalAttrib;
        union {
            Shader::LocationHandle attrib;
            Shader::LocationHandle uniform;
            struct {
                Shader::LocationHandle positionAttrib;
                Shader::LocationHandle uniform;
            } texture;
        } colorSource;
    };
    struct Object : public Renderable {
        virtual ColorSource getColorSource() = 0;
        bool hasReflection() { return getReflection() != nullptr; }
        virtual Reflection* getReflection() { return nullptr; }
        virtual void draw2(Framebuffer& framebuffer, LShader& shader, const DrawData& data) = 0;
    };
    struct ObjectData {
        Object* object;
        Mat4x4 model;
    };
    struct ShaderInstance {
        std::unique_ptr<LShader> shader;
        DrawData data;
        Shader::LocationHandle vpUniform;
        Shader::LocationHandle modelUniform;
        std::vector<ObjectData> objects;
    };
    void addLight(PointLight& light);
    void addLight(SpotLight& light);
    void addLight(AmbientLight& light);
    void addReflection(Reflection& ref);
    void addObject(Object* obj, Mat4x4& model = Mat4x4::Id);
    void draw(Mat4x4 vp);

private:
    std::list<ShaderInstance> shaders;
    std::vector<PointLight*> pointLights;
    std::vector<SpotLight*> spotLights;
    std::vector<AmbientLight*> ambientLights;
    std::vector<Reflection*> reflections;
    void doDraw(Framebuffer& framebuffer, Mat4x4& mvp, bool drawReflections = true);
};
