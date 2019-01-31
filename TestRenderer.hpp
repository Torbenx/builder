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
#include "input/Look3d.hpp"
#include "ui/text.hpp"
#include "ui/TextComponent.hpp"
#include <cmath>

class DebugRenderable : public Renderable{
    public:
        Bufferf *source;
        Vec3 color;
        DebugRenderable(Bufferf *s,Vec3 c){
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
            shader->uniformVec3(colorUnifrom, color);
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
            model = Mat4x4::diagonal(Vec3(1,1,1)*0.2f) * Mat4x4::rotate(-M_PI/2.0, Vec3(1,0,0)) * Mat4x4::translate(Vec3(-16,-11,0));
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
        uint64_t lframe;
        uint64_t lsecond;
        int frameCounter;
        float obj5_coords[4*3];
        Vec3 up;
        PointLight pointLight;
        SpotLight spotLight;
        AmbientLight ambientLight;
        Bufferf f1;
        Bufferf f2;
        Bufferf f3;
        Bufferf f4;
        Bufferf f5;
        Bufferf d_coord;
        Bufferf d_texCoord;
        Bufferf tex_Coords;
        Bufferf tex_Position;
        Bufferf pawnVertex;
        MvpMatrix mvpMatrix;
        Shader depthShader;
        Shader texShader;
        Reflection reflection;
        Look3d look;
        TextComponent textComp;
        int32_t w = -1,a = -1,s = -1,d = -1;
        Vec3 pos;
        bool wd = false, ad = false, sd = false, dd = false;
        int lookPointerId = POINTER_ID_INVALID, navPointerId = POINTER_ID_INVALID;
        int navBaseX, navBaseY;
        int navDeltaX = 0, navDeltaY = 0;
        TextureComponent refTextureComp;
        
        TestRenderer();
        void create();
        void change(int width, int height);
        void pointer(PointerEvent event);
        void rawPointer(int dx, int dy);
        void key(int32_t keycode, bool down);
        void drawFrame();
        void destroy();
};

#endif