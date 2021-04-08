#include "ui/DepthMapComponent.hpp"

DepthMapComponent::DepthMapComponent() : TextureComponent(true){
}

void DepthMapComponent::doUniforms(){
    //currently this doesn't override the shader
    //shader->uniform1f(static_cast<SimpleDepthMapShader*>(shader.get())->farUniform, farDepth);
}
