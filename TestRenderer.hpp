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
#include "ui/TestComponent.hpp"
#include "ui/DepthCubeMapComponent.hpp"
#include "3d/Scene.hpp"
#include <cmath>

class DebugRenderable : public SceneObject{
    public:
        Bufferf *source;
        Vec3 color;
        Mat4x4 model;
        int num;
        GLenum mode = GL_TRIANGLES;
        
        DebugRenderable(Bufferf *s, Vec3 c, int n, Mat4x4 m=Mat4x4::Id){
            source = s;
            color = c;
            num = n;
            model = m;
        }
        
        ColorSources getColorSources(){
            ColorSources colors;
            colors.nUniforms = 1;
            return colors;
        }        
        
        void draw(Shader *shader, std::string positionAttrib, std::string modelUniform){
            shader->uniformMat4x4(modelUniform, model);
            shader->attribPointer(positionAttrib, 3, *source);
            glDrawArrays(mode, 0, num);
        }
        
        void draw2(LShader *shader, SceneDrawData& data){
            shader->uniformMat4x4(data.modelUniform.name, model);
            shader->uniformVec3(data.uniforms[0].name, color);
            shader->attribPointer(data.positionAttrib.name, 3, *source);
            glDrawArrays(mode, 0, num);
        }
};

class ReflectionDebugRenderable : public DebugRenderable{
    public:
        Reflection *reflection;
        
        ReflectionDebugRenderable(Bufferf *s, Vec3 c, int n, Reflection *r, Mat4x4 m=Mat4x4::Id) : DebugRenderable(s, c, n, m){
            reflection = r;
        }
        
        ColorSources getColorSources(){
            ColorSources colors = DebugRenderable::getColorSources();
            colors.nReflections = 1;
            return colors;
        }
        
        Reflection** getReflections(){
            return &reflection;
        }
        
        void draw2(LShader *shader, SceneDrawData& data){
            shader->reflection(*reflection);
            DebugRenderable::draw2(shader, data);
        }
};

class TestRenderer : public Renderer{
    public:
        DebugRenderable* obj1;
        DebugRenderable* obj2;
        ReflectionDebugRenderable* obj3;
        DebugRenderable* obj4;
        DebugRenderable* obj5;
        DebugRenderable* pawn;
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
        Bufferf pawnVertex;
        MvpMatrix mvpMatrix;
        Reflection reflection;
        Look3d look;
        TextComponent textComp;
        uint32_t w = -1,a = -1,s = -1,d = -1, fullscreen = -1, exitKey = -1;
        Vec3 pos;
        bool wd = false, ad = false, sd = false, dd = false;
        int lookPointerId = POINTER_ID_INVALID, navPointerId = POINTER_ID_INVALID;
        int navBaseX, navBaseY;
        int navDeltaX = 0, navDeltaY = 0;
        DepthCubeMapComponent cubeComp;
        TestComponent rComp, gComp, bComp;
        TextureComponent refComp;
        Scene scene;
        
        TestRenderer();
        void create();
        void change(int width, int height);
        void pointer(PointerEvent event);
        void rawPointer(int dx, int dy);
        void key(uint32_t keycode, bool down);
        void drawFrame();
        void destroy();
};

#endif