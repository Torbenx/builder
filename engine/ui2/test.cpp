#include "Instance.hpp"
#include "Stylable.hpp"
#include "box.hpp"
#include "equation_code.hpp"
#include "event/InlineCallback.hpp"
#include "event/conditions.hpp"
#include "event/Event.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "opengl/core.hpp"
#include "platform/Window.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "platform/windows/core.hpp"
#include "platform/windows/glcore.hpp"
#include "util/LogTimer.hpp"
#include <memory>
#include <sstream>

namespace {

struct BasicAtlasAllocator : ui::AtlasAllocator {
    ui::TextureHandle allocateAtlas(ui::IntVec size) override {
        if (size.x() == 0)
            size.x() = 16384;
        if (size.y() == 0)
            size.y() = 2048;
        else
            size.y() = size.y() * 2;
        uint32_t* data = std::allocator<uint32_t>().allocate(size.x() * size.y());
        return { data, size };
    }
    void deallocateAtlas(ui::TextureHandle texture) override {
        std::allocator<uint32_t>().deallocate(texture.data, texture.dimensions.x() * texture.dimensions.y());
    }
};

DECLARE_STYLE_KEY(DefaultBoxStyle, ui::PaddingStyle);

enum class ColorChannel {
    Red,
    Green,
    Blue,
};

DECLARE_STYLE_KEY(Channel1, ColorChannel);
DECLARE_STYLE_KEY(Channel2, ColorChannel);
using TestStyle = ui::SmallStyleTable<Channel1, Channel2>;
DECLARE_STYLE_KEY(DefaultTestStyle, TestStyle);

struct TestRenderer : ui::BasicRenderer<TestRenderer, TestStyle> {
    void render(const ui::TextureState& texture, const TestStyle& style) {
        auto channelToShift = [](ColorChannel channel) -> int {
            switch (channel) {
            case ColorChannel::Red:
                return 16;
            case ColorChannel::Green:
                return 8;
            case ColorChannel::Blue:
                return 0;
            default:
                VERIFY_NOT_REACHED(Ui);
            }
        };
        int shift1 = channelToShift(style.get<Channel1>());
        int shift2 = channelToShift(style.get<Channel2>());
        ui::TextureView view = texture.texture;
        ui::IntVec dim = texture.dimensions;
        for (int_t j = 0; j < dim.y(); j++) {
            for (int_t i = 0; i < dim.x(); i++) {
                float fracX = i / (float)(dim.x() - 1);
                float fracY = j / (float)(dim.y() - 1);
                uint32_t c1 = fracX * 255;
                uint32_t c2 = (1.0f - fracX) * 255;
                uint32_t c3 = fracY * fracY * 255;
                view[j][i] = 0xff'00'00'00 | (c3 << 24) | (c1 << shift1) | (c2 << shift2);
            }
        }
    }
};
evt::Script uiTestInit() {
    co_await (evt::OnExecutor(ui::coreExecutor()) && evt::Defer());

    auto bigTable = ui::Ref<ui::BigStyleTable>::allocate();
    {
        ui::Ref<TestStyle>& testStyle = bigTable.get<DefaultTestStyle>();
        testStyle.get<Channel1>() = ColorChannel::Green;
        testStyle.get<Channel2>() = ColorChannel::Blue;
        ui::Ref<ui::PaddingStyle>& boxStyle = bigTable.get<DefaultBoxStyle>();
        boxStyle.get<ui::Padding>() = 50_px;
    }
    auto bigStyle = bigTable.finalize();

    ui::Padded<ui::ContentBox<TestRenderer>> box1;
    ui::RectEqn box1Eqn;

    using namespace ui::code;
    ui::RectVarHandle screen = ui::instance()->screen();
    box1Eqn = box1.box() == screen;
    /*box1Eqn.left() = box1.outerBox.left() + box1.outerBox.right() == screen.left() + screen.right();
    box1Eqn.bottom() = box1.outerBox.bottom() + box1.outerBox.top() == screen.bottom() + screen.top();
    box1Eqn.right() = box1.innerBox.width() / 16.0f == box1.innerBox.height() / 9.0f;
    box1Eqn.top() = box1.innerBox.width() == 0.5f * screen.width();*/

    box1.setTable(bigStyle);
    box1.padded().setStyle(ui::key<DefaultBoxStyle>());
    box1.renderer().setStyle(ui::key<DefaultTestStyle>());
    
    {
        auto& boxStyle = box1.padded().mutateLocal();
        boxStyle.get<ui::Padding>().x() = 10_px;
        boxStyle.get<ui::Padding>().y() = 10_px;
        auto& style = box1.renderer().mutateLocal();
        style.get<Channel1>() = ColorChannel::Red;
    }

    co_await "nothing"_ev;

    co_return;
}

struct UiTest : plt::WindowStateCallback {

    std::unique_ptr<uint32_t[]> framebuffer;
    ui::IntVec frameSize;

    void onResize(plt::Window* window) override {
        ui::IntVec windowSize { (int)window->width(), (int)window->height() };
        ui::instance()->resize(windowSize, 0.5f);

        ui::IntVec windowBlockSize = (windowSize + 15) / 16;
        frameSize = windowBlockSize * 16;
        framebuffer = std::make_unique<uint32_t[]>(frameSize.x() * frameSize.y());
    }

    uint64_t lastFrame = 0;
    int_t frames = 0;
    void draw(plt::Window* window, ui::Instance& layout) {
        layout.doTick();
        uint64_t now = getNanos();
        if (now - lastFrame >= 1'000'000'000) {
            lastFrame = now;
            log("FPS: {}", frames);
            frames = 0;
        }
        frames += 1;

        ui::TextureView frameView { framebuffer.get(), (window->width() + 15) / 16 * 16 };
        uint32_t background_color = 0xff'ff'ff'ff;
        for (int_t j = 0; j < window->height(); j++) {
            for (int_t i = 0; i < window->width(); i++) {
                frameView[j][i] = background_color >> 8;
            }
        }
        auto multiply = [](uint32_t in, uint32_t alpha) {
            return (((in * alpha / 255) & 0xff0000) | (((in & 0xff00) * alpha / 255) & 0xff00) | ((in & 0xff) * alpha / 255));
        };
        int_t blockCount = layout.outputOffsets.size() / (layout.outputLayers + 1);
        for (int_t k = 0; k < blockCount; k++) {
            ui::IntVec* block = layout.outputOffsets.data() + k * (layout.outputLayers + 1);
            ui::IntVec screenPos = block[0];
            for (int_t j = 0; j < 16; j++) {
                for (int_t i = 0; i < 16; i++) {
                    uint32_t color = (frameView + screenPos)[j][i];
                    for (int_t layer = 0; layer < layout.outputLayers; layer++) {
                        ui::IntVec layerPos = block[layer + 1];
                        if (layerPos == ui::IntVec {})
                            break;
                        uint32_t in = (layout.atlasView() + layerPos)[j][i];
                        uint32_t alpha = in >> 24;
                        color = multiply(in & 0xff'ff'ff, alpha) + multiply(color, 255 - alpha);
                        // color = in & 0xff'ff'ff;
                    }
                    (frameView + screenPos)[j][i] = color;
                }
            }
        }

        auto* impl = (plt::windows::WindowImpl*)window;
        BITMAPINFO bitmapInfo {
            .bmiHeader = {
                .biSize = sizeof(BITMAPINFOHEADER),
                .biWidth = frameSize.x(),
                .biHeight = frameSize.y(),
                .biPlanes = 1,
                .biBitCount = 32,
                .biCompression = BI_RGB,
                .biSizeImage = 0,
                .biXPelsPerMeter = 0,
                .biYPelsPerMeter = 0,
                .biClrUsed = 0,
                .biClrImportant = 0,
            },
            .bmiColors = {},
        };
        auto dc = impl->deviceContext();
        int linesProcessed = SetDIBitsToDevice(dc, 0, 0, window->width(), window->height(), 0, 0, 0, window->height(), framebuffer.get(), &bitmapInfo, DIB_RGB_COLORS);
        WIN_VERIFY(linesProcessed != 0);
    }
};

}

namespace ui {

void test() {
    /*sparse::LUDecomposition lu;
    lu.u.appendColumn(sparse::StaticVector({ { 1, 1.0f }, { 3, -0.5f } }).copy());
    lu.u.appendColumn(sparse::StaticVector({ { 0, 0.5f }, { 1, 1.0f } }).copy());
    lu.u.appendColumn(sparse::allocateEmpty());
    lu.u.appendColumn(sparse::StaticVector({ { 0, -0.5f }, { 3, 3.0f } }).copy());
    lu.decompose(4);
    lu.l.debug();
    lu.u.debug();
    float result[4] = {};
    lu.inverseTranspose(std::array<float, 4> { 1.0f, 2.0f, 1000.0f, 3.0f }.data(), result);
    log("result = ({}, {}, {}, {})", result[0], result[1], result[2], result[3]);*/
    evt::test();

    std::unique_ptr<plt::Window> window = plt::Window::construct();
    BasicAtlasAllocator atlasAlloc;
    Instance instance(&atlasAlloc);
    setInstance(&instance);
    uiTestInit();
    UiTest callback;
    window->setDimensions(700, 700);
    window->setStateCallback(&callback);
    window->setTitle("UiTest");
    // gl::initialize();
    // gl::attachWindow(window.get());
    window->initialize();
    while (!window->destoryed()) {
        plt::processEvents([&callback, &window]() { callback.draw(window.get(), *ui::instance()); });
        if (window->initialized())
            callback.draw(window.get(), instance);
    }
}

}