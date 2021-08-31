#include "TestRenderer.hpp"
#include <Renderer.hpp>
#include <containers/CacheMap.hpp>
#include <containers/IdAllocator.hpp>
#include <events/ExecutionContext.hpp>
#include <events/Task.hpp>
#include <fstream>
#include <input/keycodes.hpp>
#include <math/constants.hpp>
#include <numeric>
#include <resource/JpegDecoder.hpp>
#include <resource/ObjParser.hpp>
#include <resource/asset_istream.hpp>
#include <string>
#include <thread>
#include <util/LogTimer.hpp>
#include <util/draw_util.hpp>
#include <util/fullscreen.hpp>
#include <util/log.hpp>
#include <util/target.hpp>
#include <util/time.hpp>
#include <events/EventLogic.hpp>

#ifdef PLATFORM_WINDOWS
#include <windows/fullscreen.hpp>
#endif

template<Gapi G>
TestRenderer<G>::TestRenderer()
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
    , pos(7, 1, 0)
    , renderpass(&Framebuffer::DEFAULT)
    , col1(Vec3(1, 0, 0) * 0.5f)
    , col2(Vec3(0, 1, 0) * 0.5f)
    , col3(Vec3(1, 1, 1) * 0.1f)
    , col4(Vec3(1, 1, 0) * 0.5f)
    , col5(Vec3(0, 0, 1) * 0.5f) {

    Framebuffer::DEFAULT.clearColor(0, 0, 0, 1);
    reflection.framebuffer.clearColor(0, 0, 0, 1);

    float f1Scale = 0.5f;
    f1 = VertexBuffer::upload({ Vec3(-1, -1, 0) * f1Scale, Vec3(-1, 1, 0) * f1Scale, Vec3(1, -1, 0) * f1Scale, Vec3(1, 1, 0) * f1Scale });
    f2 = VertexBuffer::upload({ Vec3(-1, 3, 0), Vec3(-1, 3, 2), Vec3(1, 3, 0), Vec3(1, 3, 2) });
    f3 = VertexBuffer::upload({ Vec3(-10, -2, -10), Vec3(-10, -2, 10), Vec3(10, -2, -10), Vec3(10, -2, 10) });
    f4 = VertexBuffer::upload({ Vec3(-10, 4, -10), Vec3(-10, 4, 10), Vec3(10, 4, -10), Vec3(10, 4, 10) });
    f5 = VertexBuffer::upload(obj5_coords, WriteFrequency::Often);
    fNormal = VertexBuffer::upload({ up, up, up, up });

    id_t id1 = objects.insertObject({ Mat4x4::Id, DrawMode::TriangleStrip, f1, fNormal, IndexBuffer::withoutData(4) });
    id_t id2 = objects.insertObject({ Mat4x4::Id, DrawMode::TriangleStrip, f2, fNormal, IndexBuffer::withoutData(4) });
    id_t id3 = objects.insertObject({ Mat4x4::Id, DrawMode::TriangleStrip, f3, fNormal, IndexBuffer::withoutData(4) });
    id_t id4 = objects.insertObject({ Mat4x4::Id, DrawMode::TriangleStrip, f4, fNormal, IndexBuffer::withoutData(4) });
    id_t id5 = objects.insertObject({ Mat4x4::Id, DrawMode::TriangleStrip, f5, fNormal, IndexBuffer::withoutData(4) });

    std::vector<Effect::Handle> effects1 { col1.handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle() };
    std::vector<Effect::Handle> effects2 { col2.handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle() };
    std::vector<Effect::Handle> effects3 { col3.handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle(), reflection.handle() };
    std::vector<Effect::Handle> effects4 { col4.handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle() };
    std::vector<Effect::Handle> effects5 { col5.handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle() };

    renderpass.addObject(id1, effects1);
    renderpass.addObject(id2, effects2);
    renderpass.addObject(id3, effects3);
    renderpass.addObject(id4, effects4);
    renderpass.addObject(id5, effects5);

    reflection.addObject(id1, effects1);
    reflection.addObject(id2, effects2);
    reflection.addObject(id4, effects4);
    reflection.addObject(id5, effects5);

    spotLight.addObject(id1);
    spotLight.addObject(id2);
    spotLight.addObject(id3);
    spotLight.addObject(id4);
    spotLight.addObject(id5);

    pointLight.addObject(id1);
    pointLight.addObject(id2);
    pointLight.addObject(id3);
    pointLight.addObject(id4);
    pointLight.addObject(id5);

    look.dir = Vec3::normalize(Vec3(0, -1, 0) - pos);
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

    refComp.texture = &reflection.texture.value;

    Mat4x4 base = Mat4x4::diagonal(Vec3(1, 1, 1) * 0.1f) * Mat4x4::rotate(-F_PI / 2.0f, Vec3(1, 0, 0)) * Mat4x4::translate(Vec3(+2, -16, 0));
    Effect* textureEffect = new TextureColor(pawnTexture, pawnTexCoords);

    for (int_t i = -0; i <= 0; i++) {
        id_t id = objects.insertObject({ Mat4x4::translate(Vec3(i * 0.5f, -1.5, 0)) * base, DrawMode::Triangles, pawnVertex, pawnNormals, pawnIndices });
        renderpass.addObject(id, { textureEffect->handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle() });
        reflection.addObject(id, { textureEffect->handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle() });
        spotLight.addObject(id);
        pointLight.addObject(id);
    }

    glEnable(GL_DEPTH_TEST);
    pointLight.draw(objects);
    spotLight.draw(objects);
    glDisable(GL_DEPTH_TEST);

    event_logic::InitialHolder<__COUNTER__, int> a = 1;
    event_logic::InitialHolder<__COUNTER__, int> b = 2;
    event_logic::InitialHolder<__COUNTER__, int> c = 3;
    auto r = (a && c) || (b && c);
    //event_logic::Result<c> r { 0 };
    auto tuple2 = event_logic::deduplicate(r.tuple);
    log("a={}, b={}, c={}, size={}, size_dedup={}", r.LS.bitmask(a.ID), r.LS.bitmask(b.ID), r.LS.bitmask(c.ID), std::tuple_size<decltype(r.tuple)>(), std::tuple_size<decltype(tuple2)>());
}

template<Gapi G>
void TestRenderer<G>::change(int_t newWidth, int_t newHeight) {
    Framebuffer::DEFAULT.viewport(newWidth, newHeight);
    float as = (float)newWidth / (float)newHeight;
    float fovV = 67.0f * F_PI / 180.0f;
    projection = Mat4x4::frustumV(fovV, as, 0.1f, 100.0f);
    ScreenMetrics metrics;
    metrics.x.size = newWidth;
    metrics.y.size = newHeight;

    textComp.updateContent(metrics, uiScene.allocator);
    uiScene.metrics = metrics;
    uiScene.build();

    CacheMap<uint32_t, 2> map;
    uint16_t count = 50;
    uint16_t multX = 1;
    uint16_t multY = 1;
    for (int_t x = count - 1; x >= 0; x--) {
        map.insertOrAssign({ (uint16_t)((x * multX) % count), (uint16_t)((x * multY) % count) }, x);
        map.insertOrAssign({ (uint16_t)((x * multX) % count), (uint16_t)((x * multY + 3) % count) }, x);
    }
    map.insertOrAssign({ 27, 40 }, 10000);
    map.remove({ 26, 26 });
    map.remove({ 25, 23 });
    map.remove({ 24, 1 });
    std::stringstream ss;
    for (uint16_t x = 0; x < count; x++) {
        for (uint16_t y = 0; y < count; y++) {
            auto hint = map.find({ x, y });
            if (hint.isValid()) {
                ss << *hint << ", ";
            } else {
                ss << "-, ";
            }
        }
        ss << '\n';
    }
    log("{}", ss.str());
}

class MyTask : public Task {
public:
    Task* sched;
    void MIGHT_YIELD run() {
        while (true) {
            // Task::schedulingQueue.pushBack(sched);
            /*for (volatile int_t i = 0; i < 100000; ++i) {

            }*/
            yield();
        }
    }
};

template<Gapi G>
void TestRenderer<G>::pointer(PointerEvent event) {
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
        const int_t taskCount = 1000;
        const int_t extraThreadCount = 0;
        MyTask* tasks = new MyTask[taskCount];
        Task::schedulingQueue.clear();

        for (int_t i = 0; i < taskCount; i++) {
            tasks[i].sched = &tasks[(taskCount + i - 1) % taskCount];
            Task::schedulingQueue.pushBack(&tasks[i]);
        }

        ShutdownTask* shutdownTasks = new ShutdownTask[extraThreadCount + 1];
        for (int_t i = 0; i < extraThreadCount + 1; i++) {
            Task::schedulingQueue.pushBack(&shutdownTasks[i]);
        }
        SchedulerThead threads[extraThreadCount];
        /*for (volatile int_t i = 0; i < 100000; ++i) {

        }*/

        int_t start = getNanos();
        me.enter();
        int_t end = getNanos();

        for (int_t i = 0; i < extraThreadCount; i++) {
            threads[i].join();
        }

        delete[] tasks;

        static std::vector<int_t> runs(100);
        runs.erase(runs.begin());
        runs.push_back((end - start) / 1000);
        log("time: {} us, avg: {} us", (end - start) / 1000, std::accumulate(runs.begin(), runs.end(), 0) / (float)runs.size());
    } while (false);
}

template<Gapi G>
void TestRenderer<G>::rawPointer(int_t dx, int_t dy) {
    look.move(dx, dy);
}

template<Gapi G>
void TestRenderer<G>::rawKey(uint32_t keycode, bool down) {
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
            RendererState::exit();
    }
}

template<Gapi G>
void TestRenderer<G>::drawFrame() {
    uint64_t now = getNanos();
    if (now - lsecond >= NANOS_PER_SECOND) {
        log("FPS: {}", frameCounter);

        std::stringstream ss;
        ss << "FPS: " << frameCounter;
        textComp.text = ss.str();
        textComp.updateContent(uiScene.metrics, uiScene.allocator);
        uiScene.build();

        lsecond = now;
        frameCounter = 0;
    }
    frameCounter++;

    float maxSpeed = 2.0f;
    float dt = (now - lframe) / (float)NANOS_PER_SECOND;
    lframe = now;
    Vec3 dirX = Vec3::normalize(look.dir - Vec3::dot(look.dir, up) * up);
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
    Vec3& plp = pointLight.position;
    plp += Vec3(plp.z * dt, plp.z * dt / 2.0, -plp.x * dt);
    pointLight.updateViewProjectionMatrix();
    obj5_coords[0] = Vec3(plp.x - 0.3f, plp.y, plp.z - 0.3f);
    obj5_coords[1] = Vec3(plp.x - 0.3f, plp.y, plp.z + 0.3f);
    obj5_coords[2] = Vec3(plp.x + 0.3f, plp.y, plp.z - 0.3f);
    obj5_coords[3] = Vec3(plp.x + 0.3f, plp.y, plp.z + 0.3f);
    f5->update(obj5_coords);

    renderpass.viewProjectionMatrix = projection * view;
    reflection.updateViewProjectionMatrix(renderpass.viewProjectionMatrix);
    // glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    pointLight.draw(objects);
    reflection.draw(objects);
    renderpass.draw(objects);
    glDisable(GL_DEPTH_TEST);
    // glDisable(GL_CULL_FACE);

    if (navPointerId != POINTER_ID_INVALID) {
        Vec2 center = { navBaseX * 2.0f / uiScene.metrics.width() - 1.0f, navBaseY * -2.0f / uiScene.metrics.height() + 1.0f };
        Vec2 radius = { maxSpeed / navMult * 2.0f / uiScene.metrics.width(), maxSpeed / navMult * 2.0f / uiScene.metrics.height() };
        // util_drawCircle(center, radius, Vec4(0.7f, 0.7f, 0.7f, 1.0f));

        center = {( navBaseX + navD.x / navMult) * 2.0f / uiScene.metrics.width() - 1.0f, (navBaseY + navD.y / navMult) * -2.0f / uiScene.metrics.height() + 1.0f };
        radius *= 0.25f;
        // util_drawCircle(center, radius, Vec4(0.4f, 0.4f, 0.4f, 1.0f));
    }

    // cubeComp.draw(Framebuffer::DEFAULT, uiScene.allocator);
    // refComp.draw(Framebuffer::DEFAULT, uiScene.allocator);
    textComp.draw(Framebuffer::DEFAULT, uiScene.allocator);

    /*constexpr uint32_t N = 4;
    LinearProgramming<N> lp;
    uint32_t numConstraints = 200;
    uint32_t constraint[N];
    for (uint32_t i = 0; i < numConstraints; i++) {
        for (uint32_t j = 0; j < N; j++) {
            lp.addVariable();
            constraint[j] = i * N + j;
        }
        lp.addConstraint(constraint);
    }
    uint32_t constrains[] = {
        0, 3, 4, 5,
        1, 3, 4, 5,
        2, 3, 4, 5,
     };

    lp.addVariable();
    lp.addVariable();
    lp.addVariable();
    lp.addVariable();
    lp.addVariable();
    lp.addVariable();
    lp.addConstraint(&constrains[0]);
    lp.addConstraint(&constrains[4]);
    lp.addConstraint(&constrains[8]);*/
    //LogTimer timer;
    //lp.relaxed();
    //lp.dual.debug();
}

template<Gapi G>
TestRenderer<G>::~TestRenderer() { }

INSTANTIATE_G_API_CLASS(TestRenderer);
