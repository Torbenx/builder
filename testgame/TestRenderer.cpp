#include "TestRenderer.hpp"
#include <Renderer.hpp>
#include <containers/CacheMap.hpp>
#include <containers/RangeAllocator.hpp>
#include <fstream>
#include <input/keycodes.hpp>
#include <math/constants.hpp>
#include <math/geometry.hpp>
#include <numeric>
#include <random>
#include <resource/JpegDecoder.hpp>
#include <resource/ObjParser.hpp>
#include <resource/asset_istream.hpp>
#include <script/events.hpp>
#include <string>
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

void runShadergenTest();

template<Gapi G>
TestRenderer<G>::TestRenderer()
    : up(0, 1, 0)
    , look(up, platform::isAndroid())
    , pos(7, 1, 0)
    , renderpass(&Framebuffer::DEFAULT)
    , spotLightDepthPass(&spotLightFB)
    , pointLightDepthPasses { &pointLightFBs[0], &pointLightFBs[1], &pointLightFBs[2], &pointLightFBs[3], &pointLightFBs[4], &pointLightFBs[5] }
    , reflectionPass(&reflectionFB)
    , pointLightPosition(0, 1, -2) {

    log("=========================================");

    runShadergenTest();

    log("=========================================");

    []() {
        using namespace newmath;
        auto m = translate({ 2.0f, 1.0f, 0.0f }) * reflectAbout({ { 0.0f, 0.0f, 1.0f }, -2.0f }) * rotateAbout({ 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, 0.3f);
        auto id = inverse(m) * m;
        auto v = newmath::Vec3 { 1.0f, 2.0f, 3.0f };
        v = inverse(m)(m(v));
        log("\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n",
            id.rotation.matrix[0], id.rotation.matrix[3], id.rotation.matrix[6], id.translation.x,
            id.rotation.matrix[1], id.rotation.matrix[4], id.rotation.matrix[7], id.translation.y,
            id.rotation.matrix[2], id.rotation.matrix[5], id.rotation.matrix[8], id.translation.z);
        log("{} {} {}", v.x, v.y, v.z);
    }();

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

    f1.modelMatrix = Mat4x4::Id;
    f2.modelMatrix = Mat4x4::Id;
    f3.modelMatrix = Mat4x4::Id;
    f4.modelMatrix = Mat4x4::Id;
    f5.modelMatrix = Mat4x4::Id;
    f1.drawMode = DrawMode::TriangleStrip;
    f2.drawMode = DrawMode::TriangleStrip;
    f3.drawMode = DrawMode::TriangleStrip;
    f4.drawMode = DrawMode::TriangleStrip;
    f5.drawMode = DrawMode::TriangleStrip;
    f1.indices = IndexBuffer::withoutData(4);
    f2.indices = IndexBuffer::withoutData(4);
    f3.indices = IndexBuffer::withoutData(4);
    f4.indices = IndexBuffer::withoutData(4);
    f5.indices = IndexBuffer::withoutData(4);

    Framebuffer::DEFAULT.clearColor(0, 0, 0, 1);

    ambientLight.fragmentUniform.staticUpdate(&allocator).color = Vec3(1, 1, 1) * 1.0f;

    spotLightDepthMap = Texture::depth(2048, 2048, Texture::DepthBits::D16);
    spotLightFB.bind();
    spotLightFB.attach(spotLightDepthMap);
    spotLightFB.viewport(2048, 2048);
    Mat4x4 mat = Mat4x4::make(
        { 0.5f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.5f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.5f, 0.0f,
            0.5f, 0.5f, 0.5f - 0.001f, 1.0f });
    spotLightDepthPass.viewProjectionMatrix = Mat4x4::frustumVH(F_PI * 0.2f, F_PI * 0.4f, 0.7f, 30.0f) * Mat4x4::look(Vec3(0, 3.4f, 0), Vec3(0, -1, 0), Vec3(1, 0, 0));
    spotLight.fragmentUniform.staticUpdate(&allocator) = { .color = Vec3(1, 1, 1) * 8.0f, .lightPos = Vec3(0, 3.4f, 0) };
    spotLight.vertexUniform.staticUpdate(&allocator) = { .mvpMatrix = mat * spotLightDepthPass.viewProjectionMatrix };
    spotLight.depthMap = *spotLightDepthMap;

    pointLightDepthMap = CubeMap::depth(1024, 1024, Texture::DepthBits::D16);
    for (int_t i = 0; i < 6; i++) {
        pointLightFBs[i].bind();
        pointLightFBs[i].attach(pointLightDepthMap, CubeMap::Faces[i]);
        pointLightFBs[i].viewport(1024, 1024);
    }
    pointLight.depthMap = *pointLightDepthMap;
    pointLight.fragmentUniform.allocateDynamic(&allocator);
    pointLight.vertexUniform.allocateDynamic(&allocator);
    updatePointLightData();

    reflectionDepthBuffer = Texture::depth(2048, 2048, Texture::DepthBits::D16);
    reflectionTexture = Texture::color(2048, 2048, Texture::Format::RGBA8888);
    reflectionFB.bind();
    reflectionFB.attach(reflectionDepthBuffer);
    reflectionFB.attach(reflectionTexture);
    reflectionFB.viewport(2048, 2048);
    reflection.fragmentUniform.staticUpdate(&allocator).factor = 0.7f;
    reflection.texture = *reflectionTexture;
    reflectionFB.clearColor(0, 0, 0, 1);

    col1.fragmentUniform.staticUpdate(&allocator).color = Vec3(1, 0, 0) * 0.5f;
    col2.fragmentUniform.staticUpdate(&allocator).color = Vec3(0, 1, 0) * 0.5f;
    col3.fragmentUniform.staticUpdate(&allocator).color = Vec3(1, 1, 1) * 0.1f;
    col4.fragmentUniform.staticUpdate(&allocator).color = Vec3(1, 1, 0) * 0.5f;
    col5.fragmentUniform.staticUpdate(&allocator).color = Vec3(0, 0, 1) * 0.5f;

    float f1Scale = 0.5f;
    f1.vertices.staticUpdate({ Vec3(-1, 0, -1) * f1Scale, Vec3(-1, 0, 1) * f1Scale, Vec3(1, 0, -1) * f1Scale, Vec3(1, 0, 1) * f1Scale }, &allocator);
    f1.normals.staticUpdate({ up, up, up, up }, &allocator);
    f2.vertices.staticUpdate({ Vec3(-1, 3, 0), Vec3(-1, 3, 2), Vec3(1, 3, 0), Vec3(1, 3, 2) }, &allocator);
    f2.normals.staticUpdate({ up, up, up, up }, &allocator);
    f3.vertices.staticUpdate({ Vec3(-10, -2, -10), Vec3(-10, -2, 10), Vec3(10, -2, -10), Vec3(10, -2, 10) }, &allocator);
    f3.normals.staticUpdate({ up, up, up, up }, &allocator);
    f4.vertices.staticUpdate({ Vec3(-10, 4, -10), Vec3(-10, 4, 10), Vec3(10, 4, -10), Vec3(10, 4, 10) }, &allocator);
    f4.normals.staticUpdate({ -up, -up, -up, -up }, &allocator);
    f5.vertices.allocateDynamic(4, &allocator);
    f5.normals.staticUpdate({ up, up, up, up }, &allocator);

    id_t id1 = objects.insertObject(f1);
    id_t id2 = objects.insertObject(f2);
    id_t id3 = objects.insertObject(f3);
    id_t id4 = objects.insertObject(f4);
    id_t id5 = objects.insertObject(f5);

    std::vector<Effect::Handle> effects1 {
        col1.handle(),
        ambientLight.handle(),
    };
    std::vector<Effect::Handle> effects2 {
        col2.handle(),
        ambientLight.handle(),
    };
    std::vector<Effect::Handle> effects3 { col3.handle(), ambientLight.handle(), reflection.handle() };
    std::vector<Effect::Handle> effects4 {
        col4.handle(),
        ambientLight.handle(),
    };
    std::vector<Effect::Handle> effects5 {
        col5.handle(),
        ambientLight.handle(),
    };

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

    textBackground.color = Vec4(0, 1, 0, 1);

    cubeComp.farDepth = 1.0f;
    cubeComp.cubeMap = &pointLight.depthMap;
    cubeComp.positiveZ(Rect::bltr(-1, -1, 1, 1));
    refComp.texture = &reflection.texture;

    uiScene.layout(&textComp)
        .alignTop()
        .alignLeft()
        .wrapContentWidth()
        .wrapContentHeight();
    uiScene.layout(&textBackground)
        .align(&textComp, TypedSize::pixels(-5.0f));
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
    pawn.vertices.staticUpdate({ (Vec3*)parser.vertex, parser.vertexCount }, &allocator);
    pawn.normals.staticUpdate({ (Vec3*)parser.normals, parser.vertexCount }, &allocator);
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
        pointLightDepthPasses[i].draw(objects, &allocator);
    }
    spotLightDepthPass.draw(objects, &allocator);
    glDisable(GL_DEPTH_TEST);

    using namespace render3d;

    stx::get<SG::ObjectNode>(o1).effects = effects1;
    stx::get<SG::ObjectNode>(o1).object = &f1;
    stx::get<SG::BoundingNode>(o1).bound = { { 0.0f, 0.0f, 0.0f }, 2.0f * f1Scale };
    stx::get<SG::ObjectNode>(o2).effects = effects2;
    stx::get<SG::ObjectNode>(o2).object = &f2;
    stx::get<SG::ObjectNode>(o3).effects = effects3;
    stx::get<SG::ObjectNode>(o3).object = &f3;
    stx::get<SG::ObjectNode>(o4).effects = effects4;
    stx::get<SG::ObjectNode>(o4).object = &f4;
    stx::get<SG::ObjectNode>(o5).effects = effects5;
    stx::get<SG::ObjectNode>(o5).object = &f5;
    stx::get<SG::CompoundNode>(unreflectedItems).addSubSequence(o1);
    stx::get<SG::CompoundNode>(unreflectedItems).addSubSequence(o2);
    stx::get<SG::CompoundNode>(unreflectedItems).addSubSequence(o4);
    stx::get<SG::CompoundNode>(unreflectedItems).addSubSequence(o5);
    stx::get<SG::CompoundNode>(reflectedItems).addSubSequence(o3);

    stx::get<SG::TransformNode>(reflectionSeq).transform = Mat4x4::reflection(Vec3(0, 1, 0), Vec3(0, -2, 0));
    stx::get<SG::CompoundNode>(reflectionSeq).addSubSequence(unreflectedItems);

    stx::get<SG::TransformNode>(cameraSeq).transform = Mat4x4::look(pos, look.dir, up);
    stx::get<SG::CompoundNode>(cameraSeq).addSubSequence(unreflectedItems);
    stx::get<SG::CompoundNode>(cameraSeq).addSubSequence(reflectedItems);
    // stx::get<SG::CompoundNode>(cameraSeq).addCompoundSequence(reflectionSeq);

    renderpass3d = renderer.createRenderpass(cameraSeq);

    // lightChildren = std::make_unique<SG::CompoundNode>();
    // lightChildren->addChild(unreflectedItems.get());
    // lightChildren->addChild(reflectedItems.get());
    // spotLightTransform = std::make_unique<SG::TransformNode>();
    // spotLightTransform->child = lightChildren.get();
    // spotLightTransform->transform = Mat4x4::look(Vec3(0, 3.4f, 0), Vec3(0, -1, 0), Vec3(1, 0, 0));

    /*RTUpdater updater;
    cameraTree = static_cast<RT::CompoundNode*>(updater.dispatchUpdate(cameraTransform.get(), nullptr));
    unreflectedItems->clearUpdates();
    reflectedItems->clearUpdates();
    cameraChildren->clearUpdates();
    lightChildren->clearUpdates();*/
}

template<Gapi G>
void TestRenderer<G>::updatePointLightData() {
    float m_near = 0.2f;
    float m_far = 15.0f;

    pointLight.fragmentUniform.dynamicUpdate(&allocator) = {
        .color = Vec3(1, 0.3, 1) * 16.0f,
        .a = ((m_near + m_far) / (m_far - m_near) + 1.0f) * 0.5f - 0.001f,
        .b = 2.0f * m_near * m_far / (m_far - m_near) * 0.5f
    };
    pointLight.vertexUniform.dynamicUpdate(&allocator) = { .lightPos = pointLightPosition };

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

    {
        float near = 0.1f;
        float height = 2.0f * near * std::tan(fovV / 2.0f);
        float width = height * as;
        using namespace render3d;
        stx::get<SG::PortalNode>(cameraSeq).points.clear();
        stx::get<SG::PortalNode>(cameraSeq).points.push_back({ +width / 2.0f, -height / 2.0f, -near });
        stx::get<SG::PortalNode>(cameraSeq).points.push_back({ +width / 2.0f, +height / 2.0f, -near });
        stx::get<SG::PortalNode>(cameraSeq).points.push_back({ -width / 2.0f, +height / 2.0f, -near });
        stx::get<SG::PortalNode>(cameraSeq).points.push_back({ -width / 2.0f, -height / 2.0f, -near });
        /*stx::get<SG::PortalNode>(cameraSeq).planes.planes.clear();
        stx::get<SG::PortalNode>(cameraSeq).planes.planes.push_back({ { 0.0f, std::cos(fovV / 2.0f), -std::sin(fovV / 2.0f) }, 0.0f });
        stx::get<SG::PortalNode>(cameraSeq).planes.planes.push_back({ { 0.0f, -std::cos(fovV / 2.0f), -std::sin(fovV / 2.0f) }, 0.0f });
        stx::get<SG::PortalNode>(cameraSeq).planes.planes.push_back({ Vec3::normalize({ std::cos(fovV / 2.0f), 0.0f, -std::sin(fovV / 2.0f) * as }), 0.0f });
        stx::get<SG::PortalNode>(cameraSeq).planes.planes.push_back({ Vec3::normalize({ -std::cos(fovV / 2.0f), 0.0f, -std::sin(fovV / 2.0f) * as }), 0.0f });*/
    }

    ScreenMetrics metrics;
    metrics.x.size = newWidth;
    metrics.y.size = newHeight;

    textComp.updateContent(metrics, &allocator);
    uiScene.metrics = metrics;
    uiScene.build(&allocator);

    /*if (uiRenderer)
        uiRenderer->release();
    uiRenderer = std::make_unique<ui_renderer::Renderer>(3, newWidth, newHeight, allocator);
    uiRenderer->add(1, { 0, 300, 0, 200 });
    uiRenderer->add(1, { 100, 350, 100, 250 });*/
    uiTexture = Texture::color(newWidth, newHeight, Texture::Format::RGBA8888);
    refComp.texture = &uiTexture.value;

    allocator.debug();

    allocator.stage();

    std::mt19937 gen(2);

    /*constexpr int_t DIM = 1;
    CacheMapBase<uint32_t, AVX2CacheSet<uint32_t, DIM>, DIM> map;
    int_t step = 4;
    int_t items = 0;
    std::vector<std::pair<int_t, int_t>> v;
    for (int_t i = 0; i < 2000; i += step) {
        std::uniform_int_distribution<uint16_t> dist(0, (i + 8) * 4);
        for (int_t j = 0; j < step; j++) {
            uint16_t key[DIM];
            for (int_t k = 0; k < DIM; k++) {
                key[k] = dist(gen);
            }
            if (!map.insertOrAssign(key, 0)) {
                items++;
            }
        }
        v.push_back({ items, map.setCount() * map.N });
    }
    std::stringstream str;
    for (auto i : v) {
        str << i.first << ",";
    }
    log("{}", str.str());
    str.str({});
    for (auto i : v) {
        str << i.second << ",";
    }
    log("{}", str.str());
    str.str({});
    for (auto i : v) {
        str << ((float)i.first / i.second) << ",";
    }
    log("{}", str.str());*/
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

#include <3d/BindingSolver.hpp>

template<Gapi G>
void TestRenderer<G>::rawPointer(int_t dx, int_t dy) {
    look.move(dx, dy);
    return;

    // log("\n\n--------------------------------------");
    /*sparse::Inverse inv;
    {
        LogTimer timer;
        uint32_t matSize = 4000;
        for (uint32_t i = 0; i < matSize; i++) {
            sparse::Vector col = sparse::allocateEmpty(3);
            sparse::Vector row = sparse::allocateEmpty(3);
            //if (i > 0) {
            //    col.appendEntry({ i - 1, -0.5f });
            //    row.appendEntry({ i - 1, -0.5f });
            //}
            inv.expand(col, row, 1.0f);
        }
    }
    // inv.inverse.debug();
    // inv.inverseTranspose.debug();

    sparse::Vector col = sparse::allocateEmpty(2);
    col.appendEntry({ 3, -1.0f });
    col.appendEntry({ 4, 1.0f });
    inv.replaceColumn(4, col);

    // inv.inverse.debug();
    // inv.inverseTranspose.debug();*/

    BindingSolver solver;
    for (int_t i = 0; i < 1600; i++) {
        solver.addUsage(std::array<uint32_t, 4> {
            solver.addResource(),
            solver.addResource(),
            solver.addResource(),
            solver.addResource() });
    }
    /*uint32_t r0 = solver.addResource();
    uint32_t r1 = solver.addResource();
    uint32_t r2 = solver.addResource();
    uint32_t r3 = solver.addResource();
    solver.addUsage(std::array<uint32_t, 4> {
        r0,
        r2,
        solver.addResource(),
        solver.addResource() });
    solver.addUsage(std::array<uint32_t, 4> {
        r1,
        r2,
        solver.addResource(),
        solver.addResource() });
    solver.addUsage(std::array<uint32_t, 4> {
        r0,
        r3,
        solver.addResource(),
        solver.addResource() });
    solver.addUsage(std::array<uint32_t, 4> {
        r1,
        r3,
        solver.addResource(),
        solver.addResource() });*/
    {
        LogTimer timer;
        while (solver.iterate()) { }
    }
    // solver.debug();
    // solver.A.debug();
    /*VDUMP(solver.iterations);
    constexpr int_t clocksCount = std::tuple_size_v<decltype(solver.clocks)> - 1;
    for (int_t i = 0; i < clocksCount; i++) {
        log("{}: {}", i, (solver.clocks[i + 1] - solver.clocks[i]) / solver.iterations);
    }
    log("total: {}", (solver.clocks[clocksCount - 1] - solver.clocks[0]) / solver.iterations);*/
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

#include <math/ConvexPolygon.hpp>

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

        textComp.updateContent(uiScene.metrics, &allocator);
        uiScene.build(&allocator);
        allocator.stage();

        lsecond = now;
        frameCounter = 0;

        VDUMP(renderpass3d->objectItems.size());
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
    // cameraTransform->transform = view;

    Vec3& plp = pointLightPosition;
    plp += Vec3(plp.z * dt, plp.z * dt * 0.5f, -plp.x * dt);
    obj5_coords[0] = Vec3(plp.x - 0.3f, plp.y, plp.z - 0.3f);
    obj5_coords[1] = Vec3(plp.x - 0.3f, plp.y, plp.z + 0.3f);
    obj5_coords[2] = Vec3(plp.x + 0.3f, plp.y, plp.z - 0.3f);
    obj5_coords[3] = Vec3(plp.x + 0.3f, plp.y, plp.z + 0.3f);
    f5.vertices.dynamicUpdate(obj5_coords, &allocator);
    updatePointLightData();
    // uiRenderer->update(allocator);

    allocator.pumpDynamicData();
    glWaitSync(lastSync, 0, GL_TIMEOUT_IGNORED);
    lastSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    for (int_t i = 0; i < 6; i++) {
        // pointLightDepthPasses[i].draw(objects);
    }

    // reflectionPass.draw(objects);
    // renderpass.draw(objects);

    Framebuffer::DEFAULT.bind();
    Framebuffer::DEFAULT.clear();
    stx::get<render3d::SG::TransformNode>(cameraSeq).transform = view;
    renderer.update();
    renderpass3d->render(&allocator, &Framebuffer::DEFAULT, projection);

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

    // cubeComp.draw(Framebuffer::DEFAULT);

    // uiRenderer->draw(uiTexture);
    // refComp.draw(Framebuffer::DEFAULT);
    textBackground.draw(Framebuffer::DEFAULT, &allocator);
    textComp.draw(Framebuffer::DEFAULT, &allocator);

    /*
    ConvexPolygon screen = ConvexPolygon::fromOrderedVertices({ { 1.0f, 1.0f }, { -1.0f, 1.0f }, { -1.0f, -1.0f }, { 1.0f, -1.0f } });
    std::array<Vec3, 4> worldVerts = { Vec3(-10, -2, -10), Vec3(-10, -2, 10), Vec3(10, -2, 10), Vec3(10, -2, -10) };
    Vec3 prevViewSpace = (view * Vec4(worldVerts.back(), 1.0f)).xyz;
    for (int_t i = 0; i < 4; i++) {
        //const_cast<Vec3&>(worldVerts[i]).debug();
        Vec3 viewSpace = (view * Vec4(worldVerts[i], 1.0f)).xyz;
        Vec3 normal = Vec3::normalize(Vec3::cross(viewSpace, prevViewSpace));
        //normal.debug();
        float w = -Vec3::dot(normal, viewSpace);
        Vec4 plane = Vec4(normal, w) * Mat4x4::invert(projection);
        //plane.debug();
        Vec4 screenSpace = projection * Vec4(viewSpace, 1.0f);
        //log("check: {}", Vec2::dot(plane.xy, screenSpace.xy / screenSpace.w) + plane.w);
        screen.extendBy({ plane.xy, plane.w });

        prevViewSpace = viewSpace;
    }
    VDUMP(screen.vertices.size());*/
}

template<Gapi G>
TestRenderer<G>::~TestRenderer() {
    uiScene.clear(&allocator);
}

INSTANTIATE_G_API_CLASS(TestRenderer);
