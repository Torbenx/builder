#include "ui/TestComponent.hpp"

TestComponent::TestComponent() : shader(Shaders::color()), coords(VertexBuffer::alloc<Vec2>(4).move()) {}

void TestComponent::draw(Framebuffer& framebuffer){
    shader->use();
    shader->attribPointer(shader->positionAttrib, coords);
    shader->uniformVec4(shader->colorUniform, color);
    framebuffer.draw(DrawMode::TriangleStrip, 4);
}

void TestComponent::updateRect(ScreenMetrics&){
    this->coords.update(rect.corners());
}
