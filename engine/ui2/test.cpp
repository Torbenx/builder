#include "Layout.hpp"
#include "SmallStyle.hpp"
#include "StyleKey.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "opengl/core.hpp"
#include "platform/Window.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "platform/windows/core.hpp"
#include "platform/windows/glcore.hpp"
#include <memory>
#include <sstream>

namespace {

struct UiTest : plt::WindowStateCallback {

    ui::Layout layout;
    ui::LayoutBox box1;
    ui::RectEqn box1Eqn;

    UiTest()
        : box1(layout) {
        box1Eqn = layout.align(box1.outerBox, layout.screen());
    }

    void onInitialize(plt::Window*) override {
    }

    struct TestRenderer : ui::RendererBase {
        void updateTexture(ui::TextureView view, ui::IntVec dim) {
            VDUMP((void*)view.data);
            VDUMP(dim.x());
            VDUMP(dim.y());
            for (int_t j = 0; j < dim.y(); j++) {
                for (int_t i = 0; i < dim.x(); i++) {
                    view.data[i + j * view.stride] = 0xff0000ff;
                }
            }
        }
    };
    void onVisible(plt::Window* window) override {
        using namespace ui::code;
        box1.padding.y() = 100_px;
        box1.padding.x() = 20_pt;
        box1.updatePaddings();

        TestRenderer renderer;
        layout.draw(renderer, box1.innerBox);

        layout.doLayout({ (int)window->width(), (int)window->height() }, 0.5f);
        layout.debug(layout.screen());
        layout.debug(box1.outerBox);
        layout.debug(box1.innerBox);

        
        /*PAINTSTRUCT paintInfo;
        HDC paint = BeginPaint(impl->hwnd(), &paintInfo);
        WIN_VERIFY(paint != nullptr);

        
        //TextOut(paint, 0, 0, "Hello, Windows!", 15);

        bool ret = EndPaint(impl->hwnd(), &paintInfo);
        WIN_VERIFY(ret);*/
    }

    void draw(plt::Window* window) {
        layout.doDraw();
        auto* impl = (plt::windows::WindowImpl*)window;
        BITMAPINFO bitmapInfo {
            .bmiHeader = {
                .biSize = sizeof(BITMAPINFOHEADER),
                .biWidth = (int)window->width(),
                .biHeight = (int)window->height(),
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
        int linesProcessed = SetDIBitsToDevice(dc, 0, 0, window->width(), window->height(), 0, 0, 0, window->height(), layout.framebuffer.data, &bitmapInfo, DIB_RGB_COLORS);
        WIN_VERIFY(linesProcessed != 0);
    }
};

}

namespace ui {

struct Padding : StyleKey<Padding, Rect> { };
struct TextSize : StyleKey<TextSize, Size> { };
struct BoldText : StyleKey<BoldText, bool> { };
struct Factor : StyleKey<Factor, float> { };
struct Count : StyleKey<Count, int> { };
enum class Mode {
    Forward,
    Backward,
};
struct ReadMode : StyleKey<ReadMode, Mode> { };
struct Foo {
    char buf[3] = { 1, 2, 3 };
};
struct FooKey : StyleKey<FooKey, Foo> { };
using TextBoxStyle = SmallStyle<Padding, TextSize, BoldText>;
struct NormalTextBox : StyleKey<NormalTextBox, TextBoxStyle> { };

void test() {

    auto style = Ref<BigStyle>::allocate();
    Ref<TextBoxStyle>& textBoxStyle = style.get<NormalTextBox>();
    textBoxStyle.get<TextSize>() = ref<TextSize>();
    style.get<TextSize>() = 10_px;
    VDUMP(style.get<TextSize>().value().value());
    style.get<BoldText>() = true;
    VDUMP(style.get<BoldText>().value());
    style.get<Factor>() = 3.0f;
    VDUMP(style.get<Factor>().value());
    style.get<Count>() = 5;
    VDUMP(style.get<Count>().value());
    style.get<ReadMode>() = Mode::Backward;
    VDUMP(style.get<ReadMode>().value() == Mode::Backward);
    style.get<FooKey>() = Foo {};
    // VDUMP(get<TextSize>(resolve<NormalTextBox>(style)).value());

    sparse::LUDecomposition lu;
    lu.u.appendColumn(sparse::StaticVector({ { 1, 1.0f }, { 3, -0.5f } }).copy());
    lu.u.appendColumn(sparse::StaticVector({ { 0, 0.5f }, { 1, 1.0f } }).copy());
    lu.u.appendColumn(sparse::allocateEmpty());
    lu.u.appendColumn(sparse::StaticVector({ { 0, -0.5f }, { 3, 3.0f } }).copy());
    lu.decompose(4);
    lu.l.debug();
    lu.u.debug();
    float result[4] = {};
    lu.inverseTranspose(std::array<float, 4> { 1.0f, 2.0f, 1000.0f, 3.0f }.data(), result);
    log("result = ({}, {}, {}, {})", result[0], result[1], result[2], result[3]);

    std::unique_ptr<plt::Window> window = plt::Window::construct();
    UiTest callback;
    window->setStateCallback(&callback);
    window->setTitle("UiTest");
    //gl::initialize();
    //gl::attachWindow(window.get());
    window->initialize();
    while (!window->destoryed()) {
        plt::processEvents();
        callback.draw(window.get());
    }
}

}