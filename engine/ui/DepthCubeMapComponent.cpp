#include "ui/DepthCubeMapComponent.hpp"

DepthCubeMapComponent::DepthCubeMapComponent() : CubeMapComponent(false){
    shader = Shaders::depthCubeMap();
}

void DepthCubeMapComponent::doUniforms(){
    shader->uniform1f(static_cast<SimpleDepthCubeMapShader*>(shader.get())->farUniform, farDepth);
}
