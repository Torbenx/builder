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
    obj1 = new DebugRenderable(&f1,Vec3(1,0,0), 4); obj1->mode = GL_TRIANGLE_STRIP;
    obj2 = new DebugRenderable(&f2,Vec3(0,1,0), 4); obj2->mode = GL_TRIANGLE_STRIP;
    obj3 = new ReflectionDebugRenderable(&f3, Vec3(0.5,0.5,0.5), 4, &reflection); obj3->mode = GL_TRIANGLE_STRIP;
    obj4 = new DebugRenderable(&f4,Vec3(1,1,0), 4); obj4->mode = GL_TRIANGLE_STRIP;
    obj5 = new DebugRenderable(&f5,Vec3(0,0,1), 4); obj5->mode = GL_TRIANGLE_STRIP;
    mvpMatrix.model = Mat4x4::Id;
    scene.addLight(ambientLight);
    scene.addLight(pointLight);
    scene.addLight(spotLight);
    scene.addObject(obj1);
    scene.addObject(obj2);
    scene.addObject(obj3);
    scene.addObject(obj4);
    scene.addObject(obj5);
    scene.addReflection(reflection);
    ObjParser parser;
    asset_istream file("Pawn/Pawn.obj");
    parser.parse(file, 20000, 20000);
    pawnVertex.init();
    pawnVertex.bufferData(parser.vertex,parser.vertexSize*4,GL_STATIC_DRAW);
    pawn = new DebugRenderable(&pawnVertex, Vec3(0,1,1), parser.n, Mat4x4::diagonal(Vec3(1,1,1)*0.2f) * Mat4x4::rotate(-M_PI/2.0, Vec3(1,0,0)) * Mat4x4::translate(Vec3(-16,-11,0)));
    scene.addObject(pawn);
    look.dir = Vec3(0,-1,0) - Vec3(7,1,0);
    lframe = getNanos();
    
    #ifdef PLATFORM_ANDROID
    goFullscreen(FullscreenConfig::DEFAULT);
    #endif
    
    w = charToKeycode('w');
    a = charToKeycode('a');
    s = charToKeycode('s');
    d = charToKeycode('d');
    fullscreen = charToKeycode(SPECIAL_KEY_F(11));
    exitKey = charToKeycode(SPECIAL_KEY_ESC);
    
    textComp.text = "Hello World";
    textComp.textSize = TypedSize::gl(0.1f);
    textComp.textColor = Vec4(1, 0, 0, 1);
    textComp.noBreaks = true;
    
    cubeComp.farDepth = 10.0f;
    cubeComp.cubeMap = &pointLight.depthMap.texture;
    cubeComp.positiveZ(Rect::bltr(-1, -1, 1, 1));
    
    refComp.texture = &reflection.texture;
    
    
    TypedSize pad = TypedSize::gl(0.02f);
    uiScene.layout(textComp)
        .alignLeft(cubeComp, -pad)
        .alignBottom(cubeComp, -pad)
        .wrapContent();
    uiScene.layout(cubeComp)
        .alignRight(textComp, pad)
        .alignTop(textComp, pad)
        .centerHorizontal()
        .centerVertical();
    uiScene.layout(refComp)
        .alignParentTop()
        .alignParentBottom()
        .alignParentLeft()
        .alignParentRight();
}

void TestRenderer::change(int newWidth,int newHeight){
    Framebuffer::DEFAULT.viewport(newWidth,newHeight);
    float as = (float)newWidth/(float)newHeight;
    mvpMatrix.projection = Mat4x4::frustumH(67.0 * M_PI / 180.0, as, 1, 100);
    ScreenMetrics metrics;
    metrics.x.size = newWidth;
    metrics.y.size = newHeight;
    
    textComp.metrics = metrics;
    textComp.updateContent();
    uiScene.metrics = metrics;
    uiScene.build();
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

void TestRenderer::key(uint32_t keycode, bool down){
    if(keycode == w){
        wd = down;
    }else if(keycode == a){
        ad = down;
    }else if(keycode == s){
        sd = down;
    }else if(keycode == d){
        dd = down;
    }else if(keycode == fullscreen){
        if(down){
            if(!isFullscreen()){
                #ifdef PLATFORM_WINDOWS
                //goFullscreen(FullscreenConfig::DEFAULT);
                goFullscreen(getFullscreenConfig(getDisplayList().front(), 1920, 1080, 144));
                #endif
            }else{
                exitFullscreen();
            }
        }
    }else if(keycode == exitKey){
        if(down)exit();
    }
}

void TestRenderer::drawFrame(){
    uint64_t now = getNanos();
    if(now - lsecond >= NANOS_PER_SECOND){
        //LOGD("FPS: %d",frameCounter);
        
        std::stringstream ss;
        ss << "FPS: " << frameCounter;
        textComp.text = ss.str();
        textComp.updateContent();
        uiScene.build();
        
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
    scene.draw(mvpMatrix);
    glDisable(GL_DEPTH_TEST);
    
    if(navPointerId != POINTER_ID_INVALID){
        Vec2 center(navBaseX * 2.0f / Framebuffer::DEFAULT.width - 1.0f, navBaseY * -2.0f / Framebuffer::DEFAULT.height + 1.0f);
        Vec2 radius(maxSpeed / navMult * 2.0f / Framebuffer::DEFAULT.width, maxSpeed / navMult * 2.0f / Framebuffer::DEFAULT.height);
        util_drawCircle(center, radius, Vec4(0.7f, 0.7f, 0.7f, 1.0f));
        
        center.set((navBaseX + navD.x / navMult) * 2.0f / Framebuffer::DEFAULT.width - 1.0f, (navBaseY + navD.y / navMult) * -2.0f / Framebuffer::DEFAULT.height + 1.0f);
        radius *= 0.25f;
        util_drawCircle(center, radius, Vec4(0.4f, 0.4f, 0.4f, 1.0f));
    }
    
    cubeComp.draw();
    textComp.draw();
    //refComp.draw();
}

void TestRenderer::destroy(){
    delete obj1;
    delete obj2;
    delete obj3;
    delete obj4;
    delete obj5;
    delete pawn;
}