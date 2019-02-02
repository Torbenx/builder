#include "DepthMapComponent.hpp"

DepthMapComponent::DepthMapComponent() : DepthMapComponent("sampler2D", "texture2D"){
    
}

DepthMapComponent::DepthMapComponent(std::string sampler, std::string texFunc) : TextureComponent(
    "uniform float u_far;\n",
    sampler, texFunc,
    "vec4 bits = vec4(u_far,u_far/255.0,u_far/255.0/255.0,u_far/255.0/255.0/255.0);\n"
    "gl_FragColor = vec4(vec3(dot(bits,gl_FragColor)), 1.0);\n"
){
    
}

void DepthMapComponent::doUniforms(){
    shader.uniform1f("u_far", farDepth);
}