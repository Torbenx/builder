#include "ui/TestComponent.hpp"

TestComponent::TestComponent()
    : shader(Shaders::color()) { }

void TestComponent::draw(Framebuffer& framebuffer, Context<GAllocator*> allocator) {
    allocator.forward();
    shader->use();
    shader->attribPointer(shader->positionAttrib, coords.vertexBuffer());
    shader->uniformVec4(shader->colorUniform, color);
    framebuffer.draw(DrawMode::TriangleStrip, 4);
}

void TestComponent::updateRect(ScreenMetrics&, Context<GAllocator*> allocator) {
    allocator.forward();
    coords.staticUpdate(rect.corners());
}
