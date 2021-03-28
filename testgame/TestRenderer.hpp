#ifndef HEADER_TESTRENDERER_H
#define HEADER_TESTRENDERER_H

#include "Renderer.hpp"
#include "gl/glwrapper.hpp"
#include "math/Vec2.hpp"
#include "math/Vec3.hpp"
#include "math/Mat4x4.hpp"
#include "graphic/Shader.hpp"
#include "lgraphic/DepthMap.hpp"
#include "input/Look3d.hpp"
#include "ui/text.hpp"
//#include "ui/TextComponent.hpp"
//#include "ui/TestComponent.hpp"
//#include "ui/DepthCubeMapComponent.hpp"
//#include "ui/DepthMapComponent.hpp"
#include "3d/Scene.hpp"
#include "ui/UiScene.hpp"
#include <cmath>

class DebugRenderable : public SceneObject{
    public:
        Bufferf *source;
        Bufferf *normals;
        Vec3 color;
        Mat4x4 model;
        int num;
        GLenum mode = GL_TRIANGLES;

        DebugRenderable(Bufferf *s, Bufferf *nor, Vec3 c, int n, Mat4x4 m=Mat4x4::Id){
            source = s;
            normals = nor;
            color = c;
            num = n;
            model = m;
        }

        ColorSources getColorSources(){
            ColorSources colors;
            colors.nUniforms = 1;
            return colors;
        }

        void draw(Shader& shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform){
            shader.uniformMat4x4(modelUniform, model);
            shader.attribPointer(positionAttrib, 3, *source);
            glDrawArrays(mode, 0, num);
        }

        void draw2(LShader& shader, const SceneDrawData& data){
            shader.uniformMat4x4(data.modelUniform, model);
            shader.uniformVec3(data.uniforms[0], color);
            shader.attribPointer(data.positionAttrib, 3, *source);
            shader.attribPointer(data.normalAttrib, 3, *normals);
            glDrawArrays(mode, 0, num);
        }
};

class ObjectRenderable : public SceneObject{
    public:
        Bufferf *source;
        Bufferf *texCoords;
        Bufferf *normals;
        Bufferi *indices;
        Texture *texture;
        Mat4x4 model;
        int num;
        GLenum mode = GL_TRIANGLES;

        ObjectRenderable(Bufferf *s, Bufferi *i, Bufferf *t, Bufferf *nor, Texture *tex, int n, Mat4x4 m=Mat4x4::Id){
            source = s;
            indices = i;
            texCoords = t;
            normals = nor;
            texture = tex;
            num = n;
            model = m;
        }

        ColorSources getColorSources(){
            ColorSources colors;
            colors.nTextures = 1;
            return colors;
        }

        std::string* getTextureTypes(){
            static std::string r[] = {"2D"};
            return r;
        }

        void draw(Shader& shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform){
            shader.uniformMat4x4(modelUniform, model);
            shader.attribPointer(positionAttrib, 3, *source);
            if(indices){
                indices->bind();
                glDrawElements(mode, num, GL_UNSIGNED_INT, 0);
            }else{
                glDrawArrays(mode, 0, num);
            }
        }

        void draw2(LShader& shader, const SceneDrawData& data){
            shader.uniformMat4x4(data.modelUniform, model);
            shader.attribPointer(data.positionAttrib, 3, *source);
            shader.attribPointer(data.normalAttrib, 3, *normals);
            shader.uniformTexture(data.textureUniforms[0], *texture, 10);
            shader.attribPointer(data.texPositionAttribs[0], 2, *texCoords);
            if(indices){
                indices->bind();
                glDrawElements(mode, num, GL_UNSIGNED_INT, 0);
            }else{
                glDrawArrays(mode, 0, num);
            }
        }
};

class ReflectionDebugRenderable : public DebugRenderable{
    public:
        Reflection *reflection;

        ReflectionDebugRenderable(Bufferf *s, Bufferf *nor, Vec3 c, int n, Reflection *r, Mat4x4 m=Mat4x4::Id) : DebugRenderable(s, nor, c, n, m){
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
};

class TestRenderer : public Renderer{
    public:
        DebugRenderable* obj1;
        DebugRenderable* obj2;
        ReflectionDebugRenderable* obj3;
        DebugRenderable* obj4;
        DebugRenderable* obj5;
        ObjectRenderable* pawn;
        uint64_t lframe;
        uint64_t lsecond;
        int frameCounter = 0;
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
        Bufferf fNormal;
        Bufferf pawnVertex;
        Bufferi pawnIndices;
        Bufferf pawnTexCoords;
        Bufferf pawnNormals;
        Texture pawnTexture;
        MvpMatrix mvpMatrix;
        Reflection reflection;
        Look3d look;
        uint32_t w = -1,a = -1,s = -1,d = -1, fullscreen = -1, exitKey = -1, upKey = -1, downKey = -1;
        Vec3 pos;
        bool wd = false, ad = false, sd = false, dd = false, upKeyDown = false, downKeyDown = false;
        int lookPointerId = POINTER_ID_INVALID, navPointerId = POINTER_ID_INVALID;
        int navBaseX, navBaseY;
        int navDeltaX = 0, navDeltaY = 0;
        //TextComponent textComp;
        //DepthMapComponent cubeComp;
        //TextureComponent refComp;
        Scene scene;
        UiScene uiScene;

        TestRenderer();
        void create();
        void change(int width, int height);
        void pointer(PointerEvent event);
        void rawPointer(int dx, int dy);
        void rawKey(uint32_t keycode, bool down);
        void drawFrame();
        void destroy();
};

#endif
