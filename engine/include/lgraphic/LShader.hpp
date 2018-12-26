#include "../graphic/Shader.hpp"
#include "ShaderBuilder.hpp"
#include "reflection/Reflection.hpp"

#ifndef HEADER_LGRAPHIC_LSHADER_H
#define HEADER_LGRAPHIC_LSHADER_H

class LShader : public Shader{
    public:
        std::map<void*,std::string> locations;
        std::map<void*,unsigned int> textures;
        unsigned int textureSlot = 0;
        LShader(std::string vSource,std::string fSource,std::map<void*,std::string> locations);
        void lighting(AmbientLighting& l);
        void lighting(SpotLighting& l,Mat4x4 model);
        void lighting(PointLighting& l,Mat4x4 model);
        void reflection(Reflection& r);
        unsigned int getTextureSlot(void *tex);
};

#endif
