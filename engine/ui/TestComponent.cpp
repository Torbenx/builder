#include "ui/TestComponent.hpp"

TestComponent::TestComponent()
    : shader(Shaders::color()) { }

void TestComponent::draw(Framebuffer& framebuffer, GAllocator& allocator) {
    shader->use();
    shader->attribPointer(shader->positionAttrib, coords(allocator));
    shader->uniformVec4(shader->colorUniform, color);
    framebuffer.draw(DrawMode::TriangleStrip, 4);
}

void TestComponent::updateRect(ScreenMetrics&, GAllocator& allocator) {
    //coords.value.update(rect.corners());
    coords.update(allocator) = rect.corners();
}

void TestComponent::detach(GAllocator& allocator) {
    coords.release(allocator);
}
