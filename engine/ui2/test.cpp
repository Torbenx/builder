#include "Instance.hpp"
#include "box.hpp"
#include "equation_code.hpp"
#include "event/Awaitable.hpp"
#include "event/Event.hpp"
#include "event/conditions.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "opengl/core.hpp"
#include "platform/Window.hpp"
#include "platform/windows/WindowImpl.hpp"
#include "platform/windows/core.hpp"
#include "platform/windows/glcore.hpp"
#include "util/LogTimer.hpp"
#include <memory>
#include <sstream>
#include "graphics/wip.hpp"

namespace {

struct BasicAtlasAllocator : ui::AtlasAllocator {
    ui::IntVec nextAtlasSize(ui::IntVec size) override {
        if (size.x() == 0)
            size.x() = 4096;
        if (size.y() == 0)
            size.y() = 2048;
        else
            size.y() = size.y() * 2;
        return size;
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

struct TestRenderer : ui::Renderer<TestRenderer, TestStyle> {
    static void render(const ui::TextureState& texture, const TestStyle& style) {
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
        ui::TextureView view = texture.texture;
        ui::IntVec dim = texture.dimensions;
        for (int_t j = 0; j < dim.y(); j++) {
            for (int_t i = 0; i < dim.x(); i++) {
                float fracX = i / (float)(dim.x() - 1);
                float fracY = j / (float)(dim.y() - 1);
                uint32_t c1 = fracX * 255;
                uint32_t c2 = 255 - c1;
                uint32_t c3 = fracY * fracY * 255;
                view[j][i] = 0xff'00'00'00 | (c3 << 24) | (c1 << shift1) | (c2 << shift2);
            }
        }
    }
};
evt::Script uiTestInit() {
    await { evt::OnExecutor(ui::coreExecutor()) };

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

struct PerformDrawTask : dep::Task {
    ui::Instance& instance;
    plt::Window nativeWindow;
    ui::TextureHandle atlas;
    std::unique_ptr<uint32_t[]> framebuffer;
    ui::IntVec frameSize;
    evt::Relay<evt::ResizeEvent> resizeRelay;

    PerformDrawTask(ui::Instance& instance, evt::Source<evt::ResizeEvent>& resizeEvent, plt::Window nativeWindow)
        : dep::Task { stx::CallDerivedMember<&PerformDrawTask::draw>() }
        , instance(instance)
        , nativeWindow(nativeWindow)
        , resizeRelay(evt::relayEvents(resizeEvent)) {
    }

    uint64_t lastFrame = 0;
    int_t frames = 0;
    uint64_t totalDrawTime = 0;
    void draw(dep::Scheduler) {
        uint64_t now = getNanos();
        if (now - lastFrame >= 1'000'000'000) {
            log("FPS: {}, draw time: {}%", frames, totalDrawTime * 100.0f / (now - lastFrame));
            lastFrame = now;
            totalDrawTime = 0;
            frames = 0;
        }
        frames += 1;

        auto resizeEvents = resizeRelay.output->takeEvents<evt::ResizeEvent>();
        if (resizeEvents.hasEvent()) {
            ui::IntVec windowBlockSize = (resizeEvents.event.value().newDimensions + 15) / 16;
            frameSize = windowBlockSize * 16;
            framebuffer = std::make_unique<uint32_t[]>(frameSize.x() * frameSize.y());
        }

        if (atlas.dimensions != instance.atlasDimensions) {
            std::allocator<uint32_t> alloc;
            alloc.deallocate(atlas.data, atlas.dimensions.x() * atlas.dimensions.y());
            atlas.dimensions = instance.atlasDimensions;
            atlas.data = alloc.allocate(atlas.dimensions.x() * atlas.dimensions.y());
        }

        for (int_t i = 0; i < instance.incrementalRenderOutput.size(); i++) {
            auto& outputs = instance.incrementalRenderOutput[i];
            const ui::RendererRecord& record = ui::RendererBase::record(i);
            int_t stride = ui::RenderOutputUtil::outputSize(i);
            for (auto it = outputs.begin(); it != outputs.end(); it += stride) {
                ui::RenderOutputUtil output(&*it);
                ui::TextureState texture { atlas.view() + output.atlasRect().bottomLeft(), output.atlasRect().dimensions() };
                ui::TextureState paddedTexture { texture.texture - 16, texture.dimensions + 32 };
                for (int_t j = 0; j < paddedTexture.dimensions.y(); j++) {
                    for (int_t i = 0; i < paddedTexture.dimensions.x(); i++) {
                        paddedTexture.texture[j][i] = 0;
                    }
                }
                record.render(texture, output.renderDataPtr());
            }
            outputs.clear();
        }
        instance.outputGeneration += 1;

        drawToFramebuffer();
        totalDrawTime += getNanos() - now;
    };

    void drawToFramebuffer() {
        if (!framebuffer || nativeWindow.destroyed())
            return;
        ui::TextureView frameView { framebuffer.get(), frameSize.x() };
        uint32_t background_color = 0xff'ff'ff'ff;
        ui::IntVec windowSize = instance.windowDimensions;
        for (int_t j = 0; j < windowSize.y(); j++) {
            for (int_t i = 0; i < windowSize.x(); i++) {
                frameView[j][i] = background_color >> 8;
            }
        }
        auto multiply = [](uint32_t in, uint32_t alpha) {
            return (((in * alpha / 255) & 0xff0000) | (((in & 0xff00) * alpha / 255) & 0xff00) | ((in & 0xff) * alpha / 255));
        };
        int_t blockCount = instance.outputOffsets.size() / (instance.outputLayers + 1);
        for (int_t k = 0; k < blockCount; k++) {
            ui::IntVec* block = instance.outputOffsets.data() + k * (instance.outputLayers + 1);
            ui::IntVec screenPos = block[0];
            for (int_t j = 0; j < 16; j++) {
                for (int_t i = 0; i < 16; i++) {
                    uint32_t color = (frameView + screenPos)[j][i];
                    for (int_t layer = 0; layer < instance.outputLayers; layer++) {
                        ui::IntVec layerPos = block[layer + 1];
                        if (layerPos == ui::IntVec {})
                            break;
                        uint32_t in = (atlas.view() + layerPos)[j][i];
                        uint32_t alpha = in >> 24;
                        color = multiply(in & 0xff'ff'ff, alpha) + multiply(color, 255 - alpha);
                        // color = in & 0xff'ff'ff;
                    }
                    (frameView + screenPos)[j][i] = color;
                }
            }
        }

        auto* impl = plt::windows::windowImpl(nativeWindow);
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
        ui::IntVec dim = instance.windowDimensions;
        int linesProcessed = SetDIBitsToDevice(dc, 0, 0, dim.x(), dim.y(), 0, 0, 0, dim.y(), framebuffer.get(), &bitmapInfo, DIB_RGB_COLORS);
        WIN_VERIFY(linesProcessed != 0);
    }
};

struct UpdateUiTask : dep::Task {
    ui::Instance& instance;
    evt::ExecutorImpl* executor = nullptr;

    UpdateUiTask(ui::Instance& instance, evt::ExecutorImpl* exe)
        : dep::Task { stx::CallDerivedMember<&UpdateUiTask::update>() }
        , instance(instance)
        , executor(exe) { }

    void update(dep::Scheduler scheduler) {
        ui::setInstance(&instance);
        executor->execute();
        instance.doTick(scheduler);
    }
};

struct ShutdownEvent { };
evt::Script uiTestWrapper2(plt::Window window, stx::Owned<evt::RelayOutputPromise<evt::WindowStateUpdate, evt::ResizeEvent>*> relayedBackend, ui::Instance& instance, UpdateUiTask& updateUi, evt::Source<ShutdownEvent>& shutdownSource, dep::Scheduler scheduler) {
    relayedBackend->startRelay(instance.coreExe);

    PerformDrawTask performDraw { instance, relayedBackend->output<evt::ResizeEvent>(), window };
    stx::Owned<dep::Dependency> dependencies[] = {
        instance.outputComplete.createDependency(performDraw),
        performDraw.createDependency(plt::tickComplete()),
        performDraw.createNextTickDependency(updateUi),
    };
    performDraw.unlock(scheduler);

    ui::setInstance(&instance);
    auto testScript = uiTestInit();
    await { evt::OnExecutor(ui::coreExecutor()) && evt::NextTick() && *testScript->completion };

    plt::queueCallback({
        &shutdownSource,
        [](evt::Source<ShutdownEvent>* shutdownSource, dep::Scheduler) {
            shutdownSource->sendEvent({});
        },
    });
    updateUi.dependencyCount += dep::INVALID_SIGNAL_INCREMENT;
}

evt::Script uiTestWrapper1(plt::Window window, dep::Scheduler scheduler) {
    evt::Source<ShutdownEvent> shutdownSource;

    evt::ExecutorImpl uiExecutor;
    evt::Relay windowRelay = evt::relayEvents(window.stateUpdate(), window.resizeEvent());
    BasicAtlasAllocator atlasAlloc;
    ui::Instance instance { windowRelay.output->output<evt::WindowStateUpdate>(), windowRelay.output->output<evt::ResizeEvent>(), &uiExecutor, &atlasAlloc };

    UpdateUiTask updateUi { instance, &uiExecutor };
    auto inputDep = plt::processedEvents().createDependency(updateUi);
    auto outputDep = updateUi.createDependency(plt::tickComplete());
    updateUi.unlock(scheduler);

    uiTestWrapper2(window, std::move(windowRelay.output), instance, updateUi, shutdownSource, scheduler);

    await { shutdownSource };
}

}

namespace ui {

void test(dep::Scheduler scheduler) {
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
    uiTestWrapper1(window, scheduler);
    window.setDimensions({ 700, 700 });
    window.setTitle("UiTest");
    window.initialize();
}

}