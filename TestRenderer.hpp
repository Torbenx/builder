#ifndef HEADER_TESTRENDERER_H
#define HEADER_TESTRENDERER_H

#include "Renderer.hpp"
#include "gl/glwrapper.hpp"
#include "math/Vec2.hpp"
#include "math/Vec3.hpp"
#include "math/Mat4x4.hpp"
#include "graphic/Shader.hpp"
#include "lgraphic/DepthMap.hpp"
#include "lgraphic/ShaderBuilder.hpp"
#include <cmath>
#include <ctime>

class DebugRenderable : public Renderable{
    public:
        Bufferf *source;
        Vec4 color;
        DebugRenderable(Bufferf *s,Vec4 c){
            source = s;
            color = c;
        }
        void draw(
            Shader *shader,
            std::string positionAttrib,
            std::string modelUniform
        ){
            shader->uniformMat4x4(modelUniform, Mat4x4::Id);
            shader->attribPointer(positionAttrib,3,*source);
            glDrawArrays(GL_TRIANGLE_STRIP,0,4);
        }
        
        void draw2(
            Shader *shader,
            std::string positionAttrib,
            std::string colorUnifrom
        ){
            shader->uniformVec3(colorUnifrom, color.xyz());
            shader->attribPointer(positionAttrib,3,*source);
            glDrawArrays(GL_TRIANGLE_STRIP,0,4);
        }
};

class ObjRenderable : public Renderable{
    public:
        Bufferf* vertex;
        Vec3 color;
        Mat4x4 model;
        int num;
        
        ObjRenderable(Bufferf *v, Vec3 c, int n){
            vertex = v;
            color = c;
            num = n;
            model.setIdentity();
            model.setRotate(-M_PI/2.0, Vec3(1,0,0));
            model.translate(Vec3(-16,-11,0));
            model.scale(Vec3(1,1,1)*0.2f);
            //model.translateRaw(Vec3(0,0.5f,0));
        }
        void draw(Shader *shader, std::string positionAttrib, std::string modelUniform){
            shader->uniformMat4x4(modelUniform, model);
            shader->attribPointer(positionAttrib, 3, *vertex);
            glDrawArrays(GL_TRIANGLES, 0, num);
        }
        void draw2(Shader *shader, std::string positionAttrib, std::string modelUniform, std::string colorUniform){
            shader->uniformMat4x4(modelUniform, model);
            shader->uniformVec3(colorUniform, color);
            shader->attribPointer(positionAttrib, 3, *vertex);
            glDrawArrays(GL_TRIANGLES, 0, num);
        }
};

class TestRenderer : public Renderer{
    public:
        LShader* shader;
        LShader* refShader;
        DebugRenderable* obj1;
        DebugRenderable* obj2;
        DebugRenderable* obj3;
        DebugRenderable* obj4;
        DebugRenderable* obj5;
        ObjRenderable* pawn;
        clock_t lticks;
        float obj5_coords[4*3];
        Vec3 up;
        PointLighting pointLight;
        SpotLighting spotLight;
        AmbientLighting ambientLight;
        Bufferf f1;
        Bufferf f2;
        Bufferf f3;
        Bufferf f4;
        Bufferf f5;
        Bufferf d_coord;
        Bufferf d_texCoord;
        Bufferf tex_Coords;
        Bufferf pawnVertex;
        MvpMatrix mvpMatrix;
        Shader depthShader;
        Shader texShader;
        Reflection reflection;
        
        TestRenderer();
        void create();
        void change(int width, int height);
        void drawFrame();
        void destroy();
};

#endif