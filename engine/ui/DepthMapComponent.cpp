#include "ui/DepthMapComponent.hpp"

DepthMapComponent::DepthMapComponent() : TextureComponent(true){
}

void DepthMapComponent::doUniforms(){
    //shader->uniform1f(dynamic_cast<SimpleDepthMapShader*>(shader.get())->farUniform, farDepth);
}
