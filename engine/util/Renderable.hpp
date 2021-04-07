#pragma once

#include "GL/Shader.hpp"
#include <string>

class Renderable{
    public:
        virtual void draw(
            Framebuffer& framebuffer,
            Shader& shader,
            Shader::LocationHandle postionHandle
        ) = 0;
        virtual ~Renderable(){}
};
