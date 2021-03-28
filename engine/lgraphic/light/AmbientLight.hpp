#ifndef HEADER_LGAPHIC_LIGHT_AMBIENTLIGHT_H
#define HEADER_LGAPHIC_LIGHT_AMBIENTLIGHT_H

#include "../../math/Vec4.hpp"
#include "../LShader.hpp"

class AmbientLight : public LShader::Effect{
    public:
        Vec3 color;
        AmbientLight(Vec3 color){
            this->color = color;
        }

        void addTo(LShaderBuilder& s){
            LShader::EffectData data;
            s.newContext();
            std::string colorUniform = s.uniqueName("u_color");
            data.handles[0] = s.addFragmentUniform("vec3", colorUniform);
            s.fMain << LShader::LIGHT_INTENSITY << " += " << colorUniform << ";\n";
            s.addEffect(*this, data);
        }

        virtual void apply(LShader& s, const LShader::EffectData& data) override{
            s.uniformVec3(data.handles[0], color);
        }
};

#endif
