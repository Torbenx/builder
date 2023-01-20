#include "graphics/shadergen/compute.hpp"

#include "AsyncInstance.hpp"
#include "FixedPoint.hpp"
#include "event/Awaitable.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "util/log.hpp"

namespace ui {

Instance::Instance(const evt::Source<evt::WindowStateUpdate>& stateUpdate, const evt::Source<evt::ResizeEvent>& resizeEvent, AtlasAllocator* atlasAlloc)
    : windowState(stateUpdate)
    , atlasAlloc(atlasAlloc)
    , rendererUpdates(RendererBase::recordCount())
    , incrementalRenderOutput(RendererBase::recordCount())
    , styleConsumerUpdates(StyleConsumerBase::recordCount()) {

    windowRect = newRect();

    resizeEventScript = [](Instance* instance, const evt::Source<evt::ResizeEvent>& resizeEvent) -> evt::Script {
        co_await evt::onTimeline(instance);
        IntVec initialDim = resizeEvent.currentDimensions;
        log("initialDim: {} x {}", initialDim.x(), initialDim.y());
        if (initialDim.x() > 0 && initialDim.y() > 0)
            instance->resize();
        while (true) {
            await { resizeEvent };
            log("resize!");
            instance->windowDimensions = resizeEvent.currentDimensions;
            instance->resize();
        }
    }(this, resizeEvent);

    auto fix = [&](VarHandle var) -> EquationHandle {
        EquationHandle eq = newEquation();
        setEquation(eq, { { var.id, 1.0f } });
        return eq;
    };
    windowEquations[0] = fix(windowRect.left());
    windowEquations[1] = fix(windowRect.right());
    windowEquations[2] = fix(windowRect.bottom());
    windowEquations[3] = fix(windowRect.top());
}

Instance::~Instance() {
    for (int_t i = 0; i < 4; i++)
        removeEqn(windowEquations[i]);
    deleteVariable(windowRect);
    atlasInUse.destroy();
    afterAtlasUpdateChain.destroy();
}

uint32_t Instance::allocateVariableIds(int_t count) {
    uint32_t id = nextVarId;
    nextVarId += count;
    varValues.resize(nextVarId);
    return id;
}

VarHandle Instance::newVar() { return { allocateVariableIds(1) }; }
RangeVarHandle Instance::newRange() { return { allocateVariableIds(2) }; }
RectVarHandle Instance::newRect() { return { allocateVariableIds(4) }; }

EquationHandle Instance::newEquation() {
    EquationHandle eq = { eqnIdAlloc.allocateId() };
    if (inhomTyped.size() <= eq.id)
        inhomTyped.resize(eq.id + 1);
    if (inhomValues.size() <= eq.id)
        inhomValues.resize(eq.id + 1);
    if (eq.id == matrix.size())
        matrix.appendColumn({});
    return eq;
}

void Instance::setEquation(EquationHandle eq, ArrayView<const sparse::Entry> terms) {
    sparse::Vector vec = sparse::allocate(terms.size());
    std::copy(terms.begin(), terms.end(), vec.begin());
    vec.setSize(terms.size());
    vec.sortAndDedup();
    matrix[eq.id] = std::move(vec);
}

void Instance::updateInhom(EquationHandle eq, Size inhom) {
    inhomTyped[eq.id] = inhom;
}

void Instance::removeEqn(EquationHandle eqn) {
    matrix[eqn.id].resizeTo(0);
    eqnIdAlloc.freeId(eqn.id);
}
void Instance::deleteVariable(VarHandle) {
    VERIFY_NOT_REACHED(Ui);
}
void Instance::deleteVariable(RangeVarHandle) {
    VERIFY_NOT_REACHED(Ui);
}
void Instance::deleteVariable(RectVarHandle) {
    VERIFY_NOT_REACHED(Ui);
}

void Instance::dumpVarValues() {
    for (int_t i = 0; i < (int_t)varValues.size(); i++) {
        log<Tag::Ui>("{} -> {}", i, varValues[i]);
    }
}
void Instance::dumpShiftValues() {
    for (int_t i = 0; i < (int_t)inhomValues.size(); i++) {
        log<Tag::Ui>("{} -> {}", i, inhomValues[i]);
    }
}
void Instance::dumpShiftSizes() {
    for (int_t i = 0; i < (int_t)inhomTyped.size(); i++) {
        log<Tag::Ui>("{} -> {}{}{}", i, inhomTyped[i].value(), inhomTyped[i].unitBit() ? "pt" : "px", inhomTyped[i].payloadBit() ? " +1px" : "");
    }
}

struct Instance::AtlasJob {
    Instance* instance = nullptr;
    IntVec atlasDimensions;
    stx::DynArray<std::vector<std::byte>> atlasUpdates;
    sw::Semaphore signal;

    void updateAtlas() {
        if (atlasDimensions != instance->atlas.dimensions())
            instance->atlas = ui::ImageView::allocate(atlasDimensions);
        ImageView atlas = instance->atlas;

        // FIXME: should not read from instance here
        LayoutMetadata meta { instance->ptScale };

        for (int_t i = 0; i < atlasUpdates.size(); i++) {
            auto& updates = atlasUpdates[i];
            const ui::RendererRecord& record = ui::RendererBase::record(i);
            int_t stride = ui::RenderOutputUtil::outputSize(i);
            for (auto it = updates.begin(); it != updates.end(); it += stride) {
                ui::RenderOutputUtil output(&*it);
                ui::IntRect paddedRect {
                    ui::BottomLeft(output.atlasRect().bottomLeft() - 16),
                    ui::TopRight(output.atlasRect().topRight() + 16),
                };
                ImageView texture { atlas + output.atlasRect().bottomLeft(), output.atlasRect().dimensions() };
                ImageView paddedTexture { atlas + paddedRect.bottomLeft(), paddedRect.dimensions() };
                for (int j = 0; j < paddedTexture.dimensions().y(); j++) {
                    for (int i = 0; i < paddedTexture.dimensions().x(); i++) {
                        paddedTexture[{ i, j }] = 0;
                    }
                }
                record.render(texture, output.renderDataPtr(), meta);
            }
        }

        signal.signal();
        delete this;
    }
};

struct Instance::SWPainter::Job : Instance::PainterJob {
    ImageView framebuffer;
    sw::Semaphore signal;

    void paint() {
        int_t blockCount = blocks.blockOffsets.size() / (blocks.blockLayers + 1);
        for (int_t k = 0; k < blockCount; k++) {
            const IntVec* block = blocks.blockOffsets.data() + k * (blocks.blockLayers + 1);
            IntVec screenPos = block[0];
            int_t h = std::min(16, framebuffer.dimensions().y() - screenPos.y());
            for (int_t j = 0; j < h; j++) {
                uint32_t* buffer = (framebuffer + screenPos).row(j);
                Register<Color> dst = Register<Color>::load(buffer);
                for (int_t layer = 0; layer < blocks.blockLayers; layer++) {
                    IntVec layerPos = block[layer + 1];
                    if (layerPos == IntVec {})
                        break;
                    Register<Color> src = Register<Color>::load((atlas + layerPos).row(j));
                    dst = blendPremultiplied(src, dst);
                }
                dst.store(buffer);
            }
        }

        doneUsingAtlas.signal();
        signal.signal();
        delete this;
    }
};

void Instance::SWPainter::paint(sw::Semaphore waitFor, sw::Semaphore signal) {
    Job* job = new Job;
    job->framebuffer = framebuffer.get();
    job->signal = std::move(signal);
    waitFor.setAsyncCallback({ *job, stx::CallMember<&Job::paint>() });
    instance->paintEvents().push_back({
        .atlasReadyForUse = std::move(waitFor),
        .painter = job,
    });
}

void Instance::SWPainter::setFramebuffer(gfx::SWImageHandle<IMAGE_FORMAT> framebuffer) {
    this->framebuffer = framebuffer;
}

struct Instance::HWPainter::Job : Instance::PainterJob {
    sw::Semaphore signal;
    sw::Semaphore uploadLock;
    IntVec* positionsBuffer = nullptr;
    gfx::Shader<Shader> shader;

    void paint() {
        std::copy_n(blocks.blockOffsets.data(), blocks.blockOffsets.size(), positionsBuffer);
        uploadLock.signal();

        // painter->shader;

        signal.signal();
        doneUsingAtlas.signal();
        delete this;
    }
};

constexpr void Instance::HWPainter::Shader::defineShader(shadergen::ComputeDriver& prog) {
    using namespace shadergen::code;
    auto data = prog.uniformBuffer<&Shader::data>("data");
    auto positions = prog.storageBuffer<&Shader::positions>("positions", MemQual::NoWrite);
    auto screen = prog.storageImage<&Shader::screen>("screen", (MemQual)0);
    auto atlas = prog.storageImage<&Shader::atlas>("atlas", MemQual::NoWrite);

    AUTO_VARIABLE(prog, offset) = prog.globalIndex.x * (data.maxLayers + constant(1));
    AUTO_VARIABLE(prog, screenPos) = positions[offset] + prog.localIndex.xy();
    AUTO_VARIABLE(prog, color) = imageLoad(screen, screenPos).xyz();
    prog.range(constant(0), data.maxLayers, [&](auto& i) {
        AUTO_VARIABLE(prog, atlasPos) = positions[i + constant(1)] + prog.localIndex.xy();
        AUTO_VARIABLE(prog, inColor) = imageLoad(atlas, atlasPos);
        prog, color = inColor.xyz() * inColor.w + color * (constant(1.0f) - inColor.w);
    });
    prog, imageStore(screen, screenPos, compose(color, constant(1.0f)));
}

}
DEFINE_SHADER(ui::Instance::HWPainter::Shader);
namespace ui {

Instance::HWPainter::HWPainter(AsyncInstance* instance)
    : instance(instance) {
    metadataBuffer = decltype(metadataBuffer)::create();
    metadataBuffer.staticUpdate();
    *metadataBuffer.mapping() = { .maxLayers = 1 };

    positionsBuffer = decltype(positionsBuffer)::create();
    positionsBuffer.beginDynamicUpdates(10000);

    shader.mutate() = Shader {
        .data = metadataBuffer,
        .positions = positionsBuffer,
        .atlas = atlasTexture.image(),
    };
}

void Instance::HWPainter::paint(sw::Semaphore signal) {
    Job* job = new Job;
    job->signal = std::move(signal);
    job->positionsBuffer = positionsBuffer.mapping();
    job->uploadLock = gfx::allocator()->uploadLock();
    sw::Semaphore beginPaint = sw::Semaphore::create();
    beginPaint.setAsyncCallback({ *job, stx::CallMember<&Job::paint>() });
    instance->paintEvents().push_back({
        .atlasReadyForUse = std::move(beginPaint),
        .painter = job,
    });
}

void Instance::HWPainter::setFramebuffer(gfx::Texture<IMAGE_FORMAT> framebuffer) {
    shader.mutate().screen = framebuffer.image();
}

void Instance::doTick() {
    beginTick();
    executeScripts();

    LayoutMetadata meta { ptScale };

    for (int_t i = 0; i < styleConsumerUpdates.size(); i++) {
        auto& updates = styleConsumerUpdates[i];
        const auto& record = StyleConsumerBase::record(i);
        for (StyleConsumerBase* toUpdate : updates)
            record.updateStyle(toUpdate, meta);
        updates.clear();
    }
    for (int_t i = 0; i < rendererUpdates.size(); i++) {
        auto& updates = rendererUpdates[i];
        const auto& record = RendererBase::record(i);
        for (uintptr_t& toUpdateUint : updates) {
            RendererBase* toUpdate = (RendererBase*)toUpdateUint;
            RenderOutputUtil output = allocateRenderOutput(renders[toUpdate->id]);
            record.updateStyleAndPrepareRender(toUpdate, output.renderDataPtr(), meta);
            toUpdateUint = (uintptr_t)toUpdate->id;
        }
    }

    updateLayout(false);

    for (int_t i = 0; i < rendererUpdates.size(); i++) {
        auto& updates = rendererUpdates[i];
        for (uintptr_t toUpdate : updates) {
            auto& render = renders[toUpdate];
            RenderOutputUtil output = findRenderOutput(render);
            IntVec dim = intRect(render.rect).dimensions();
            IntRect atlasRect { BottomLeft(render.allocPosition), Dimensions(dim) };
            output.atlasRect() = atlasRect;
        }
        updates.clear();
    }

    int_t totalUpdates = 0;
    for (auto& updates : incrementalRenderOutput)
        totalUpdates += updates.size();
    bool atlasDirty = totalUpdates > 0;
    if (paintEvents().size() > 0) {
        if (atlasDirty) {
            AtlasJob* job = new AtlasJob;
            job->atlasUpdates = std::move(incrementalRenderOutput);
            incrementalRenderOutput = stx::DynArray<std::vector<std::byte>> { RendererBase::recordCount() };
            outputGeneration += 1;
            job->instance = this;
            job->atlasDimensions = atlasDimensions;

            // run job after prev user is done using the atlas
            sw::Semaphore param[] = { std::move(atlasInUse), std::move(afterAtlasUpdateChain) };
            sw::Semaphore::merge(param).thenAsync({ *job, stx::CallMember<&AtlasJob::updateAtlas>() });
            atlasInUse = sw::Semaphore::create();
            afterAtlasUpdateChain = sw::Semaphore::create();
            job->signal = afterAtlasUpdateChain.fork();
        }
        for (auto& event : paintEvents()) {
            event.painter->doneUsingAtlas = atlasInUse.fork();
            event.painter->blocks = buildBlocks();
        }
        struct DistJob {
            std::vector<PaintEvent> events;
            Instance* instance;
            sw::Semaphore signal;
            void run() {
                for (auto& event : events) {
                    event.painter->atlas = instance->atlas;
                    event.atlasReadyForUse.signal();
                }
                signal.signal();
                delete this;
            }
        };
        sw::Semaphore nextChain = sw::Semaphore::create();
        DistJob* distJob = new DistJob { std::move(paintEvents()), this, nextChain.fork() };
        afterAtlasUpdateChain.thenSync({ *distJob, stx::CallMember<&DistJob::run>() });
        afterAtlasUpdateChain = std::move(nextChain);
    }

    endTick();
}

void Instance::resize() {
    updateInhom(windowEquations[1], (windowDimensions.x() - 1) * 1_px);
    updateInhom(windowEquations[3], (windowDimensions.y() - 1) * 1_px);

    IntVec windowBlocks = (windowDimensions + 15) / 16;
    blockMap.clear();
    blockMap.resize(windowBlocks.x() * windowBlocks.y());
}

void Instance::updateLayout(bool clearTextures) {
    sparse::LUDecomposition lu;
    lu.u = matrix.copy();
    int_t varCount = nextVarId;
    lu.decompose(varCount);

    Size::asPixels(inhomTyped.data(), inhomValues.data(), inhomTyped.size(), ptScale, 1.0f);

    std::vector<float> newVarValues(varCount);
    lu.inverseTranspose(inhomValues.data(), newVarValues.data());

    auto updateRenderTexture = [this](Render& render, IntRect screenRect) {
        RenderOutputUtil output;
        if (render.lastOutputGen == outputGeneration) {
            output = findRenderOutput(render);
        } else {
            output = allocateRenderOutput(render);
            render.handle.prepareRender(output.renderDataPtr(), LayoutMetadata { ptScale });
        }
        IntRect atlasRect { BottomLeft(render.allocPosition), Dimensions(screenRect.dimensions()) };
        output.atlasRect() = atlasRect;
    };

    if (clearTextures) {
        deallocateAllTextures();
        for (int_t i = 1; i < (int_t)renders.size(); i++) {
            auto& render = renders[i];
            IntRect rect = intRect(render.rect, newVarValues.data());
            render.allocPosition = allocateTexture(rect.dimensions());
            updateRenderTexture(render, rect);
        }
    } else {
        for (int_t i = 1; i < (int_t)renders.size(); i++) {
            auto& render = renders[i];
            IntRect oldRect = intRect(render.rect, varValues.data());
            IntRect newRect = intRect(render.rect, newVarValues.data());
            if (newRect.dimensions() == oldRect.dimensions())
                continue;
            if (render.allocPosition != IntVec {}) {
                deallocateTexture({ BottomLeft(render.allocPosition), Dimensions(oldRect.dimensions()) });
                render.allocPosition = {};
            }
            render.allocPosition = allocateTexture(newRect.dimensions());
            updateRenderTexture(render, newRect);
        }
    }

    varValues = std::move(newVarValues);
}

Instance::Blocks Instance::buildBlocks() {
    std::vector<IntVec> blockOffsets;
    int_t blockLayers = 3;
    static constexpr uint32_t INVALID_BLOCK_ID = -1;
    for (uint32_t& u : blockMap)
        u = INVALID_BLOCK_ID;
    IntVec blockDimensions = (windowDimensions + 15) / 16;
    for (int_t k = 1; k < (int_t)renders.size(); k++) {
        auto& render = renders[k];
        auto rect = intRect(render.rect);
        IntRect blockRect {
            BottomLeft(rect.bottomLeft() / 16),
            TopRight(rect.topRight() / 16)
        };
        for (int j = 0; j < blockRect.height(); j++) {
            for (int i = 0; i < blockRect.width(); i++) {
                IntVec block = blockRect.bottomLeft() + IntVec { i, j };
                if (!IntRect(blockDimensions).contains(block))
                    continue;
                uint32_t& blockId = blockMap[block.y() * blockDimensions.x() + block.x()];
                // log("block: ({}, {}), id = {:#x}", block.x(), block.y(), blockId);
                if (blockId == INVALID_BLOCK_ID) {
                    blockId = blockOffsets.size() / (blockLayers + 1);
                    VERIFY(Ui, blockOffsets.size() % (blockLayers + 1) == 0);
                    blockOffsets.push_back(block * 16);
                    blockOffsets.resize(blockOffsets.size() + blockLayers);
                }
                uint32_t blockIndex = blockId * (blockLayers + 1);
                int_t layer = 0;
                for (; layer < blockLayers; layer++) {
                    if (blockOffsets[blockIndex + 1 + layer] == IntVec {})
                        break;
                    //log("layer {} used: ({}, {})", layer, blockOffsets[blockIndex + 1 + layer].x(), blockOffsets[blockIndex + 1 + layer].y());
                }
                if (layer == blockLayers) {
                    VERIFY_NOT_REACHED(Ui);
                    continue;
                }
                blockOffsets[blockIndex + 1 + layer] = render.allocPosition - rect.bottomLeft() + block * 16;
            }
        }
    }
    return { blockOffsets, blockLayers };
}

std::byte* Instance::findRenderOutput(const Render& render) {
    VERIFY(Ui, render.lastOutputGen == outputGeneration);
    return incrementalRenderOutput[render.handle.classIndex].data() + render.outputOffsetBytes;
}
std::byte* Instance::allocateRenderOutput(Render& render) {
    int_t stride = RenderOutputUtil::outputSize(render.handle.classIndex);

    std::vector<std::byte>& vec = incrementalRenderOutput[render.handle.classIndex];
    if (render.lastOutputGen != outputGeneration) {
        render.lastOutputGen = outputGeneration;
        render.outputOffsetBytes = vec.size();
        vec.resize(vec.size() + stride);
    }
    return vec.data() + render.outputOffsetBytes;
}
void Instance::draw(RendererHandle handle, RectVarHandle rect) {
    if (handle->id == 0) {
        handle->id = renders.size();
        renders.push_back({ handle, rect });
    } else {
        renders[handle->id].rect = rect;
    }
}
void Instance::cancelDraw(RendererHandle handle) {
    VERIFY(Ui, handle->id != 0);

    Render back = renders.back();
    renders.pop_back();
    back.handle->id = handle->id;
    handle->id = 0;

    renders[back.handle->id] = back;
}

IntVec Instance::allocateTexture(IntVec dimensions) {
    VERIFY(Ui, dimensions.x() <= atlasDimensions.x() || atlasDimensions.x() == 0);
    IntVec allocPos { atlasPadding, nextAtlasAllocationHeight + atlasPadding };
    nextAtlasAllocationHeight += atlasPadding + dimensions.y();
    int_t neededHeight = nextAtlasAllocationHeight + atlasPadding;
    while (neededHeight >= atlasDimensions.y()) {
        IntVec newAtlasDimensions = atlasAlloc->nextAtlasSize(atlasDimensions);
        VERIFY(Ui, newAtlasDimensions.x() == atlasDimensions.x() || atlasDimensions.x() == 0);
        atlasDimensions = newAtlasDimensions;
    }
    return allocPos;
}
void Instance::deallocateTexture(IntRect) {
    VERIFY_NOT_REACHED(Ui);
}

void Instance::deallocateAllTextures() {
    nextAtlasAllocationHeight = 0;
}

void StyleConsumerBase::invalidateStyleInternal(int_t index) {
    instance()->styleConsumerUpdates[index].push_back(this);
}
void RendererBase::invalidateStyleInternal(int_t index) {
    instance()->rendererUpdates[index].push_back((uintptr_t)this);
}

void AsyncInstance::doTick() {
    timeline()->doTick();
}

}