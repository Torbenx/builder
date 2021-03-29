#ifndef HEADER_UI_DEPTH_CUBE_MAP_COMPONENT_H
#define HEADER_UI_DEPTH_CUBE_MAP_COMPONENT_H

#include "CubeMapComponent.hpp"

class DepthCubeMapComponent : public CubeMapComponent{
    public:
        float farDepth;
        DepthCubeMapComponent();
        void doUniforms();
};

#endif
