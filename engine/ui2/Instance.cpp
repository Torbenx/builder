#include "AsyncInstance.hpp"
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
    atlasUse.destroy();
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

namespace {
    struct PaintJob {
        Instance::Blocks blocks;
        PaintEvent event;
        Instance* instance = nullptr;
        IntVec atlasDimensions;
        stx::DynArray<std::vector<std::byte>> atlasUpdates;
        sw::Semaphore atlasUse;

        void paint() {
            log("paint");
            sw::TextureView atlas = instance->atlas;
            sw::TextureView framebuffer = event.output;

            auto multiply = [](uint32_t in, uint32_t alpha) {
                return (((in * alpha / 255) & 0xff0000) | (((in & 0xff00) * alpha / 255) & 0xff00) | ((in & 0xff) * alpha / 255));
            };
            int_t blockCount = blocks.blockOffsets.size() / (blocks.blockLayers + 1);
            for (int_t k = 0; k < blockCount; k++) {
                ui::IntVec* block = blocks.blockOffsets.data() + k * (blocks.blockLayers + 1);
                ui::IntVec screenPos = block[0];
                int_t h = std::min(16, framebuffer.dimensions.y() - screenPos.y());
                for (int_t j = 0; j < h; j++) {
                    for (int_t i = 0; i < 16; i++) {
                        uint32_t color = (framebuffer.pos() + screenPos)[j][i];
                        for (int_t layer = 0; layer < blocks.blockLayers; layer++) {
                            ui::IntVec layerPos = block[layer + 1];
                            if (layerPos == ui::IntVec {})
                                break;
                            uint32_t in = (atlas.pos() + layerPos)[j][i];
                            uint32_t alpha = in >> 24;
                            color = multiply(in & 0xff'ff'ff, alpha) + multiply(color, 255 - alpha);
                            // color = in & 0xff'ff'ff;
                        }
                        (framebuffer.pos() + screenPos)[j][i] = color;
                    }
                }
            }

            atlasUse.signal();
            event.signal.signal();
            delete this;
        }

        void updateAtlas() {
            log("updateAtlas");
            sw::TextureView atlas = instance->atlas;
            if (atlasDimensions != atlas.dimensions)
                atlas = instance->atlas = sw::allocateTexture(atlasDimensions);

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
                    sw::TextureView texture { atlas.pos() + output.atlasRect().bottomLeft(), output.atlasRect().dimensions() };
                    sw::TextureView paddedTexture { atlas.pos() + paddedRect.bottomLeft(), paddedRect.dimensions() };
                    for (int_t j = 0; j < paddedTexture.dimensions.y(); j++) {
                        for (int_t i = 0; i < paddedTexture.dimensions.x(); i++) {
                            paddedTexture.pos()[j][i] = 0;
                        }
                    }
                    record.render(texture, output.renderDataPtr());
                }
            }
            atlasUpdates.clear();

            event.waitFor.thenAsync({ *this, stx::CallMember<&PaintJob::paint>() });
        }
    };
}

void Instance::doTick() {
    beginTick();
    executeScripts();

    for (int_t i = 0; i < styleConsumerUpdates.size(); i++) {
        auto& updates = styleConsumerUpdates[i];
        const auto& record = StyleConsumerBase::record(i);
        for (StyleConsumerBase* toUpdate : updates)
            record.updateStyle(toUpdate);
        updates.clear();
    }
    for (int_t i = 0; i < rendererUpdates.size(); i++) {
        auto& updates = rendererUpdates[i];
        const auto& record = RendererBase::record(i);
        for (uintptr_t& toUpdateUint : updates) {
            RendererBase* toUpdate = (RendererBase*)toUpdateUint;
            RenderOutputUtil output = allocateRenderOutput(renders[toUpdate->id]);
            record.updateStyleAndPrepareRender(toUpdate, output.renderDataPtr());
            toUpdateUint = (uintptr_t)toUpdate->id;
        }
    }

    updateLayout(true);

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

    if (paintEvent().has_value()) {
        PaintJob* job = new PaintJob;
        job->atlasUpdates = std::move(incrementalRenderOutput);
        incrementalRenderOutput = stx::DynArray<std::vector<std::byte>> { RendererBase::recordCount() };
        outputGeneration += 1;
        job->blocks = buildBlocks();
        job->instance = this;
        job->atlasDimensions = atlasDimensions;
        job->event = std::move(paintEvent().value());
        paintEvent().reset();

        atlasUse.thenAsync({ *job, stx::CallMember<&PaintJob::updateAtlas>() });
        atlasUse = sw::Semaphore::create();
        job->atlasUse = atlasUse.fork();
    }

    endTick();
}

void Instance::resize() {
    updateInhom(windowEquations[1], windowDimensions.x() * 1_px);
    updateInhom(windowEquations[3], windowDimensions.y() * 1_px);

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
            render.handle.prepareRender(output.renderDataPtr());
        }
        IntRect atlasRect { BottomLeft(render.allocPosition), Dimensions(screenRect.dimensions()) };
        output.atlasRect() = atlasRect;
    };

    if (clearTextures) {
        deallocateAllTextures();
        for (int_t i = 1; i < (int_t)renders.size(); i++) {
            auto& render = renders[i];
            IntRect rect = intRect(render.rect);
            render.allocPosition = allocateTexture(rect.dimensions());
            updateRenderTexture(render, rect);
        }
    } else {
        std::vector<int_t> textureUpdates;
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
            textureUpdates.push_back(i);
        }
        for (int_t updateId : textureUpdates) {
            auto& render = renders[updateId];
            IntRect rect = intRect(render.rect);
            render.allocPosition = allocateTexture(rect.dimensions());
            updateRenderTexture(render, rect);
        }
    }

    varValues = std::move(newVarValues);
}

Instance::Blocks Instance::buildBlocks() {
    std::vector<IntVec> blockOffsets;
    int_t blockLayers = 1;
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
        // log("blockRect: ({}, {}) - ({}, {})", blockRect.left(), blockRect.bottom(), blockRect.right(), blockRect.top());
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
                    log("layer {} used: ({}, {})", layer, blockOffsets[blockIndex + 1 + layer].x(), blockOffsets[blockIndex + 1 + layer].y());
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

void AsyncInstance::asyncPaint(sw::TextureView output, sw::Semaphore waitFor, sw::Semaphore signal) {
    timeline()->paintRing[tick()] = PaintEvent { output, std::move(waitFor), std::move(signal) };
}

}