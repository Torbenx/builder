#pragma once

#include "lgraphic/LShader.hpp"
#include "math/Vec4.hpp"

class AmbientLight : public LShader::Effect {
public:
    Vec3 color;
    AmbientLight(Vec3 color) {
        this->color = color;
    }

    static LShader::EffectLocationHandle addTo(LShaderBuilder& s) {
        LShader::EffectData data;
        s.newContext();
        std::string colorUniform = s.uniqueName("u_color");
        data.handles[0] = s.addFragmentUniform("vec3", colorUniform);
        s.fMain << LShader::LIGHT_INTENSITY << " += " << colorUniform << ";\n";
        return s.addEffect(data);
    }

    virtual void bindTo(LShader& s, const LShader::EffectData& data) override {
        s.uniformVec3(data.handles[0], color);
    }
};
