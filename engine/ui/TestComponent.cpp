#include "TestComponent.hpp"

TestComponent::TestComponent(Vec3 color) : 
shader(
"attribute vec4 a_position;\n"
"void main(){\n"
"   gl_Position = a_position;\n"
"}",
"uniform vec3 u_color;\n"
"void main(){\n"
" gl_FragColor = vec4(u_color, 1.0);\n"
"}"){
    this->color = color;
    shader.init();
}

void TestComponent::draw(){
    shader.use();
    shader.attribPointer("a_position", 2, coords);
    shader.uniformVec3("u_color", color);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void TestComponent::updateRect(){
    coords[0] = rect.getLeft();
    coords[1] = rect.getBottom();
    
    coords[2] = rect.getRight();
    coords[3] = rect.getBottom();
    
    coords[4] = rect.getLeft();
    coords[5] = rect.getTop();
    
    coords[6] = rect.getRight();
    coords[7] = rect.getTop();
}