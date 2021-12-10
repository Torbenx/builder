#include "ui/CubeMapComponent.hpp"

CubeMapComponent::CubeMapComponent(bool createShader) : coords(VertexBuffer::alloc<Vec2>(4)), texCoords(VertexBuffer::alloc<Vec3>(4)){
    if(createShader)
        shader = Shaders::cubeMap();
}

void CubeMapComponent::draw(Framebuffer& framebuffer, [[context]] GAllocator*){
    shader->use();
    shader->attribPointer(shader->positionAttrib, coords.value);
    shader->attribPointer(shader->texCoordAttrib, texCoords.value);
    shader->uniformCubeMap(shader->cubeMapUniform, *cubeMap);
    doUniforms();
    framebuffer.draw(DrawMode::TriangleStrip, 4);
}

void CubeMapComponent::updateRect(ScreenMetrics&, [[context]] GAllocator*) {
    coords.value.update(rect.corners());
}

void CubeMapComponent::positiveX(Rect rect){
    texCoords.value.update({
        Vec3(1, rect.left, rect.bottom),
        Vec3(1, rect.right, rect.bottom),
        Vec3(1, rect.left, rect.top),
        Vec3(1, rect.right, rect.top)
    });
}

void CubeMapComponent::negativeX(Rect rect){
    texCoords.value.update({
        Vec3(-1, rect.left, rect.bottom),
        Vec3(-1, rect.right, rect.bottom),
        Vec3(-1, rect.left, rect.top),
        Vec3(-1, rect.right, rect.top)
    });
}

void CubeMapComponent::positiveY(Rect rect){
    texCoords.value.update({
        Vec3(rect.left, 1, rect.bottom),
        Vec3(rect.right, 1, rect.bottom),
        Vec3(rect.left, 1, rect.top),
        Vec3(rect.right, 1, rect.top)
    });
}

void CubeMapComponent::negativeY(Rect rect){
    texCoords.value.update({
        Vec3(rect.left, -1, rect.bottom),
        Vec3(rect.right, -1, rect.bottom),
        Vec3(rect.left, -1, rect.top),
        Vec3(rect.right, -1, rect.top)
    });
}

void CubeMapComponent::positiveZ(Rect rect){
    texCoords.value.update({
        Vec3(rect.left, rect.bottom, 1),
        Vec3(rect.right, rect.bottom, 1),
        Vec3(rect.left, rect.top, 1),
        Vec3(rect.right, rect.top, 1)
    });
}

void CubeMapComponent::negativeZ(Rect rect){
    texCoords.value.update({
        Vec3(rect.left, rect.bottom, -1),
        Vec3(rect.right, rect.bottom, -1),
        Vec3(rect.left, rect.top, -1),
        Vec3(rect.right, rect.top, -1)
    });
}
