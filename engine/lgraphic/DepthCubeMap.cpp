#include "lgraphic/DepthCubeMap.hpp"

DepthCubeMap::DepthCubeMap(int width, int height, float _far) :
    depthAttachment(Texture::depth(width, height, 16, true).move()),
    texture(CubeMap::color(width, height, Texture::Format::RGBA8888).move()),
    m_far(_far)
{
    shader = Shaders::depth();
    framebuffer.attach(depthAttachment);
    framebuffer.clearColor(1, 1, 1, 1);
    framebuffer.viewport(width, height);
}

void DepthCubeMap::draw(Mat4x4 model, Vec3 position, Renderable **ptrArray, int count){
    GL_VERIFY(glIsEnabled(GL_DEPTH_TEST));
    framebuffer.bind();
    shader->use();
    MvpMatrix mvpMatrix;
    mvpMatrix.projection = Mat4x4::frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, m_far);
    mvpMatrix.model = model;
    shader->uniformVec3(shader->lightPosUniform, position);
    shader->uniform1f(shader->farUniform, m_far);
    for(auto face : CubeMap::Faces){
        mvpMatrix.view = Mat4x4::Id;
        switch(face){
            case CubeMap::PosX:
                mvpMatrix.view = Mat4x4::look(position, Vec3(+1,0,0), Vec3(0,-1,0));
                break;
            case CubeMap::NegX:
                mvpMatrix.view = Mat4x4::look(position, Vec3(-1,0,0), Vec3(0,-1,0));
                break;
            case CubeMap::PosY:
                mvpMatrix.view = Mat4x4::look(position, Vec3(0,+1,0), Vec3(0,0,+1));
                break;
            case CubeMap::NegY:
                mvpMatrix.view = Mat4x4::look(position, Vec3(0,-1,0), Vec3(0,0,-1));
                break;
            case CubeMap::PosZ:
                mvpMatrix.view = Mat4x4::look(position, Vec3(0,0,+1), Vec3(0,-1,0));
                break;
            case CubeMap::NegZ:
                mvpMatrix.view = Mat4x4::look(position, Vec3(0,0,-1), Vec3(0,-1,0));
                break;
        }
        shader->uniformMat4x4(shader->mvpUniform, mvpMatrix.setupMvp());
        framebuffer.attach(texture, face);
        framebuffer.clear();
        for(int i=0;i<count;i++){
            ptrArray[i]->draw(framebuffer, *shader, shader->positionAttrib, shader->modelUniform);
        }
    }
}
