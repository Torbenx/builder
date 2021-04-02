#include "ui/CubeMapComponent.hpp"

CubeMapComponent::CubeMapComponent(bool createShader) : coords(4), texCoords(4){
    if(createShader)
        shader = Shaders::cubeMap();
}

void CubeMapComponent::draw(Framebuffer& framebuffer){
    shader->use();
    shader->attribPointer(shader->positionAttrib, coords);
    shader->attribPointer(shader->texCoordAttrib, texCoords);
    shader->uniformCubeMap(shader->cubeMapUniform, *cubeMap, 0);
    doUniforms();
    framebuffer.draw(DrawMode::TriangleStrip, 4);
}

void CubeMapComponent::updateRect(ScreenMetrics&){
    coords.update(rect.corners());
}

void CubeMapComponent::positiveX(Rect rect){
    texCoords.update({
        Vec3(1, rect.left, rect.bottom),
        Vec3(1, rect.right, rect.bottom),
        Vec3(1, rect.left, rect.top),
        Vec3(1, rect.right, rect.top)
    });
}

void CubeMapComponent::negativeX(Rect rect){
    texCoords.update({
        Vec3(-1, rect.left, rect.bottom),
        Vec3(-1, rect.right, rect.bottom),
        Vec3(-1, rect.left, rect.top),
        Vec3(-1, rect.right, rect.top)
    });
}

void CubeMapComponent::positiveY(Rect rect){
    texCoords.update({
        Vec3(rect.left, 1, rect.bottom),
        Vec3(rect.right, 1, rect.bottom),
        Vec3(rect.left, 1, rect.top),
        Vec3(rect.right, 1, rect.top)
    });
}

void CubeMapComponent::negativeY(Rect rect){
    texCoords.update({
        Vec3(rect.left, -1, rect.bottom),
        Vec3(rect.right, -1, rect.bottom),
        Vec3(rect.left, -1, rect.top),
        Vec3(rect.right, -1, rect.top)
    });
}

void CubeMapComponent::positiveZ(Rect rect){
    texCoords.update({
        Vec3(rect.left, rect.bottom, 1),
        Vec3(rect.right, rect.bottom, 1),
        Vec3(rect.left, rect.top, 1),
        Vec3(rect.right, rect.top, 1)
    });
}

void CubeMapComponent::negativeZ(Rect rect){
    texCoords.update({
        Vec3(rect.left, rect.bottom, -1),
        Vec3(rect.right, rect.bottom, -1),
        Vec3(rect.left, rect.top, -1),
        Vec3(rect.right, rect.top, -1)
    });
}
