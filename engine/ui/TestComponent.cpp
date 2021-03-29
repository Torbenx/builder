#include "TestComponent.hpp"

TestComponent::TestComponent(){
    shader = Shaders::color();
}

void TestComponent::draw(){
    shader->use();
    shader->attribPointer(shader->positionAttrib, 2, coords);
    shader->uniformVec4(shader->colorUniform, color);
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
