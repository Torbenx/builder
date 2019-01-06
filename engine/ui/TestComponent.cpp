#include "TestComponent.hpp"

TestComponent::TestComponent(Vec3 color) : 
shader(
"attribute vec4 a_position;\n"
"uniform mat4 u_transform;\n"
"void main(){\n"
"   gl_Position = u_transform * a_position;\n"
"}",
"uniform vec3 u_color;\n"
"void main(){\n"
" gl_FragColor = vec4(u_color, 1.0);\n"
"}"){
    this->color = color;
    shader.init();
    coords[0] = 0.0f;
    coords[1] = 0.0f;
    
    coords[2] = 1.0f;
    coords[3] = 0.0f;
    
    coords[4] = 0.0f;
    coords[5] = 1.0f;
    
    coords[6] = 1.0f;
    coords[7] = 1.0f;
}

void TestComponent::setTransform(Mat4x4 transform){
    this->transform = transform;
}

void TestComponent::draw(){
    shader.use();
    shader.attribPointer("a_position", 2, coords);
    shader.uniformMat4x4("u_transform", transform);
    shader.uniformVec3("u_color", color);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

Boundary TestComponent::getBoundary(){
    Boundary boundary = {Vec2(0.0f, 1.0f), Vec2(1.0f, 0.0f)};
    return boundary;
}