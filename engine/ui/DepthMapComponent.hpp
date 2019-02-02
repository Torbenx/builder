#ifndef HEADER_UI_DEPTH_MAP_COMPONENT_H
#define HEADER_UI_DEPTH_MAP_COMPONENT_H

#include "TextureComponent.hpp"

class DepthMapComponent : public TextureComponent{
    public:
        float farDepth;
        DepthMapComponent();
        DepthMapComponent(std::string sampler, std::string texFunc);
        void doUniforms();
};

#endif