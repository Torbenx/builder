#ifndef HEADER_UTIL_RENDERABLE_H
#define HEADER_UTIL_RENDERABLE_H

#include <string>
#include "../graphic/Shader.hpp"

class Renderable{
    public:
        virtual void draw(
            Shader& shader,
            Shader::LocationHandle postionHandle,
            Shader::LocationHandle modelHandle
        ) = 0;
        virtual ~Renderable(){}
};

#endif
