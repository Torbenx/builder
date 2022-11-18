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
    static void render(ui::ImageView image, const TestStyle& style) {
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
        ui::IntVec dim = image.dimensions();
        for (int j = 0; j < dim.y(); j++) {
            for (int i = 0; i < dim.x(); i++) {
                float fracX = i / (float)(dim.x() - 1);
                float fracY = j / (float)(dim.y() - 1);
                uint32_t c1 = fracX * 255;
                uint32_t c2 = 255 - c1;
                uint32_t c3 = fracY * fracY * 255;
                image[{ i, j }] = 0xff'00'00'00 | (c3 << 24) | (c1 << shift1) | (c2 << shift2);
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

struct Clearer {
    struct Job {
        ui::ImageView image;
        sw::Semaphore signal;
        uint32_t color = 0;

        void clear() {
            std::fill_n(image.data(), image.stride() * image.dimensions().y(), color);
            signal.signal();
            delete this;
        }
    };

    uint32_t color = 0;
    gfx::SWImageHandle<ui::IMAGE_FORMAT> framebuffer;

    void clear(sw::Semaphore waitFor, sw::Semaphore signal) {
        Job* job = new Job;
        job->image = framebuffer.get();
        job->color = color;
        job->signal = std::move(signal);
        waitFor.thenAsync({ *job, stx::CallMember<&Job::clear>() });
    }

    void setFramebuffer(gfx::SWImageHandle<ui::IMAGE_FORMAT> framebuffer) {
        this->framebuffer = framebuffer;
    }
    void setColor(uint32_t color) { this->color = color; }
};

struct Presenter {
    struct Job {
        plt::Window window;
        ui::ImageView framebuffer;
        sw::Semaphore signal;

        void present() {
            static std::atomic<int_t> frames = 0;
            static std::atomic<uint64_t> lastFrame = 0;
            uint64_t now = getNanos();
            uint64_t lastFrameCopy = lastFrame.load();
            if (now - lastFrameCopy >= 1'000'000'000 && lastFrame.compare_exchange_strong(lastFrameCopy, now)) {
                int_t framesCopy = frames.exchange(0);
                log("FPS: {}", framesCopy);
            }
            frames.fetch_add(1);

            auto* impl = plt::windows::windowImpl(window);
            BITMAPINFO bitmapInfo {
                .bmiHeader = {
                    .biSize = sizeof(BITMAPINFOHEADER),
                    .biWidth = (int)framebuffer.stride(),
                    .biHeight = framebuffer.dimensions().y(),
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
            ui::IntVec dim = framebuffer.dimensions();
            int linesProcessed = SetDIBitsToDevice(dc, 0, 0, dim.x(), dim.y(), 0, 0, 0, dim.y(), framebuffer.data(), &bitmapInfo, DIB_RGB_COLORS);
            WIN_VERIFY(linesProcessed > 0);
            signal.signal();
            delete this;
        }
    };

    plt::Window window;
    gfx::SWImageHandle<ui::IMAGE_FORMAT> framebuffer;

    Presenter(plt::Window window)
        : window(window) { }

    void present(sw::Semaphore waitFor, sw::Semaphore signal) {
        Job* job = new Job;
        job->window = window;
        job->framebuffer = framebuffer.get();
        job->signal = std::move(signal);
        waitFor.thenAsync({ *job, stx::CallMember<&Job::present>() });
    }

    void setFramebuffer(gfx::SWImageHandle<ui::IMAGE_FORMAT> framebuffer) {
        this->framebuffer = framebuffer;
    }
};

evt::Script uiTestWrapper(plt::Window window) {
    await { evt::Defer() };
    ui::AsyncInstance instance { window.stateUpdate(), window.resizeEvent() };
    evt::Script script = evt::asyncScript(instance, uiTestInit);
    window.initialize();

    stx::Owned<Clearer*> clearer = new Clearer;
    clearer->setColor(0xff'ff'ff'ff);
    stx::Owned<ui::Instance::SWPainter*> uiPainter = instance.createSWPainter();
    stx::Owned<Presenter*> presenter = new Presenter(window);

    auto graphicsInst = std::make_unique<gfx::Instance>();
    gfx::SWImageHandle<ui::IMAGE_FORMAT> framebuffer;
    sw::iVec2 framebufferDimensions = {};

    auto paint = [&]() {
        if (window.dimensions().x() == 0 || window.dimensions().y() == 0)
            return;
        if (!graphicsInst->nextFrameReady()) {
            // log("next frame was not ready");
            // VDUMP(graphicsInst->allocationOngoing.load());
            // VDUMP(graphicsInst->framesReadyMask.load(), ":#b");
            return;
        }

        sw::Semaphore allocationComplete = sw::Semaphore::create();
        sw::Semaphore paintUi = sw::Semaphore::create();
        sw::Semaphore present = sw::Semaphore::create();
        sw::Semaphore frameComplete = sw::Semaphore::create();

        graphicsInst->beginFrame(allocationComplete.fork(), frameComplete.fork());

        if (framebufferDimensions != window.dimensions()) {
            framebufferDimensions = window.dimensions();
            log("allocating new framebuffer");
            framebuffer = gfx::SWImageHandle<ui::IMAGE_FORMAT>::createDynamic(framebufferDimensions);
            clearer->setFramebuffer(framebuffer);
            uiPainter->setFramebuffer(framebuffer);
            presenter->setFramebuffer(framebuffer);
        }

        clearer->clear(sw::Semaphore::create(), paintUi.fork());
        uiPainter->paint(std::move(paintUi), present.fork());
        presenter->present(std::move(present), std::move(frameComplete));
        graphicsInst->clearFrame();
        allocationComplete.signal();
    };

    evt::Receiver<evt::ScriptCompletionEvent> completionEv;
    while (!completionEv.hasEvent()) {
        instance.asyncTick();
        paint();
        await { (evt::NextTick(), script->completion > completionEv) };
    };
    instance.asyncTick();
    await { evt::Ping(instance) };
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