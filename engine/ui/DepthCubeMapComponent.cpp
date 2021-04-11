#include "ui/DepthCubeMapComponent.hpp"

DepthCubeMapComponent::DepthCubeMapComponent()
    : CubeMapComponent(true) {
}

void DepthCubeMapComponent::doUniforms() {
    //shader->uniform1f(static_cast<SimpleDepthCubeMapShader*>(shader.get())->farUniform, farDepth);
}
