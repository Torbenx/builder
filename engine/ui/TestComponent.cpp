#include "TestComponent.hpp"

TestComponent::TestComponent() : shader(Shaders::color()), coords(4) {}

void TestComponent::draw(Framebuffer& framebuffer){
    shader->use();
    shader->attribPointer(shader->positionAttrib, coords);
    shader->uniformVec4(shader->colorUniform, color);
    framebuffer.draw(DrawMode::TriangleStrip, 4);
}

void TestComponent::updateRect(){
    this->coords.update(rect.corners());
}
