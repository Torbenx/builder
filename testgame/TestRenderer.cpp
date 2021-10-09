#include "TestRenderer.hpp"
#include <Renderer.hpp>
#include <containers/CacheMap.hpp>
#include <containers/RangeAllocator.hpp>
#include <fstream>
#include <input/keycodes.hpp>
#include <math/constants.hpp>
#include <numeric>
#include <random>
#include <resource/JpegDecoder.hpp>
#include <resource/ObjParser.hpp>
#include <resource/asset_istream.hpp>
#include <script/events.hpp>
#include <string>
#include <task/Promise.hpp>
#include <thread>
#include <util/LogTimer.hpp>
#include <util/draw_util.hpp>
#include <util/fullscreen.hpp>
#include <util/log.hpp>
#include <util/target.hpp>
#include <util/time.hpp>

#ifdef PLATFORM_WINDOWS
#include <windows/fullscreen.hpp>
#endif

/*struct Test {
    int_t registerEvent(EventReceiver*, uint64_t bitmask) {
        log("bitmask: {:#b}", bitmask);
        return 0;
    }

    void cancelEvent(EventReceiver*, int_t) { }
};

Script<float> test2() {
    co_return;
}

Script<int> test() {
    ScriptPromise<int>& self = co_await get_self;
    auto a = waitable(Test {});
    auto b = waitable(Test {});
    auto c = waitable(Test {});
    auto r = (a && c) || (b && c);

    future_script<float> ref = self.acquire_future(test2());
    float& inf = ref.interface();

    //log("a={}, b={}, c={}, size={}", r.LS.bitmask(a.ID), r.LS.bitmask(b.ID), r.LS.bitmask(c.ID), std::tuple_size_v<decltype(r.tuple)>);

    co_await r;
}*/

struct MyInterface {
    int i;
};

task::Task<task::Description { task::interface<MyInterface> }> testChild() {
    auto& self = co_await get_self;

    while (true) {
        log("tick={}, i={}", task::detail::tick, self.get<MyInterface>().i);
        co_yield 0;
    }
}

task::Task<> testParent() {
    auto& self = co_await get_self;

    auto ext = self.spawnLazy<testChild>();
    auto ref = ext.get<MyInterface>(0);
    int counter = 0;
    while (true) {
        log("tick={}, c={}", task::detail::tick, counter);
        ref->i = counter;
        counter++;
        co_yield 0;
    }
}

template<Gapi G>
TestRenderer<G>::TestRenderer()
    : up(0, 1, 0)
    , look(up, platform::isAndroid())
    , pos(7, 1, 0)
    , renderpass(&Framebuffer::DEFAULT)
    , spotLightDepthPass(&spotLightFB)
    , pointLightDepthPasses { &pointLightFBs[0], &pointLightFBs[1], &pointLightFBs[2], &pointLightFBs[3], &pointLightFBs[4], &pointLightFBs[5] }
    , reflectionPass(&reflectionFB)
    , pointLightPosition(0, 1, -2)
    , f1 { .modelMatrix = Mat4x4::Id, .drawMode = DrawMode::TriangleStrip, .indices = IndexBuffer::withoutData(4) }
    , f2 { .modelMatrix = Mat4x4::Id, .drawMode = DrawMode::TriangleStrip, .indices = IndexBuffer::withoutData(4) }
    , f3 { .modelMatrix = Mat4x4::Id, .drawMode = DrawMode::TriangleStrip, .indices = IndexBuffer::withoutData(4) }
    , f4 { .modelMatrix = Mat4x4::Id, .drawMode = DrawMode::TriangleStrip, .indices = IndexBuffer::withoutData(4) }
    , f5 { .modelMatrix = Mat4x4::Id, .drawMode = DrawMode::TriangleStrip, .indices = IndexBuffer::withoutData(4) } {

    task::Global* global = new task::Global;
    task::detail::tick = 0;
    task::PromiseBase::spawnInternal(testParent, global, true);
    global->enter();

    /*HANDLE token;
    VERIFY(Default, OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token));
    LUID luid;
    VERIFY(Default, LookupPrivilegeValue(NULL, SE_LOCK_MEMORY_NAME, &luid));
    struct {
        DWORD count = 1;
        LUID_AND_ATTRIBUTES array[1];
    } info;
    info.array[0] = { luid, SE_PRIVILEGE_ENABLED };
    VERIFY(Default, AdjustTokenPrivileges(token, FALSE, (PTOKEN_PRIVILEGES)&info, 0, NULL, NULL));
    log("lastError = {}, ERROR_SUCCESS = {}, ERROR_NOT_ALL_ASSIGNED = {}", GetLastError(), ERROR_SUCCESS, ERROR_NOT_ALL_ASSIGNED);
    CloseHandle(token);

    ULONG_PTR pages = 1;
    ULONG_PTR pagesOut[1];
    VERIFY(Default, AllocateUserPhysicalPages(GetCurrentProcess(), &pages, pagesOut));
    log("lastError = {}, ERROR_PRIVILEGE_NOT_HELD = {}", GetLastError(), ERROR_PRIVILEGE_NOT_HELD);*/

    Framebuffer::DEFAULT.clearColor(0, 0, 0, 1);

    ambientLight.fragmentUniform.staticUpdate(allocator).color = Vec3(1, 1, 1) * 1.0f;

    spotLightDepthMap = Texture::depth(2048, 2048, Texture::DepthBits::D16);
    spotLightFB.bind();
    spotLightFB.attach(spotLightDepthMap);
    spotLightFB.viewport(2048, 2048);
    Mat4x4 mat = Mat4x4::make({ 0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.5f - 0.001f, 1.0f });
    spotLightDepthPass.viewProjectionMatrix = Mat4x4::frustumVH(F_PI * 0.2f, F_PI * 0.4f, 0.7f, 30.0f) * Mat4x4::look(Vec3(0, 3.4f, 0), Vec3(0, -1, 0), Vec3(1, 0, 0));
    spotLight.fragmentUniform.staticUpdate(allocator) = { .color = Vec3(1, 1, 1) * 8.0f, .lightPos = Vec3(0, 3.4f, 0) };
    spotLight.vertexUniform.staticUpdate(allocator) = { .mvpMatrix = mat * spotLightDepthPass.viewProjectionMatrix };
    spotLight.depthMap = *spotLightDepthMap;

    pointLightDepthMap = CubeMap::depth(1024, 1024, Texture::DepthBits::D16);
    for (int_t i = 0; i < 6; i++) {
        pointLightFBs[i].bind();
        pointLightFBs[i].attach(pointLightDepthMap, CubeMap::Faces[i]);
        pointLightFBs[i].viewport(1024, 1024);
    }
    pointLight.depthMap = *pointLightDepthMap;
    pointLight.fragmentUniform.allocateDynamic(allocator);
    pointLight.vertexUniform.allocateDynamic(allocator);
    updatePointLightData();

    reflectionDepthBuffer = Texture::depth(2048, 2048, Texture::DepthBits::D16);
    reflectionTexture = Texture::color(2048, 2048, Texture::Format::RGBA8888);
    reflectionFB.bind();
    reflectionFB.attach(reflectionDepthBuffer);
    reflectionFB.attach(reflectionTexture);
    reflectionFB.viewport(2048, 2048);
    reflection.fragmentUniform.staticUpdate(allocator).factor = 0.7f;
    reflection.texture = *reflectionTexture;
    reflectionFB.clearColor(0, 0, 0, 1);

    col1.fragmentUniform.staticUpdate(allocator).color = Vec3(1, 0, 0) * 0.5f;
    col2.fragmentUniform.staticUpdate(allocator).color = Vec3(0, 1, 0) * 0.5f;
    col3.fragmentUniform.staticUpdate(allocator).color = Vec3(1, 1, 1) * 0.1f;
    col4.fragmentUniform.staticUpdate(allocator).color = Vec3(1, 1, 0) * 0.5f;
    col5.fragmentUniform.staticUpdate(allocator).color = Vec3(0, 0, 1) * 0.5f;

    float f1Scale = 0.5f;
    f1.vertices.staticUpdate(allocator, { Vec3(-1, 0, -1) * f1Scale, Vec3(-1, 0, 1) * f1Scale, Vec3(1, 0, -1) * f1Scale, Vec3(1, 0, 1) * f1Scale });
    f1.normals.staticUpdate(allocator, { up, up, up, up });
    f2.vertices.staticUpdate(allocator, { Vec3(-1, 3, 0), Vec3(-1, 3, 2), Vec3(1, 3, 0), Vec3(1, 3, 2) });
    f2.normals.staticUpdate(allocator, { up, up, up, up });
    f3.vertices.staticUpdate(allocator, { Vec3(-10, -2, -10), Vec3(-10, -2, 10), Vec3(10, -2, -10), Vec3(10, -2, 10) });
    f3.normals.staticUpdate(allocator, { up, up, up, up });
    f4.vertices.staticUpdate(allocator, { Vec3(-10, 4, -10), Vec3(-10, 4, 10), Vec3(10, 4, -10), Vec3(10, 4, 10) });
    f4.normals.staticUpdate(allocator, { -up, -up, -up, -up });
    f5.vertices.allocateDynamic(allocator, 4);
    f5.normals.staticUpdate(allocator, { up, up, up, up });

    id_t id1 = objects.insertObject(f1);
    id_t id2 = objects.insertObject(f2);
    id_t id3 = objects.insertObject(f3);
    id_t id4 = objects.insertObject(f4);
    id_t id5 = objects.insertObject(f5);

    std::vector<Effect::Handle> effects1 { col1.handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle() };
    std::vector<Effect::Handle> effects2 { col2.handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle() };
    std::vector<Effect::Handle> effects3 { col3.handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle(), reflection.handle() };
    std::vector<Effect::Handle> effects4 { col4.handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle() };
    std::vector<Effect::Handle> effects5 { col5.handle(), ambientLight.handle(), spotLight.handle() };

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
    cubeComp.cubeMap = &pointLight.depthMap;
    cubeComp.positiveZ(Rect::bltr(-1, -1, 1, 1));
    refComp.texture = &reflection.texture;

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
    pawnIndices = IndexBuffer::upload(parser.indices, parser.n);

    void* addr = allocator.allocateStatic(&pawnTexCoords.ptr, 16, 2 * sizeof(float) * parser.vertexCount);
    std::memcpy(addr, parser.texCoords, 2 * sizeof(float) * parser.vertexCount);

    asset_istream jpegFile("QueenSideB/piece.jpg");
    JpegDecoder decoder;
    decoder.decode(jpegFile);
    pawnTexture = decoder.texture;

    pawnCol.texture = *pawnTexture;

    pawn.modelMatrix = Mat4x4::diagonal(Vec3(1, 1, 1) * 0.1f) * Mat4x4::rotate(-F_PI / 2.0f, Vec3(1, 0, 0)) * Mat4x4::translate(Vec3(+2, -16, 0));
    pawn.drawMode = DrawMode::Triangles;
    pawn.vertices.staticUpdate(allocator, { (Vec3*)parser.vertex, parser.vertexCount });
    pawn.normals.staticUpdate(allocator, { (Vec3*)parser.normals, parser.vertexCount });
    pawn.indices = *pawnIndices;
    id_t pawnId = objects.insertObject(pawn);

    allocator.stage();

    pawnCol.texCoords = pawnTexCoords.ptr;

    renderpass.addObject(id1, effects1);
    renderpass.addObject(id2, effects2);
    renderpass.addObject(id3, effects3);
    renderpass.addObject(id4, effects4);
    renderpass.addObject(id5, effects5);

    reflectionPass.addObject(id1, effects1);
    reflectionPass.addObject(id2, effects2);
    reflectionPass.addObject(id4, effects4);
    reflectionPass.addObject(id5, effects5);

    spotLightDepthPass.addObject(id1);
    spotLightDepthPass.addObject(id2);
    spotLightDepthPass.addObject(id3);
    spotLightDepthPass.addObject(id4);
    spotLightDepthPass.addObject(id5);

    for (int_t i = 0; i < 6; i++) {
        pointLightDepthPasses[i].addObject(id1);
        pointLightDepthPasses[i].addObject(id2);
        pointLightDepthPasses[i].addObject(id3);
        pointLightDepthPasses[i].addObject(id4);
    }

    renderpass.addObject(pawnId, { pawnCol.handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle() });
    reflectionPass.addObject(pawnId, { pawnCol.handle(), ambientLight.handle(), spotLight.handle(), pointLight.handle() });
    spotLightDepthPass.addObject(pawnId);
    for (int_t i = 0; i < 6; i++) {
        pointLightDepthPasses[i].addObject(pawnId);
    }

    glEnable(GL_DEPTH_TEST);
    for (int_t i = 0; i < 6; i++) {
        pointLightDepthPasses[i].draw(objects, allocator);
    }
    spotLightDepthPass.draw(objects, allocator);
    glDisable(GL_DEPTH_TEST);
}

template<Gapi G>
void TestRenderer<G>::updatePointLightData() {
    float m_near = 0.2f;
    float m_far = 15.0f;

    pointLight.fragmentUniform.dynamicUpdate(allocator) = {
        .color = Vec3(1, 0.3, 1) * 16.0f,
        .a = ((m_near + m_far) / (m_far - m_near) + 1.0f) * 0.5f - 0.001f,
        .b = 2.0f * m_near * m_far / (m_far - m_near) * 0.5f
    };
    pointLight.vertexUniform.dynamicUpdate(allocator) = { .lightPos = pointLightPosition };

    Mat4x4 pointLightProjection = Mat4x4::frustum(-m_near, m_near, -m_near, m_near, m_near, m_far);
    for (int_t i = 0; i < 6; i++) {
        Mat4x4 view;
        switch (CubeMap::Faces[i]) {
        case CubeMap::PosX:
            view = Mat4x4::look(pointLightPosition, Vec3(+1, 0, 0), Vec3(0, -1, 0));
            break;
        case CubeMap::NegX:
            view = Mat4x4::look(pointLightPosition, Vec3(-1, 0, 0), Vec3(0, -1, 0));
            break;
        case CubeMap::PosY:
            view = Mat4x4::look(pointLightPosition, Vec3(0, +1, 0), Vec3(0, 0, +1));
            break;
        case CubeMap::NegY:
            view = Mat4x4::look(pointLightPosition, Vec3(0, -1, 0), Vec3(0, 0, -1));
            break;
        case CubeMap::PosZ:
            view = Mat4x4::look(pointLightPosition, Vec3(0, 0, +1), Vec3(0, -1, 0));
            break;
        case CubeMap::NegZ:
            view = Mat4x4::look(pointLightPosition, Vec3(0, 0, -1), Vec3(0, -1, 0));
            break;
        }
        pointLightDepthPasses[i].viewProjectionMatrix = pointLightProjection * view;
    }
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

    textComp.updateContent(metrics, allocator);
    uiScene.metrics = metrics;
    uiScene.build(allocator);

    if (uiRenderer)
        uiRenderer->release(allocator);
    uiRenderer = std::make_unique<ui_renderer::Renderer>(3, newWidth, newHeight, allocator);
    uiRenderer->add(1, { 0, 300, 0, 200 });
    uiRenderer->add(1, { 100, 350, 100, 250 });
    uiTexture = Texture::color(newWidth, newHeight, Texture::Format::RGBA8888);
    refComp.texture = &uiTexture.value;

    allocator.debug();

    allocator.stage();

    std::mt19937 gen(0);

    CacheMap<uint32_t, 3> map;
    int_t step = 100;
    int_t items = 0;
    std::vector<std::pair<int_t, int_t>> v;
    for (int_t i = 0; i < 80000; i += step) {
        std::uniform_int_distribution<uint16_t> dist(0, 255);
        for (int_t j = 0; j < step; j++) {
            uint16_t key[3] { dist(gen), dist(gen), dist(gen) };
            if (!map.insertOrAssign(key, 0)) {
                items++;
            }
        }
        v.push_back({ items, map.setCount() * map.N });
    }
    std::stringstream str1;
    for (auto i : v) {
        str1 << i.first << ",";
    }
    std::stringstream str2;
    for (auto i : v) {
        str2 << i.second << ",";
    }
    log("{}", str1.str());
    log("{}", str2.str());
}

template<Gapi G>
void TestRenderer<G>::pointer(PointerEvent event) {
    if (!platform::rawInput()) {
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
                // goFullscreen(FullscreenConfig::DEFAULT);
                goFullscreen(getFullscreenConfig(getDisplayList().front(), 1920, 1080, 144));
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
    LogTimer timer;
    timer.disarm();
    uint64_t now = getNanos();
    if (now - lsecond >= NANOS_PER_SECOND) {
        log("FPS: {}", frameCounter);

        std::stringstream ss;
        ss << "FPS: " << frameCounter;
        textComp.text = ss.str();
        textComp.updateContent(uiScene.metrics, allocator);
        uiScene.build(allocator);

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
    renderpass.viewProjectionMatrix = projection * view;
    reflectionPass.viewProjectionMatrix = renderpass.viewProjectionMatrix * Mat4x4::reflection(Vec3(0, 1, 0), Vec3(0, -2, 0));

    Vec3& plp = pointLightPosition;
    plp += Vec3(plp.z * dt, plp.z * dt * 0.5f, -plp.x * dt);
    obj5_coords[0] = Vec3(plp.x - 0.3f, plp.y, plp.z - 0.3f);
    obj5_coords[1] = Vec3(plp.x - 0.3f, plp.y, plp.z + 0.3f);
    obj5_coords[2] = Vec3(plp.x + 0.3f, plp.y, plp.z - 0.3f);
    obj5_coords[3] = Vec3(plp.x + 0.3f, plp.y, plp.z + 0.3f);
    f5.vertices.dynamicUpdate(allocator, obj5_coords);
    updatePointLightData();
    //uiRenderer->update(allocator);

    allocator.pumpDynamicData();
    glWaitSync(lastSync, 0, GL_TIMEOUT_IGNORED);
    lastSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    for (int_t i = 0; i < 6; i++) {
        pointLightDepthPasses[i].draw(objects, allocator);
    }
    reflectionPass.draw(objects, allocator);
    renderpass.draw(objects, allocator);
    glDisable(GL_DEPTH_TEST);
    // glDisable(GL_CULL_FACE);

    if (navPointerId != POINTER_ID_INVALID) {
        Vec2 center = { navBaseX * 2.0f / uiScene.metrics.width() - 1.0f, navBaseY * -2.0f / uiScene.metrics.height() + 1.0f };
        Vec2 radius = { maxSpeed / navMult * 2.0f / uiScene.metrics.width(), maxSpeed / navMult * 2.0f / uiScene.metrics.height() };
        // util_drawCircle(center, radius, Vec4(0.7f, 0.7f, 0.7f, 1.0f));

        center = { (navBaseX + navD.x / navMult) * 2.0f / uiScene.metrics.width() - 1.0f, (navBaseY + navD.y / navMult) * -2.0f / uiScene.metrics.height() + 1.0f };
        radius *= 0.25f;
        // util_drawCircle(center, radius, Vec4(0.4f, 0.4f, 0.4f, 1.0f));
    }

    // cubeComp.draw(Framebuffer::DEFAULT, allocator);

    //uiRenderer->draw(uiTexture, allocator);
    //refComp.draw(Framebuffer::DEFAULT, allocator);
    textComp.draw(Framebuffer::DEFAULT, allocator);
}

template<Gapi G>
TestRenderer<G>::~TestRenderer() {
    uiScene.clear(allocator);
    /*ambientLightU.release(allocator);
    col1U.release(allocator);
    col2U.release(allocator);
    col3U.release(allocator);
    col4U.release(allocator);
    col5U.release(allocator);
    pawnTexCoords.release(allocator);
    spotLightFU.release(allocator);
    spotLightVU.release(allocator);
    reflectionU.release(allocator);*/
}

INSTANTIATE_G_API_CLASS(TestRenderer);
