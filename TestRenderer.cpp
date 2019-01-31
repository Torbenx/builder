#include "Renderer.hpp"
#include "util/log.hpp"
#include <string>
#include "gl/glLog.hpp"
#include "resource/ObjParser.hpp"
#include "resource/asset_istream.hpp"
#include <fstream>
#include "TestRenderer.hpp"
#include "util/time.hpp"
#include "util/target.hpp"
#include "fullscreen.hpp"
#include "ui/TestComponent.hpp"
#include "util/draw_util.hpp"

#ifdef PLATFORM_WINDOWS
#include "windows/fullscreen.hpp"
#endif

TestRenderer::TestRenderer() :
        up(0,1,0),
        pointLight(
            Vec3(0,1,-2),
            1024,1024,
            Vec3(1,1,1),
            1000,
            0.3f,
            0.2f
        ),
        spotLight(
            Vec3(0,3.5,0),
            Vec3(0.1,-3,0.1),
            M_PI*0.4,
            M_PI*0.4,
            2048,2048,
            Vec3(1,1,1)*0.4f,
            10
        ),
        ambientLight(Vec3(1, 1, 1)*0.2f),
        f1(GL_ARRAY_BUFFER),
        f2(GL_ARRAY_BUFFER),
        f3(GL_ARRAY_BUFFER),
        f4(GL_ARRAY_BUFFER),
        f5(TE_BUFFER),
        d_coord(GL_ARRAY_BUFFER),
        d_texCoord(GL_ARRAY_BUFFER),
        tex_Coords(GL_ARRAY_BUFFER),
        tex_Position(GL_ARRAY_BUFFER),
        pawnVertex(GL_ARRAY_BUFFER),
        depthShader(
            "attribute vec4 a_position;\n"
            "attribute vec3 a_texCoord;\n"
            "varying vec3 v_texCoord;\n"
            "void main(){\n"
            "    gl_Position = a_position;\n"
            "    v_texCoord = a_texCoord;\n"
            "}",
            "uniform samplerCube u_texture;\n"
            "varying vec3 v_texCoord;\n"
            "void main(){\n"
            "    float far = 1000.0;\n"
            "    vec4 bits = vec4(far,far/255.0,far/255.0/255.0,far/255.0/255.0/255.0);\n"
            "    vec4 f = textureCube(u_texture,v_texCoord);\n"
            "    gl_FragColor = vec4(vec3(dot(bits,f))/15.0,1.0);\n"
            "}"),
        texShader(
            "attribute vec4 a_position;\n"
            "attribute vec2 a_texCoord;\n"
            "varying vec2 v_texPos;\n"
            "void main(){\n"
            "    gl_Position = a_position;\n"
            "    v_texPos = a_texCoord;\n"
            "}",
            TEXT_TEXTURE_GLSL_HEADER
            "varying vec2 v_texPos;\n"
            "uniform " TEXT_TEXTURE_GLSL_SAMPLER " u_texture;\n"
            "void main(){\n"
            "    float far = 20.0;\n"
            "    vec4 bits = vec4(far,far/255.0,far/255.0/255.0,far/255.0/255.0/255.0);\n"
            "    vec4 f = texture2D(u_texture, v_texPos);\n"
            "    gl_FragColor = f;\n"//vec4(vec3(dot(bits,f))/far,1.0);\n"
            "}"),
        reflection(Vec3(0,1,0),Vec3(0,-2,0),2048,2048,0.5f),
        look(up, isAndroid()),
        pos(7,1,0)
        {}

void TestRenderer::create(){
    pointLight.init();
    spotLight.init();
    reflection.init();
    reflection.framebuffer.setClearColor(0,0,0,1);
    Framebuffer::DEFAULT.setClearColor(0,0,0,1);
    f1.init();
    float f1Src[] = {-1,-0,-1,1,-0,-1,-1,-0,1,1,-0,1};
    f1.bufferData(f1Src,3*4*4,GL_STATIC_DRAW);
    f2.init();
    float f2Src[] = {-1,3,0,1,3,0,-1,3,2,1,3,2};
    f2.bufferData(f2Src,3*4*4,GL_STATIC_DRAW);
    f3.init();
    float f3Src[] = {-10,-2,-10,-10,-2,10,10,-2,-10,10,-2,10};
    f3.bufferData(f3Src,3*4*4,GL_STATIC_DRAW);
    f4.init();
    float f4Src[] = {-10,4,-10,-10,4,10,10,4,-10,10,4,10};
    f4.bufferData(f4Src,3*4*4,GL_STATIC_DRAW);
    f5.init();
    f5.bufferData(obj5_coords,3*4*4,GL_DYNAMIC_DRAW);
    tex_Coords.init();
    float tex_CoordsSrc[] = {0,1, 1,1, 0,0, 1,0};
    tex_Coords.bufferData(tex_CoordsSrc,3*4*4,GL_STATIC_DRAW);
    tex_Position.init();
    d_coord.init();
    float x1 = -1;
    float x2 = 0;
    float y1 = -1;
    float add = 1.0f/3.0f;
    float y2 = -1+add;
    float d_coordSrc[] = {
         x1,y1,x2,y1,x1,y2, x2,y1,x1,y2,x2,y2,
         x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
         x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
         x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
         x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
         x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2
    };
    d_coord.bufferData(d_coordSrc,6*2*3*2*4,GL_STATIC_DRAW);
    d_texCoord.init();
    float d_texCoordSrc[] = {
         -1,-1,-1, 1,-1,-1, -1,1,-1,    1,-1,-1, -1,1,-1, 1,1,-1, //negZ
         -1,-1,1, 1,-1,1, -1,1,1,       1,-1,1, -1,1,1, 1,1,1, //posZ
         -1,-1,-1, 1,-1,-1, -1,-1,1,    1,-1,-1, -1,-1,1, 1,-1,1, //negY
         -1,1,-1, 1,1,-1, -1,1,1,       1,1,-1, -1,1,1, 1,1,1, //posY
         -1,-1,-1, -1,1,-1, -1,-1,1,    -1,1,-1, -1,-1,1, -1,1,1, //negX
         1,-1,-1, 1,1,-1, 1,-1,1,       1,1,-1, 1,-1,1, 1,1,1, //posX
    };
    d_texCoord.bufferData(d_texCoordSrc, 6*2*3*3*4, GL_STATIC_DRAW);
    obj1 = new DebugRenderable(&f1,Vec3(1,0,0));
    obj2 = new DebugRenderable(&f2,Vec3(0,1,0));
    obj3 = new DebugRenderable(&f3,Vec3(0.5,0.5,0.5));
    obj4 = new DebugRenderable(&f4,Vec3(1,1,0));
    obj5 = new DebugRenderable(&f5,Vec3(0,0,1));
    mvpMatrix.model = Mat4x4::Id;
    ShaderBuilder b;
    b.setVertexPositionAttrib("a_position");
    b.setFragmentColorUniform("u_color");
    b.setScreenMvpMatrixUniform("u_mvpMatrix");
    b.setWorldModelMatrixUniform("u_model");
    b.addLight(ambientLight);
    b.addLight(pointLight);
    b.addLight(spotLight);
    shader = b.build();
    ShaderBuilder rb(b);
    rb.addReflection(reflection);
    refShader = rb.build();
    shader->init();
    refShader->init();
    ObjParser parser;
    asset_istream file("Pawn/Pawn.obj");
    parser.parse(file, 20000, 20000);
    pawnVertex.init();
    pawnVertex.bufferData(parser.vertex,parser.vertexSize*4,GL_STATIC_DRAW);
    pawn = new ObjRenderable(&pawnVertex, Vec3(0,1,1), parser.n);
    look.dir = Vec3(0,-1,0) - Vec3(7,1,0);
    lframe = getNanos();
    
    #ifdef PLATFORM_WINDOWS
    //goFullscreen(getFullscreenConfig(getDisplayList().front(), 1920, 1080, 144));
    #else
    goFullscreen(FullscreenConfig::DEFAULT);
    #endif
    
    textComp.text = "Hello World";
    textComp.textSize = 0.1f;
    textComp.textColor = Vec4(1, 0, 0, 1);
    textComp.rect = Rect::bltr(-1, -1, 0, -0.5f);
    
    refTextureComp.texture = &reflection.texture;
    refTextureComp.rect = Rect::bltr(-0.5f, -0.5f, 0.5f, 0.5f);
    refTextureComp.updateRect();
}

void TestRenderer::change(int newWidth,int newHeight){
    Framebuffer::DEFAULT.viewport(newWidth,newHeight);
    float as = (float)newWidth/(float)newHeight;
    mvpMatrix.projection = Mat4x4::frustumH(67.0 * M_PI / 180.0 ,as ,1,100);
    textComp.screenWidth = newWidth;
    textComp.screenHeight = newHeight;
    textComp.updateRect();
}

void TestRenderer::pointer(PointerEvent event){
    if(!rawInput()){
        if(event.event == POINTER_EVENT_DOWN && navPointerId == POINTER_ID_INVALID){
            navPointerId = event.pointerId;
            navBaseX = event.x;
            navBaseY = event.y;
        }else if(event.event == POINTER_EVENT_DOWN && lookPointerId == POINTER_ID_INVALID){
            lookPointerId = event.pointerId;
        }else if(event.pointerId == navPointerId){
            if(event.event == POINTER_EVENT_UP){
                navDeltaX = 0;
                navDeltaY = 0;
                navPointerId = POINTER_ID_INVALID;
            }else if(event.event == POINTER_EVENT_MOVE){
                navDeltaX = event.x - navBaseX;
                navDeltaY = event.y - navBaseY;
            }
        }else if(event.pointerId == lookPointerId){
            if(event.event == POINTER_EVENT_UP){
                lookPointerId = POINTER_ID_INVALID;
            }else if(event.event == POINTER_EVENT_MOVE){
                Pointer oldPointer = getPointerById(lookPointerId);
                look.move(event.x - oldPointer.x, event.y - oldPointer.y);
            }
        }
    }
    trackPointerEvent(event);
}

void TestRenderer::rawPointer(int dx, int dy){
    look.move(dx, dy);
}

void TestRenderer::key(int32_t keycode, bool down){
    if(w == -1){
        w = keycode;
        LOGD("w=0x%x", keycode);
    }else if(keycode == w){
        wd = down;
    }else if(a == -1){
        a = keycode;
        LOGD("a=0x%x", keycode);
    }else if(keycode == a){
        ad = down;
    }else if(s == -1){
        s = keycode;
        LOGD("s=0x%x", keycode);
    }else if(keycode == s){
        sd = down;
    }else if(d == -1){
        d = keycode;
        LOGD("d=0x%x", keycode);
    }else if(keycode == d){
        dd = down;
    }
}

void TestRenderer::drawFrame(){
    uint64_t now = getNanos();
    if(now - lsecond >= NANOS_PER_SECOND){
        LOGD("FPS: %d",frameCounter);
        lsecond = now;
        frameCounter = 0;
    }
    frameCounter++;
    
    
    float maxSpeed = 2.0f;
    float dt = (now - lframe)/(float)NANOS_PER_SECOND;
    Vec3 dirX = (look.dir - Vec3::dot(look.dir, up) * up).normalize();
    if(wd){
        pos += maxSpeed * dt * dirX;
    }
    if(sd){
        pos -= maxSpeed * dt * dirX;
    }
    if(ad){
        pos -= maxSpeed * dt * Vec3::cross(dirX, up);
    }
    if(dd){
        pos += maxSpeed * dt * Vec3::cross(dirX, up);
    }
    float navMult = 0.007f;
    Vec2 navD = Vec2(navDeltaX, navDeltaY) * navMult;
    float navM = navD.length();
    if(navM >= maxSpeed){
        navD *= maxSpeed / navM;
    }
    pos += navD.x * dt * Vec3::cross(dirX, up);
    pos -= navD.y * dt * dirX;
    mvpMatrix.view = Mat4x4::look(pos, look.dir, up);
    lframe = now;
    Vec3 plp = pointLight.position;
    plp += Vec3(plp.z*dt, plp.z*dt/2.0, -plp.x*dt);
    pointLight.position = plp;
    obj5_coords[0]  = plp.x - 0.3f;
    obj5_coords[1]  = plp.y;
    obj5_coords[2]  = plp.z - 0.3f;
    
    obj5_coords[3]  = plp.x + 0.3f;
    obj5_coords[4]  = plp.y;
    obj5_coords[5]  = plp.z - 0.3f;
    
    obj5_coords[6]  = plp.x - 0.3f;
    obj5_coords[7]  = plp.y;
    obj5_coords[8]  = plp.z + 0.3f;
    
    obj5_coords[9]  = plp.x + 0.3f;
    obj5_coords[10] = plp.y;
    obj5_coords[11] = plp.z + 0.3f;
    //mvpMatrix.view.setLookAt(Vec3(plp.x*2.0f,1,plp.z*2.0f),Vec3(0,-1,0),up);
    glEnable(GL_DEPTH_TEST);
    Framebuffer::DEFAULT.bind();
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    Renderable *ptrs[] = {obj1,obj2,obj3,obj4,pawn};
    pointLight.draw(mvpMatrix.model,ptrs,5);
    spotLight.draw(mvpMatrix.model,ptrs,5);
    shader->use();
    shader->light(pointLight,mvpMatrix.model);
    shader->light(spotLight,mvpMatrix.model);
    shader->light(ambientLight);
    reflection.bind();
    reflection.getReflectionModel(mvpMatrix.model);
    shader->uniformMat4x4("u_mvpMatrix",mvpMatrix.setupMvp());
    shader->uniformMat4x4("u_model",Mat4x4::Id);
    obj1->draw2(shader,"a_position","u_color");
    obj2->draw2(shader,"a_position","u_color");
    obj4->draw2(shader,"a_position","u_color");
    pawn->draw2(shader,"a_position","u_model","u_color");
    reflection.unbind();
    mvpMatrix.model = Mat4x4::Id;
    shader->uniformMat4x4("u_mvpMatrix",mvpMatrix.setupMvp());
    shader->uniformMat4x4("u_model",Mat4x4::Id);
    obj1->draw2(shader,"a_position","u_color");
    obj2->draw2(shader,"a_position","u_color");
    obj4->draw2(shader,"a_position","u_color");
    obj5->draw2(shader,"a_position","u_color");
    pawn->draw2(shader,"a_position","u_model","u_color");
    refShader->use();
    refShader->light(ambientLight);
    refShader->reflection(reflection);
    refShader->light(pointLight,mvpMatrix.model);
    refShader->light(spotLight,mvpMatrix.model);
    refShader->uniformMat4x4("u_mvpMatrix",mvpMatrix.setupMvp());
    refShader->uniformMat4x4("u_model",Mat4x4::Id);
    obj3->draw2(refShader,"a_position","u_color");
    glDisable(GL_DEPTH_TEST);
    
    if(navPointerId != POINTER_ID_INVALID){
        Vec2 center(navBaseX * 2.0f / Framebuffer::DEFAULT.width - 1.0f, navBaseY * -2.0f / Framebuffer::DEFAULT.height + 1.0f);
        Vec2 radius(maxSpeed / navMult * 2.0f / Framebuffer::DEFAULT.width, maxSpeed / navMult * 2.0f / Framebuffer::DEFAULT.height);
        util_drawCircle(center, radius, Vec4(0.7f, 0.7f, 0.7f, 1.0f));
        
        center.set((navBaseX + navD.x / navMult) * 2.0f / Framebuffer::DEFAULT.width - 1.0f, (navBaseY + navD.y / navMult) * -2.0f / Framebuffer::DEFAULT.height + 1.0f);
        radius *= 0.25f;
        util_drawCircle(center, radius, Vec4(0.4f, 0.4f, 0.4f, 1.0f));
    }
    
    //refTextureComp.draw();
    textComp.draw();
    
    depthShader.init();
    depthShader.use();
    depthShader.attribPointer("a_position",2,d_coord);
    depthShader.attribPointer("a_texCoord",3,d_texCoord);
    depthShader.uniformCubeMap("u_texture",pointLight.depthMap->texture,0);
    //glDrawArrays(GL_TRIANGLES,0,6*2*3);
}

void TestRenderer::destroy(){
    delete obj1;
    delete obj2;
    delete obj3;
    delete obj4;
    delete obj5;
    delete pawn;
    delete shader;
    delete refShader;
}