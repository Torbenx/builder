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
#include "util/draw_util.hpp"
#include "ui/RectBuilder.hpp"
#include "input/keycodes.hpp"

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
        pawnVertex(GL_ARRAY_BUFFER),
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
    //goFullscreen(FullscreenConfig::DEFAULT);
    //goFullscreen(getFullscreenConfig(getDisplayList().front(), 1920, 1080, 144));
    #else
    goFullscreen(FullscreenConfig::DEFAULT);
    #endif
    
    w = charToKeycode('w');
    a = charToKeycode('a');
    s = charToKeycode('s');
    d = charToKeycode('d');
    
    textComp.text = "Hello World";
    textComp.textSize = TypedSize::gl(0.1f);
    textComp.textColor = Vec4(1, 0, 0, 1);
    textComp.noBreaks = true;
    
    cubeComp.farDepth = 10.0f;
    cubeComp.cubeMap = &pointLight.depthMap.texture;
    cubeComp.positiveZ(Rect::bltr(-1, -1, 1, 1));
    
    rComp.color = Vec4(1,0,0,1);
    gComp.color = Vec4(0,1,0,1);
    bComp.color = Vec4(0,0,1,1);
}

void TestRenderer::change(int newWidth,int newHeight){
    Framebuffer::DEFAULT.viewport(newWidth,newHeight);
    float as = (float)newWidth/(float)newHeight;
    mvpMatrix.projection = Mat4x4::frustumH(67.0 * M_PI / 180.0, as, 1, 100);
    ScreenMatrics metrics;
    metrics.x.size = newWidth;
    metrics.y.size = newHeight;
    textComp.metrics = metrics;
    textComp.updateContent();
    
    
    RectBuilder cubeRect;
    cubeRect.name = "cube";
    RectBuilder textRect;
    textRect.name = "text";
    
    TypedSize pad = TypedSize::gl(0.02f);
    textRect.alignLeft(&cubeRect, -pad);
    textRect.alignBottom(&cubeRect, -pad);
    textRect.width(textComp.getContentWidth());
    textRect.height(textComp.getContentHeight());
    cubeRect.alignRight(&textRect, pad);
    cubeRect.alignTop(&textRect, pad);
    cubeRect.centerVerticalInside(nullptr);
    cubeRect.centerHorizontalInside(nullptr, 1.0f/3.0f);
    
    textComp.rect = textRect.build(metrics);
    textComp.updateRect();
    cubeComp.rect = cubeRect.build(metrics);
    cubeComp.updateRect();
    
    RectBuilder parentRect,rRect,gRect,bRect;
    rRect.name="r";
    gRect.name="g";
    bRect.name="b";
    rRect.width(TypedSize::gl(0.5f));
    gRect.width(TypedSize::gl(0.5f));
    bRect.width(TypedSize::gl(0.5f));
    rRect.height(TypedSize::gl(0.2f));
    gRect.height(TypedSize::gl(0.2f));
    bRect.height(TypedSize::gl(0.2f));
    
    parentRect.alignBottom(&bRect);
    parentRect.centerVerticalInside(nullptr);
    rRect.alignTop(&parentRect);
    gRect.below(&rRect, TypedSize::gl(0.05f));
    bRect.below(&gRect, TypedSize::gl(0.05f));
    
    parentRect.centerHorizontalInside(nullptr, 2.0f/3.0f);
    rRect.alignLeft(&parentRect);
    gRect.alignLeft(&parentRect);
    bRect.alignLeft(&parentRect);
    parentRect.alignRight(&rRect);
    
    rComp.rect = rRect.build(metrics);
    rComp.updateRect();
    gComp.rect = gRect.build(metrics);
    gComp.updateRect();
    bComp.rect = bRect.build(metrics);
    bComp.updateRect();
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
    if(keycode == w){
        wd = down;
    }else if(keycode == a){
        ad = down;
    }else if(keycode == s){
        sd = down;
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
    
    rComp.draw();
    gComp.draw();
    bComp.draw();
    cubeComp.draw();
    textComp.draw();
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