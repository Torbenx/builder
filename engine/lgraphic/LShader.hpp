#ifndef HEADER_LGRAPHIC_LSHADER_H
#define HEADER_LGRAPHIC_LSHADER_H

#include "../graphic/Shader.hpp"
#include "reflection/Reflection.hpp"
#include "light/AmbientLight.hpp"
#include "light/SpotLight.hpp"
#include "light/PointLight.hpp"

class LShader : public Shader{
    public:
        std::map<void*,std::string> locations;
        std::map<void*,unsigned int> textures;
        unsigned int textureSlot = 0;
        LShader(std::string vSource,std::string fSource,std::map<void*,std::string> locations);
        void light(AmbientLight& l);
        void light(SpotLight& l,Mat4x4 model);
        void light(PointLight& l,Mat4x4 model);
        void reflection(Reflection& r);
        unsigned int getTextureSlot(void *tex);
};

#endif
