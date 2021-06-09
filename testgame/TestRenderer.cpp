#include "TestRenderer.hpp"
#include <Renderer.hpp>
#include <events/ExecutionContext.hpp>
#include <events/Task.hpp>
#include <fstream>
#include <input/keycodes.hpp>
#include <math/constants.hpp>
#include <resource/JpegDecoder.hpp>
#include <resource/ObjParser.hpp>
#include <resource/asset_istream.hpp>
#include <string>
#include <thread>
#include <util/draw_util.hpp>
#include <util/fullscreen.hpp>
#include <util/log.hpp>
#include <util/target.hpp>
#include <util/time.hpp>

#ifdef PLATFORM_WINDOWS
#include <windows/fullscreen.hpp>
#endif

TestRenderer::TestRenderer()
    : up(0, 1, 0)
    , pointLight(
          Vec3(0, 1, -2),
          1024, 1024,
          Vec3(1, 0.3, 1) * 12.0f,
          0.3f,
          30)
    , spotLight(
          Vec3(0, 3.4, 0),
          Vec3(0, -1, 0),
          F_PI * 0.2f,
          F_PI * 0.4f,
          Vec3(1, 0, 0),
          2048, 2048,
          Vec3(1, 1, 1) * 8.0f,
          0.7f,
          30)
    , ambientLight(Vec3(1, 1, 1) * 1.0f)
    , reflection(Vec3(0, 1, 0), Vec3(0, -2, 0), 2048, 2048, 0.7f)
    , look(up, isAndroid())
    , pos(7, 1, 0) {

    Framebuffer::DEFAULT.clearColor(0, 0, 0, 1);
    reflection.framebuffer.clearColor(0, 0, 0, 1);

    float f1Scale = 0.5f;
    f1 = VertexBuffer::upload({ Vec3(-1, 0, -1) * f1Scale, Vec3(-1, 0, 1) * f1Scale, Vec3(1, 0, -1) * f1Scale, Vec3(1, 0, 1) * f1Scale });
    f2 = VertexBuffer::upload({ Vec3(-1, 3, 0), Vec3(-1, 3, 2), Vec3(1, 3, 0), Vec3(1, 3, 2) });
    f3 = VertexBuffer::upload({ Vec3(-10, -2, -10), Vec3(-10, -2, 10), Vec3(10, -2, -10), Vec3(10, -2, 10) });
    f4 = VertexBuffer::upload({ Vec3(-10, 4, -10), Vec3(-10, 4, 10), Vec3(10, 4, -10), Vec3(10, 4, 10) });
    f5.setEternal(VertexBuffer::wrap(obj5_coords));
    fNormal = VertexBuffer::upload({ up, up, up, up });

    scene.addEffect(&ambientLight);
    spotLight.addTo(scene);
    pointLight.addTo(scene);

    scene.addRenderpass(&Framebuffer::DEFAULT, Mat4x4::Id);
    scene.addObject({ Mat4x4::Id, DrawMode::TriangleStrip, f3, fNormal, IndexBuffer::withoutData(4), Box::INFINIT }, { { new UniformColor(Vec3(1, 1, 1) * 0.1f), &reflection } });
    reflection.addTo(scene);
    scene.addObject({ Mat4x4::Id, DrawMode::TriangleStrip, f1, fNormal, IndexBuffer::withoutData(4), Box::INFINIT }, { new UniformColor(Vec3(1, 0, 0) * 0.5f) });
    scene.addObject({ Mat4x4::Id, DrawMode::TriangleStrip, f2, fNormal, IndexBuffer::withoutData(4), Box::INFINIT }, { new UniformColor(Vec3(0, 1, 0) * 0.5f) });
    // scene.addObject({ Mat4x4::Id, DrawMode::TriangleStrip, f4.value, fNormal.value, IndexBuffer::withoutData(4), Box::INFINIT }, { new UniformColor(Vec3(1, 1, 0) * 0.5f) });
    scene.addObject({ Mat4x4::Id, DrawMode::TriangleStrip, f5, fNormal, IndexBuffer::withoutData(4), Box::INFINIT }, { new UniformColor(Vec3(0, 0, 1) * 0.5f) });

    look.dir = (Vec3(0, -1, 0) - pos).normalize();
    lframe = getNanos();

#ifdef PLATFORM_ANDROID
    goFullscreen(FullscreenConfig::DEFAULT);
#endif

    w = charToKeycode('w');
    a = charToKeycode('a');
    s = charToKeycode('s');
    d = charToKeycode('d');
    downKey = charToKeycode(SPECIAL_KEY_SHIFT);
    upKey = charToKeycode(' ');
    fullscreen = charToKeycode(SPECIAL_KEY_F(11));
    exitKey = charToKeycode(SPECIAL_KEY_ESC);

    textComp.text = "Hello World";
    textComp.textSize = TypedSize::gl(0.1f);
    textComp.textColor = Vec4(1, 0, 0, 1);
    textComp.noBreaks = true;

    cubeComp.farDepth = 1.0f;
    cubeComp.texture = &spotLight.depthMap.value;
    refComp.texture = &reflection.texture.value;

    uiScene.layout(&textComp)
        .alignTop()
        .alignLeft()
        .wrapContentWidth()
        .wrapContentHeight();
    uiScene.layout(&cubeComp)
        .align(nullptr, TypedSize::gl(0.1f));
    uiScene.layout(&refComp)
        .align();

    ObjParser parser;
    asset_istream file("QueenSideB/piece.obj");
    parser.parse(file);
    pawnVertex = parser.vertex;
    pawnTexCoords = parser.texCoords;
    pawnNormals = parser.normals;
    pawnIndices = parser.indices;

    asset_istream jpegFile("QueenSideB/piece.jpg");
    JpegDecoder decoder;
    decoder.decode(jpegFile);
    pawnTexture = decoder.texture;

    refComp.texture = &pawnTexture.value;

    Mat4x4 base = Mat4x4::diagonal(Vec3(1, 1, 1) * 0.1f) * Mat4x4::rotate(-F_PI / 2.0f, Vec3(1, 0, 0)) * Mat4x4::translate(Vec3(+2, -16, 0));
    TextureColor* textureEffect = new TextureColor(pawnTexture, pawnTexCoords);

    for (int i = -5; i <= 5; i++) {
        scene.addObject({ Mat4x4::translate(Vec3(i * 0.5f, -1.5, 0)) * base, DrawMode::Triangles, pawnVertex, pawnNormals, pawnIndices, parser.boundingBox }, { textureEffect });
    }
}

void TestRenderer::change(int newWidth, int newHeight) {
    Framebuffer::DEFAULT.viewport(newWidth, newHeight);
    float as = (float)newWidth / (float)newHeight;
    projection = Mat4x4::frustumH(67.0f * F_PI / 180.0f, as, 0.1f, 100.0f);
    ScreenMetrics metrics;
    metrics.x.size = newWidth;
    metrics.y.size = newHeight;

    textComp.updateContent(metrics);
    uiScene.metrics = metrics;
    uiScene.build();
}

std::atomic<size_t> testCounter = 0;

class MyTask : public Task {
public:
    Task* sched;
    void run() {
        while (true) {
            size_t counter = testCounter.fetch_add(1, std::memory_order_relaxed);
            Task::schedulingQueue.pushBack(sched);
            yield();
        }
    }
};

void TestRenderer::pointer(PointerEvent event) {
    if (!rawInput()) {
        if (event.event == POINTER_EVENT_DOWN && navPointerId == POINTER_ID_INVALID) {
            navPointerId = event.pointerId;
            navBaseX = event.x;
            navBaseY = event.y;
        } else if (event.event == POINTER_EVENT_DOWN && lookPointerId == POINTER_ID_INVALID) {
            lookPointerId = event.pointerId;
        } else if (event.pointerId == navPointerId) {
            if (event.event == POINTER_EVENT_UP) {
                navDeltaX = 0;
                navDeltaY = 0;
                navPointerId = POINTER_ID_INVALID;
            } else if (event.event == POINTER_EVENT_MOVE) {
                navDeltaX = event.x - navBaseX;
                navDeltaY = event.y - navBaseY;
            }
        } else if (event.pointerId == lookPointerId) {
            if (event.event == POINTER_EVENT_UP) {
                lookPointerId = POINTER_ID_INVALID;
            } else if (event.event == POINTER_EVENT_MOVE) {
                Pointer oldPointer = getPointerById(lookPointerId);
                look.move(event.x - oldPointer.x, event.y - oldPointer.y);
            }
        }
    }
    trackPointerEvent(event);

    do {
        static Scheduler me;
        const size_t targetCounter = 100;
        const size_t taskCount = targetCounter;
        const size_t extraThreadCount = 0;
        testCounter = 0;
        MyTask* tasks = new MyTask[taskCount];
        Task::schedulingQueue.clear();

        for (size_t i = 0; i < taskCount; i++) {
            tasks[i].sched = &tasks[(taskCount + i - 1) % taskCount];
            if (i != 0)
                Task::schedulingQueue.pushBack(&tasks[i]);
        }

        ShutdownTask* shutdownTasks = new ShutdownTask[extraThreadCount + 1];
        for (size_t i = 0; i < extraThreadCount + 1; i++) {
            Task::schedulingQueue.pushBack(&shutdownTasks[i]);
        }
        SchedulerThead threads[extraThreadCount];

        size_t start = getNanos();
        me.enter();
        size_t end = getNanos();

        for (int i = 0; i < extraThreadCount; i++) {
            threads[i].join();
        }

        delete[] tasks;
        static size_t testCount = 0;
        testCount++;
        static size_t testTotal = 0;
        testTotal += (end - start) / 1000;
        LOGD("counter: %u, time: %u us, avg: %f", testCounter.load(std::memory_order_relaxed), (end - start) / 1000, testTotal / (float)testCount);
    } while (false);
}

void TestRenderer::rawPointer(int dx, int dy) {
    look.move(dx, dy);
}

void TestRenderer::rawKey(uint32_t keycode, bool down) {
    if (keycode == w) {
        wd = down;
    } else if (keycode == a) {
        ad = down;
    } else if (keycode == s) {
        sd = down;
    } else if (keycode == d) {
        dd = down;
    } else if (keycode == upKey) {
        upKeyDown = down;
    } else if (keycode == downKey) {
        downKeyDown = down;
    } else if (keycode == fullscreen) {
        if (down) {
            if (!isFullscreen()) {
#ifdef PLATFORM_WINDOWS
                goFullscreen(FullscreenConfig::DEFAULT);
                // goFullscreen(getFullscreenConfig(getDisplayList().front(), 1920, 1080, 144));
#endif
            } else {
                exitFullscreen();
            }
        }
    } else if (keycode == exitKey) {
        if (down)
            exit();
    }
}

void TestRenderer::drawFrame() {
    uint64_t now = getNanos();
    if (now - lsecond >= NANOS_PER_SECOND) {
        LOGD("FPS: %d", frameCounter);

        std::stringstream ss;
        ss << "FPS: " << frameCounter;
        textComp.text = ss.str();
        textComp.updateContent(uiScene.metrics);
        uiScene.build();

        lsecond = now;
        frameCounter = 0;
    }
    frameCounter++;

    float maxSpeed = 2.0f;
    float dt = (now - lframe) / (float)NANOS_PER_SECOND;
    lframe = now;
    Vec3 dirX = (look.dir - Vec3::dot(look.dir, up) * up).normalize();
    if (wd) {
        pos += maxSpeed * dt * dirX;
    }
    if (sd) {
        pos -= maxSpeed * dt * dirX;
    }
    if (ad) {
        pos -= maxSpeed * dt * Vec3::cross(dirX, up);
    }
    if (dd) {
        pos += maxSpeed * dt * Vec3::cross(dirX, up);
    }
    if (downKeyDown) {
        pos -= maxSpeed * dt * up;
    }
    if (upKeyDown) {
        pos += maxSpeed * dt * up;
    }
    float navMult = 0.007f;
    Vec2 navD = Vec2(navDeltaX, navDeltaY) * navMult;
    float navM = navD.length();
    if (navM >= maxSpeed) {
        navD *= maxSpeed / navM;
    }
    pos += navD.x * dt * Vec3::cross(dirX, up);
    pos -= navD.y * dt * dirX;
    Mat4x4 view = Mat4x4::look(pos, look.dir, up);
    Vec3 plp = pointLight.position;
    // plp += Vec3(plp.z * dt, plp.z * dt / 2.0, -plp.x * dt);
    pointLight.position = plp;
    obj5_coords[0] = Vec3(plp.x - 0.3f, plp.y, plp.z - 0.3f);
    obj5_coords[1] = Vec3(plp.x - 0.3f, plp.y, plp.z + 0.3f);
    obj5_coords[2] = Vec3(plp.x + 0.3f, plp.y, plp.z - 0.3f);
    obj5_coords[3] = Vec3(plp.x + 0.3f, plp.y, plp.z + 0.3f);

    scene.updateViewProjectionMatrix(0, projection * view);
    scene.updateViewProjectionMatrix(1, projection * view * reflection.getReflectionMatrix());
    // glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    scene.draw();
    glDisable(GL_DEPTH_TEST);
    // glDisable(GL_CULL_FACE);

    if (navPointerId != POINTER_ID_INVALID) {
        Vec2 center(navBaseX * 2.0f / uiScene.metrics.width() - 1.0f, navBaseY * -2.0f / uiScene.metrics.height() + 1.0f);
        Vec2 radius(maxSpeed / navMult * 2.0f / uiScene.metrics.width(), maxSpeed / navMult * 2.0f / uiScene.metrics.height());
        // util_drawCircle(center, radius, Vec4(0.7f, 0.7f, 0.7f, 1.0f));

        center.set((navBaseX + navD.x / navMult) * 2.0f / uiScene.metrics.width() - 1.0f, (navBaseY + navD.y / navMult) * -2.0f / uiScene.metrics.height() + 1.0f);
        radius *= 0.25f;
        // util_drawCircle(center, radius, Vec4(0.4f, 0.4f, 0.4f, 1.0f));
    }

    // cubeComp.draw(Framebuffer::DEFAULT);
    // refComp.draw(Framebuffer::DEFAULT);
    textComp.draw(Framebuffer::DEFAULT);
}

TestRenderer::~TestRenderer() {
    /*delete obj1;
    delete obj2;
    delete obj3;
    delete obj4;
    delete obj5;
    delete pawn;*/
}
