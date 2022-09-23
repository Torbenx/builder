#include "common.hpp"
#include "Instance.hpp"
#include "equation_code.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "opengl/core.hpp"
#include "platform/Window.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "platform/windows/core.hpp"
#include "platform/windows/glcore.hpp"
#include <memory>
#include <sstream>
#include "event/Awaitable.hpp"

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

struct LayoutBox {
    ui::RectVar outerBox;
    ui::RectVar innerBox;
    ui::Rect padding;
    ui::RectEqn paddingEqn;
    LayoutBox() {
        using namespace ui::code;
        paddingEqn = extendBy(innerBox, padding) == outerBox;
    }
    void updatePaddings() {
        paddingEqn.updateShift(padding);
    }
};

DECLARE_STYLE_KEY(TestFlag, bool);
using TestStyle = ui::SmallStyleTable<TestFlag>;

struct UiTest : plt::WindowStateCallback {

    LayoutBox box1;
    ui::RectEqn box1Eqn;
    std::unique_ptr<uint32_t[]> framebuffer;
    ui::IntVec frameSize;

    UiTest() {
        using namespace ui::code;
        //box1Eqn = box1.outerBox == ui::instance()->screen();
        ui::RectVarHandle screen = ui::instance()->screen();
        box1Eqn.left() = box1.outerBox.left() + box1.outerBox.right() == screen.left() + screen.right();
        box1Eqn.bottom() = box1.outerBox.bottom() + box1.outerBox.top() == screen.bottom() + screen.top();
        box1Eqn.right() = box1.innerBox.width() / 16.0f == box1.innerBox.height() / 9.0f;
        box1Eqn.top() = box1.innerBox.width() == 0.5f * screen.width();
    }

    void onInitialize(plt::Window*) override {
    }

    struct TestRenderer : ui::BasicRenderer<TestRenderer, TestStyle> {
        void render(const ui::TextureState& texture, const TestStyle&) {
            ui::TextureView view = texture.texture;
            ui::IntVec dim = texture.dimensions;
            VDUMP((void*)view.data);
            VDUMP(dim.x());
            VDUMP(dim.y());
            for (int_t j = 0; j < dim.y(); j++) {
                for (int_t i = 0; i < dim.x(); i++) {
                    float fracX = i / (float)(dim.x() - 1);
                    float fracY = j / (float)(dim.y() - 1);
                    uint32_t c1 = fracX * 255;
                    uint32_t c2 = (1.0f - fracX) * 255;
                    uint32_t c3 = fracY * fracY * 255;
                    view[j][i] = 0xff'00'00'00 | (c3 << 24) | (c1 << 16) | c2;
                }
            }
        }
    };
    void onVisible(plt::Window* window) override {
        box1.padding.y() = 10_px;
        box1.padding.x() = 10_px;
        box1.updatePaddings();

        ui::CompoundStylable<ui::TextRenderer, TestRenderer> renderer;
        ui::Ptr<ui::SmallStyleTable<ui::TextSize, TestFlag>> style;
        renderer.setStyle(style);
        ui::draw(renderer.get<TestRenderer>(), box1.innerBox);

        ui::IntVec windowSize { (int)window->width(), (int)window->height() };
        ui::instance()->resize(windowSize, 0.5f);
        ui::instance()->debug(ui::instance()->screen());
        ui::instance()->debug(box1.outerBox);
        ui::instance()->debug(box1.innerBox);

        ui::IntVec windowBlockSize = (windowSize + 15) / 16;
        frameSize = windowBlockSize * 16;
        framebuffer = std::make_unique<uint32_t[]>(frameSize.x() * frameSize.y());
    }

    void draw(plt::Window* window, ui::Instance& layout) {
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
                        color = multiply(in >> 8, alpha) + multiply(color, 255 - alpha);
                        // color = in >> 8;
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
    evt::test_awaitable();

    Ref<TextStyle> style;
    style.get<TextSize>() = ref<TextSize>();
    VDUMP(style.get<TextSize>().isPtr());

    std::unique_ptr<plt::Window> window = plt::Window::construct();
    BasicAtlasAllocator atlasAlloc;
    Instance instance(&atlasAlloc);
    setInstance(&instance);
    UiTest callback;
    window->setDimensions(700, 700);
    window->setStateCallback(&callback);
    window->setTitle("UiTest");
    // gl::initialize();
    // gl::attachWindow(window.get());
    window->initialize();
    while (!window->destoryed()) {
        plt::processEvents();
        callback.draw(window.get(), instance);
    }
}

}