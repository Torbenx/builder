#include "ui/TestComponent.hpp"

TestComponent::TestComponent()
    : shader(Shaders::color()) { }

void TestComponent::draw(Framebuffer& framebuffer, [[context]] GAllocator* allocator) {
    shader->use();
    shader->attribPointer(shader->positionAttrib, coords.vertexBuffer());
    shader->uniformVec4(shader->colorUniform, color);
    framebuffer.draw(DrawMode::TriangleStrip, 4);
}

void TestComponent::updateRect(ScreenMetrics&, [[context]] GAllocator* allocator) {
    coords.staticUpdate(rect.corners());
}
