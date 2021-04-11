#include "ui/TestComponent.hpp"

TestComponent::TestComponent() : shader(Shaders::color()), coords(VertexBuffer::alloc<Vec2>(4)) {}

void TestComponent::draw(Framebuffer& framebuffer){
    shader->use();
    shader->attribPointer(shader->positionAttrib, coords.value);
    shader->uniformVec4(shader->colorUniform, color);
    framebuffer.draw(DrawMode::TriangleStrip, 4);
}

void TestComponent::updateRect(ScreenMetrics&){
    coords.value.update(rect.corners());
}
