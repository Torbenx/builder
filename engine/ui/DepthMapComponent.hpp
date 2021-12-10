#pragma once

#include "ui/TextureComponent.hpp"

class DepthMapComponent : public TextureComponent {
public:
    float farDepth;
    DepthMapComponent();
    void doUniforms() override;
    ~DepthMapComponent([[context]] GAllocator*) { }
};
