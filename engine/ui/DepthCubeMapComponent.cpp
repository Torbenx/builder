#include "DepthCubeMapComponent.hpp"

DepthCubeMapComponent::DepthCubeMapComponent() : DepthCubeMapComponent("samplerCube", "textureCube"){
    
}

DepthCubeMapComponent::DepthCubeMapComponent(std::string sampler, std::string texFunc) : CubeMapComponent(
    "uniform float u_far;\n",
    sampler, texFunc,
    "vec4 bits = vec4(u_far,u_far/255.0,u_far/255.0/255.0,u_far/255.0/255.0/255.0);\n"
    "gl_FragColor = vec4(vec3(dot(bits,gl_FragColor)), 1.0);\n"
){
    
}

void DepthCubeMapComponent::doUniforms(){
    shader.uniform1f("u_far", farDepth);
}
