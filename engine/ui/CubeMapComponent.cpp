#include "CubeMapComponent.hpp"

CubeMapComponent::CubeMapComponent(bool createShader){
    if(createShader)
        shader = Shaders::cubeMap();
}

void CubeMapComponent::draw(){
    shader->use();
    shader->attribPointer(shader->positionAttrib, 2, coords);
    shader->attribPointer(shader->texCoordAttrib, 3, texCoords);
    shader->uniformCubeMap(shader->cubeMapUniform, *cubeMap, 0);
    doUniforms();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void CubeMapComponent::updateRect(){
    coords[0] = rect.getLeft();
    coords[1] = rect.getBottom();

    coords[2] = rect.getRight();
    coords[3] = rect.getBottom();

    coords[4] = rect.getLeft();
    coords[5] = rect.getTop();

    coords[6] = rect.getRight();
    coords[7] = rect.getTop();
}

void CubeMapComponent::positiveX(Rect rect){
    texCoords[ 0] = 1;
    texCoords[ 1] = rect.getLeft();
    texCoords[ 2] = rect.getBottom();

    texCoords[ 3] = 1;
    texCoords[ 4] = rect.getRight();
    texCoords[ 5] = rect.getBottom();

    texCoords[ 6] = 1;
    texCoords[ 7] = rect.getLeft();
    texCoords[ 8] = rect.getTop();

    texCoords[ 9] = 1;
    texCoords[10] = rect.getRight();
    texCoords[11] = rect.getTop();
}

void CubeMapComponent::negativeX(Rect rect){
    texCoords[ 0] = -1;
    texCoords[ 1] = rect.getLeft();
    texCoords[ 2] = rect.getBottom();

    texCoords[ 3] = -1;
    texCoords[ 4] = rect.getRight();
    texCoords[ 5] = rect.getBottom();

    texCoords[ 6] = -1;
    texCoords[ 7] = rect.getLeft();
    texCoords[ 8] = rect.getTop();

    texCoords[ 9] = -1;
    texCoords[10] = rect.getRight();
    texCoords[11] = rect.getTop();
}

void CubeMapComponent::positiveY(Rect rect){
    texCoords[ 0] = rect.getLeft();
    texCoords[ 1] = 1;
    texCoords[ 2] = rect.getBottom();

    texCoords[ 3] = rect.getRight();
    texCoords[ 4] = 1;
    texCoords[ 5] = rect.getBottom();

    texCoords[ 6] = rect.getLeft();
    texCoords[ 7] = 1;
    texCoords[ 8] = rect.getTop();

    texCoords[ 9] = rect.getRight();
    texCoords[10] = 1;
    texCoords[11] = rect.getTop();
}

void CubeMapComponent::negativeY(Rect rect){
    texCoords[ 0] = rect.getLeft();
    texCoords[ 1] = -1;
    texCoords[ 2] = rect.getBottom();

    texCoords[ 3] = rect.getRight();
    texCoords[ 4] = -1;
    texCoords[ 5] = rect.getBottom();

    texCoords[ 6] = rect.getLeft();
    texCoords[ 7] = -1;
    texCoords[ 8] = rect.getTop();

    texCoords[ 9] = rect.getRight();
    texCoords[10] = -1;
    texCoords[11] = rect.getTop();
}

void CubeMapComponent::positiveZ(Rect rect){
    texCoords[ 0] = rect.getLeft();
    texCoords[ 1] = rect.getBottom();
    texCoords[ 2] = 1;

    texCoords[ 3] = rect.getRight();
    texCoords[ 4] = rect.getBottom();
    texCoords[ 5] = 1;

    texCoords[ 6] = rect.getLeft();
    texCoords[ 7] = rect.getTop();
    texCoords[ 8] = 1;

    texCoords[ 9] = rect.getRight();
    texCoords[10] = rect.getTop();
    texCoords[11] = 1;
}

void CubeMapComponent::negativeZ(Rect rect){
    texCoords[ 0] = rect.getLeft();
    texCoords[ 1] = rect.getBottom();
    texCoords[ 2] = -1;

    texCoords[ 3] = rect.getRight();
    texCoords[ 4] = rect.getBottom();
    texCoords[ 5] = -1;

    texCoords[ 6] = rect.getLeft();
    texCoords[ 7] = rect.getTop();
    texCoords[ 8] = -1;

    texCoords[ 9] = rect.getRight();
    texCoords[10] = rect.getTop();
    texCoords[11] = -1;
}
