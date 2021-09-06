#include "ui/TestComponent.hpp"

TestComponent::TestComponent()
    : shader(Shaders::color()) { }

void TestComponent::draw(Framebuffer& framebuffer, GAllocator& allocator) {
    shader->use();
    shader->attribPointer(shader->positionAttrib, coords.vertexBuffer(allocator));
    shader->uniformVec4(shader->colorUniform, color);
    framebuffer.draw(DrawMode::TriangleStrip, 4);
}

void TestComponent::updateRect(ScreenMetrics&, GAllocator& allocator) {
    coords.staticUpdate(allocator) = rect.corners();
}

void TestComponent::detach(GAllocator& allocator) {
    coords.release(allocator);
}
