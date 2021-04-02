#pragma once

#include "GL/Shader.hpp"
#include <string>

class Renderable{
    public:
        virtual void draw(
            Framebuffer& framebuffer,
            Shader& shader,
            Shader::LocationHandle postionHandle,
            Shader::LocationHandle modelHandle
        ) = 0;
        virtual ~Renderable(){}
};
