#include "Instance.hpp"
#include "box.hpp"
#include "containers/AtomicQueue.hpp"
#include "equation_code.hpp"
#include "event/Awaitable.hpp"
#include "event/Timeline.hpp"
#include "event/conditions.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "opengl/core.hpp"
#include "platform/Window.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "platform/windows/core.hpp"
#include "platform/windows/glcore.hpp"
#include "ui2/AsyncInstance.hpp"
#include "util/LogTimer.hpp"
#include <memory>
#include <sstream>

namespace {

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

struct TestRenderer : ui::Renderer<TestRenderer, TestStyle> {
    static void render(sw::TextureView texture, const TestStyle& style) {
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
                return 0;
            }
        };
        int shift1 = channelToShift(style.get<Channel1>());
        int shift2 = channelToShift(style.get<Channel2>());
        ui::IntVec dim = texture.dimensions;
        for (int_t j = 0; j < dim.y(); j++) {
            for (int_t i = 0; i < dim.x(); i++) {
                float fracX = i / (float)(dim.x() - 1);
                float fracY = j / (float)(dim.y() - 1);
                uint32_t c1 = fracX * 255;
                uint32_t c2 = 255 - c1;
                uint32_t c3 = fracY * fracY * 255;
                texture.pos()[j][i] = 0xff'00'00'00 | (c3 << 24) | (c1 << shift1) | (c2 << shift2);
            }
        }
    }
};
evt::Script uiTestInit() {
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
    ui::RectVarHandle windowRect = ui::windowRect();
    box1Eqn = box1.box() == windowRect;
    /*box1Eqn.left() = box1.outerBox.left() + box1.outerBox.right() == windowRect.left() + windowRect.right();
    box1Eqn.bottom() = box1.outerBox.bottom() + box1.outerBox.top() == windowRect.bottom() + windowRect.top();
    box1Eqn.right() = box1.innerBox.width() / 16.0f == box1.innerBox.height() / 9.0f;
    box1Eqn.top() = box1.innerBox.width() == 0.5f * windowRect.width();*/

    box1.setTable(bigStyle);
    box1.padded().setStyle(ui::key<DefaultBoxStyle>());
    box1.renderer().setStyle(ui::key<DefaultTestStyle>());

    {
        auto& boxStyle = box1.padded().mutateLocal();
        boxStyle.get<ui::Padding>().x() = 0_px;
        boxStyle.get<ui::Padding>().y() = 0_px;
        auto& style = box1.renderer().mutateLocal();
        style.get<Channel1>() = ColorChannel::Red;
    }

    while (!ui::destroyed()) {
        evt::Receiver<evt::WindowStateUpdate> events;
        await { ui::stateUpdate() > [](evt::WindowStateUpdate update) {
            log("state change: {} -> {}", evt::WindowStateUtil(update.from).toString(), evt::WindowStateUtil(update.to).toString());
        } };
    }
}

struct ClearJob {
    sw::TextureView texture;
    sw::Semaphore signal;
    uint32_t color = 0;

    void clear() {
        std::fill_n(texture.pos().data, texture.pos().stride * texture.dimensions.y(), color);
        signal.signal();
        delete this;
    }
};

struct PresentJob {
    plt::Window window;
    sw::TextureView framebuffer;
    AtomicQueue<sw::TextureView>* queue = nullptr;
    static inline int_t frames = 0;
    static inline uint64_t lastFrame = 0;
    void present() {
        uint64_t now = getNanos();
        if (now - lastFrame >= 1'000'000'000) {
            lastFrame = now;
            log("FPS: {}", frames);
            frames = 0;
        }
        frames += 1;
        auto* impl = plt::windows::windowImpl(window);
        BITMAPINFO bitmapInfo {
            .bmiHeader = {
                .biSize = sizeof(BITMAPINFOHEADER),
                .biWidth = (int)framebuffer.pos().stride,
                .biHeight = framebuffer.dimensions.y(),
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
        sw::IntVec dim = framebuffer.dimensions;
        int linesProcessed = SetDIBitsToDevice(dc, 0, 0, dim.x(), dim.y(), 0, 0, 0, dim.y(), framebuffer.pos().data, &bitmapInfo, DIB_RGB_COLORS);
        WIN_VERIFY(linesProcessed > 0);
        queue->pushBack(framebuffer);
        delete this;
    }
};

evt::Script uiTestWrapper(plt::Window window) {
    await { evt::Defer() };
    ui::AsyncInstance instance { window.stateUpdate(), window.resizeEvent() };
    evt::Script script = evt::asyncScript(instance, uiTestInit);
    window.initialize();

    int_t framebuffersAlive = 0;
    AtomicQueue<sw::TextureView> framebufferQueue;

    auto paint = [&]() {
        if (window.dimensions().x() == 0 || window.dimensions().y() == 0)
            return;

        std::optional<sw::TextureView> opt;
        while (true) {
            opt = framebufferQueue.tryPopFront();
            if (!opt.has_value()) {
                framebuffersAlive += 1;
                log("made a framebuffer");
                opt = sw::allocateTexture(window.dimensions());
                break;
            } else if (opt->dimensions == window.dimensions()) {
                break;
            }
            framebuffersAlive -= 1;
            opt->deallocate();
        }
        sw::TextureView framebuffer = opt.value();

        sw::Semaphore paintUi = sw::Semaphore::create();
        {
            ClearJob* job = new ClearJob;
            job->texture = framebuffer;
            job->signal = paintUi.fork();
            job->color = 0xff'ff'ff'ff;
            evt::async({ *job, stx::CallMember<&ClearJob::clear>() });
        }

        sw::Semaphore present = sw::Semaphore::create();
        instance.asyncPaint(framebuffer, std::move(paintUi), present.fork());
        {
            PresentJob* job = new PresentJob;
            job->framebuffer = framebuffer;
            job->window = window;
            job->queue = &framebufferQueue;
            present.thenAsync({ *job, stx::CallMember<&PresentJob::present>() });
        }
    };

    evt::Receiver<evt::ScriptCompletionEvent> completionEv;
    while (!completionEv.hasEvent()) {
        instance.asyncTick();
        paint();
        await { (evt::NextTick(), script->completion > completionEv) };
    };
    instance.asyncTick();
    await { evt::Ping(instance) };

    while (true) {
        while (true) {
            std::optional<sw::TextureView> opt = framebufferQueue.tryPopFront();
            if (!opt.has_value())
                break;
            framebuffersAlive -= 1;
            opt->deallocate();
        }
        if (framebuffersAlive == 0)
            break;
        await { evt::NextTick() };
    }
}

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

    plt::Window window = plt::Window::construct();
    uiTestWrapper(window);
    window.setDimensions({ 700, 700 });
    window.setTitle("UiTest");
}

}