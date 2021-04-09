#pragma once

#include "3d/Effect.hpp"
#include "math/Vec4.hpp"

class AmbientLight : public EffectInstance {
public:
    static inline struct Effect : public ::Effect {
        virtual EffectBindings addTo(EffectShaderBuilder& s) override {
            EffectBindings data;
            s.newContext();
            std::string colorUniform = s.uniqueName("u_color");
            data.handles[0] = s.addFragmentUniform("vec3", colorUniform);
            s.fMain << EffectShader::LIGHT_INTENSITY << " += " << colorUniform << ";\n";
            return data;
        }
    } EFFECT;

    Vec3 color;
    AmbientLight(Vec3 color) : EffectInstance(&EFFECT) {
        this->color = color;
    }

    virtual void applyTo(EffectShader& s, const EffectBindings& data) override {
        s.uniformVec3(data.handles[0], color);
    }
};
