#pragma once

#include "ui/CubeMapComponent.hpp"

class DepthCubeMapComponent : public CubeMapComponent{
    public:
        float farDepth;
        DepthCubeMapComponent();
        void doUniforms();
};
