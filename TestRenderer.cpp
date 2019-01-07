#include "Renderer.hpp"
#include "util/SourceGetter.hpp"
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
#include "ui/text.hpp"

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
            "varying vec2 v_texPos;\n"
            "void main(){\n"
            "    gl_Position = a_position;\n"
            "    v_texPos = (a_position.xy + 1.0) / 2.0;\n"
            "}",
            "varying vec2 v_texPos;\n"
            "uniform sampler2D u_texture;\n"
            "void main(){\n"
            "    float far = 20.0;\n"
            "    vec4 bits = vec4(far,far/255.0,far/255.0/255.0,far/255.0/255.0/255.0);\n"
            "    vec4 f = texture2D(u_texture, v_texPos);\n"
            "    gl_FragColor = f;\n"//vec4(vec3(dot(bits,f))/far,1.0);\n"
            "}"),
        reflection(Vec3(0,1,0),Vec3(0,-2,0),2048,2048,0.5f),
        look(up, isAndroid())
        {}

void TestRenderer::create(){
    pointLight.init();
    spotLight.init();
    reflection.init();
    reflection.framebuffer.setClearColor(0,0,0,1);
    Framebuffer::DEFAULT.setClearColor(0,0,0,1);
    f1.init();
    f1.bufferData(new DirectSourceGetter<float*>(new float[3*4]{-1,-0,-1,1,-0,-1,-1,-0,1,1,-0,1}),3*4*4,GL_STATIC_DRAW);
    f2.init();
    f2.bufferData(new DirectSourceGetter<float*>(new float[3*4]{-1,3,0,1,3,0,-1,3,2,1,3,2}),3*4*4,GL_STATIC_DRAW);
    f3.init();
    f3.bufferData(new DirectSourceGetter<float*>(new float[3*4]{-10,-2,-10,-10,-2,10,10,-2,-10,10,-2,10}),3*4*4,GL_STATIC_DRAW);
    f4.init();
    f4.bufferData(new DirectSourceGetter<float*>(new float[3*4]{-10,4,-10,-10,4,10,10,4,-10,10,4,10}),3*4*4,GL_STATIC_DRAW);
    f5.init();
    f5.bufferData(new DirectSourceGetter<float*>(obj5_coords),3*4*4,GL_DYNAMIC_DRAW);
    tex_Coords.init();
    tex_Coords.bufferData(new DirectSourceGetter<float*>(new float[2*4]{-1,-1, 1,-1, -1,1, 1,1}), 2*4*4, GL_STATIC_DRAW);
    d_coord.init();
    float x1 = -1;
    float x2 = 0;
    float y1 = -1;
    float add = 1.0f/3.0f;
    float y2 = -1+add;
    d_coord.bufferData(new DirectSourceGetter<float*>(new float[6*2*3*2]{
         x1,y1,x2,y1,x1,y2, x2,y1,x1,y2,x2,y2,
         x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
         x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
         x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
         x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2,
         x1,y1+=add,x2,y1,x1,y2+=add, x2,y1,x1,y2,x2,y2
    }),6*2*3*2*4,GL_STATIC_DRAW);
    d_texCoord.init();
    d_texCoord.bufferData(new DirectSourceGetter<float*>(new float[6*2*3*3]{
         -1,-1,-1, 1,-1,-1, -1,1,-1,    1,-1,-1, -1,1,-1, 1,1,-1, //negZ
         -1,-1,1, 1,-1,1, -1,1,1,       1,-1,1, -1,1,1, 1,1,1, //posZ
         -1,-1,-1, 1,-1,-1, -1,-1,1,    1,-1,-1, -1,-1,1, 1,-1,1, //negY
         -1,1,-1, 1,1,-1, -1,1,1,       1,1,-1, -1,1,1, 1,1,1, //posY
         -1,-1,-1, -1,1,-1, -1,-1,1,    -1,1,-1, -1,-1,1, -1,1,1, //negX
         1,-1,-1, 1,1,-1, 1,-1,1,       1,1,-1, 1,-1,1, 1,1,1, //posX
    }),6*2*3*3*4,GL_STATIC_DRAW);
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
    b.addLighting(ambientLight);
    b.addLighting(pointLight);
    b.addLighting(spotLight);
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
    pawnVertex.bufferData(new DirectSourceGetter<float*>(parser.vertex),parser.vertexSize*4,GL_STATIC_DRAW);
    pawn = new ObjRenderable(&pawnVertex, Vec3(0,1,1), parser.n);
    look.dir = Vec3(0,-1,0) - Vec3(7,1,0);
    /*Vec3 max(parser.vertex[0],parser.vertex[1],parser.vertex[2]);
    Vec3 min = max;
    Vec3 avg(0);
    for(int i=1;i<parser.n;i+=3){
        Vec3 c(parser.vertex[i+0],parser.vertex[i+1],parser.vertex[i+2]);
        if(c.x > max.x)max.x = c.x;
        if(c.x < min.x)min.x = c.x;
        if(c.y > max.y)max.y = c.y;
        if(c.y < min.y)min.y = c.y;
        if(c.z > max.z)max.z = c.z;
        if(c.z < min.z)min.z = c.z;
        avg += c;
    }
    LOGD("max");
    max.debug();
    LOGD("min");
    min.debug();
    LOGD("avg");
    (avg / parser.n).debug();*/
    lframe = getNanos();
    
    //#ifdef PLATFORM_WINDOWS
    //goFullscreen(getFullscreenConfig(getDisplayList().front(), 1920, 1080, 144));
    //#else
    //goFullscreen(FullscreenConfig::DEFAULT);
    //#endif
    
    UiComponentHandle handle = layout.add(new TestComponent(Vec3(1,0,0)));
    layout.width(handle, 0.25f);
    layout.height(handle, 0.25f);
    layout.top(handle, 0.0f);
    layout.left(handle, 0.0f);
    
    UiComponentHandle below = layout.add(new TestComponent(Vec3(0,0,1)));
    layout.width(below, 0.3f);
    layout.height(below, 0.25f);
    layout.right(below, 0.0f);
    layout.above(handle, below, 0.1f);
    
    layout.setTransform(Mat4x4::Id);
    
    textTexture.init();
    initalizeTextureForText(textTexture, 1024, 1024);
}

void TestRenderer::change(int newWidth,int newHeight){
    Framebuffer::DEFAULT.viewport(newWidth,newHeight);
    float as = (float)newWidth/(float)newHeight;
    mvpMatrix.projection = Mat4x4::frustumH(67.0 * M_PI / 180.0 ,as ,1,100);
}

int lookPointerId = POINTER_ID_INVALID;
void TestRenderer::pointer(PointerEvent event){
    if(!rawInput()){
        if(event.event == POINTER_EVENT_DOWN && lookPointerId == POINTER_ID_INVALID){
            lookPointerId = event.pointerId;
        }else if(lookPointerId == event.pointerId){
            if(event.event == POINTER_EVENT_UP){
                lookPointerId = POINTER_ID_INVALID;
            }else if(event.event == POINTER_EVENT_MOVE){
                Pointer oldPointer = getPointerById(lookPointerId);
                look.move(event.x - oldPointer.x, event.y - oldPointer.y);
                mvpMatrix.view = Mat4x4::look(Vec3(7,1,0), look.dir, up);
            }
        }
    }
    trackPointerEvent(event);
}

void TestRenderer::rawPointer(int dx, int dy){
    look.move(dx, dy);
    mvpMatrix.view = Mat4x4::look(Vec3(7,1,0), look.dir, up);
}

void TestRenderer::drawFrame(){
    uint64_t now = getNanos();
    if(now - lsecond >= NANOS_PER_SECOND){
        LOGD("FPS: %d",frameCounter);
        lsecond = now;
        frameCounter = 0;
    }
    frameCounter++;
    float dt = (now - lframe)/(float)NANOS_PER_SECOND;
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
    shader->lighting(pointLight,mvpMatrix.model);
    shader->lighting(spotLight,mvpMatrix.model);
    shader->lighting(ambientLight);
    shader->uniformMat4x4("u_model",Mat4x4::Id);
    reflection.bind();
    reflection.getReflectionModel(mvpMatrix.model);
    shader->uniformMat4x4("u_mvpMatrix",mvpMatrix.setupMvp());
    obj1->draw2(shader,"a_position","u_color");
    obj2->draw2(shader,"a_position","u_color");
    obj4->draw2(shader,"a_position","u_color");
    reflection.unbind();
    mvpMatrix.model = Mat4x4::Id;
    shader->uniformMat4x4("u_mvpMatrix",mvpMatrix.setupMvp());
    obj1->draw2(shader,"a_position","u_color");
    obj2->draw2(shader,"a_position","u_color");
    obj4->draw2(shader,"a_position","u_color");
    obj5->draw2(shader,"a_position","u_color");
    pawn->draw2(shader,"a_position","u_model","u_color");
    refShader->use();
    refShader->lighting(ambientLight);
    refShader->reflection(reflection);
    refShader->lighting(pointLight,mvpMatrix.model);
    refShader->lighting(spotLight,mvpMatrix.model);
    refShader->uniformMat4x4("u_mvpMatrix",mvpMatrix.setupMvp());
    refShader->uniformMat4x4("u_model",Mat4x4::Id);
    obj3->draw2(refShader,"a_position","u_color");
    depthShader.init();
    depthShader.use();
    depthShader.attribPointer("a_position",2,d_coord);
    depthShader.attribPointer("a_texCoord",3,d_texCoord);
    depthShader.uniformCubeMap("u_texture",pointLight.depthMap->texture,0);
    //glDrawArrays(GL_TRIANGLES,0,6*2*3);
    texShader.init();
    texShader.use();
    texShader.attribPointer("a_position", 2, tex_Coords);
    texShader.uniformTexture("u_texture", textTexture, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_DEPTH_TEST);
    //layout.draw();
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