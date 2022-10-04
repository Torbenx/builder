#include "Instance.hpp"
#include "Stylable.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "util/log.hpp"

namespace ui {

Instance::Instance(const evt::WindowBackend* window, AtlasAllocator* atlasAlloc)
    : evt::Window(window), atlas(atlasAlloc) {

    windowRect = newRect();

    [](Instance* instance) -> evt::Script {
        co_await evt::OnExecutor(&instance->coreExe);
        IntVec initialDim = instance->dimensions();
        if (initialDim.x() > 0 && initialDim.y() > 0)
            instance->resize();
        while (true) {
            evt::EventReceiver<evt::ResizeEvent> events;
            co_await (instance->resizeEvent() > events || instance->destroyed());
            if (instance->destroyed())
                break;
            instance->resize();
        }
    }(this);

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

void Instance::doTick() {
    evt::setCurrentExecutor(&coreExe);
    coreExe.execute();

    evt::setCurrentExecutor(&stylingExe);
    styleComputationExe.execute();
    stylingExe.execute();

    evt::setCurrentExecutor(&renderingExe);
    updateLayout(true);
    renderingExe.execute();

    evt::setCurrentExecutor(nullptr);
    rebuildOutput();
}

void Instance::resize() {
    updateInhom(windowEquations[1], dimensions().x() * 1_px);
    updateInhom(windowEquations[3], dimensions().y() * 1_px);

    IntVec windowBlocks = (dimensions() + 15) / 16;
    outputBlockMap.clear();
    outputBlockMap.resize(windowBlocks.x() * windowBlocks.y());
}

void Instance::updateLayout(bool clearTextures) {
    sparse::LUDecomposition lu;
    lu.u = matrix.copy();
    int_t varCount = nextVarId;
    lu.decompose(varCount);

    Size::asPixels(inhomTyped.data(), inhomValues.data(), inhomTyped.size(), ptScale, 1.0f);

    std::vector<float> newVarValues(varCount);
    lu.inverseTranspose(inhomValues.data(), newVarValues.data());

    if (clearTextures) {
        deallocateAllTextures();
        for (int_t i = 1; i < (int_t)renders.size(); i++) {
            auto& render = renders[i];
            IntRect rect = intRect(render.rect);
            render.allocPosition = allocateTexture(rect.dimensions());
            render.handle.signal.send();
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
            render.handle.signal.send();
        }
    }

    varValues = std::move(newVarValues);
}

void Instance::rebuildOutput() {
    outputOffsets.clear();
    static constexpr uint32_t INVALID_BLOCK_ID = -1;
    for (uint32_t& u : outputBlockMap)
        u = INVALID_BLOCK_ID;
    IntVec blockDimensions = (dimensions() + 15) / 16;
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
                uint32_t& blockId = outputBlockMap[block.y() * blockDimensions.x() + block.x()];
                // log("block: ({}, {}), id = {:#x}", block.x(), block.y(), blockId);
                if (blockId == INVALID_BLOCK_ID) {
                    blockId = outputOffsets.size() / (outputLayers + 1);
                    VERIFY(Ui, outputOffsets.size() % (outputLayers + 1) == 0);
                    outputOffsets.push_back(block * 16);
                    outputOffsets.resize(outputOffsets.size() + outputLayers);
                }
                uint32_t blockIndex = blockId * (outputLayers + 1);
                int_t layer = 0;
                for (; layer < outputLayers; layer++) {
                    if (outputOffsets[blockIndex + 1 + layer] == IntVec {})
                        break;
                    log("layer {} used: ({}, {})", layer, outputOffsets[blockIndex + 1 + layer].x(), outputOffsets[blockIndex + 1 + layer].y());
                }
                if (layer == outputLayers) {
                    VERIFY_NOT_REACHED(Ui);
                    continue;
                }
                outputOffsets[blockIndex + 1 + layer] = render.allocPosition - rect.bottomLeft() + block * 16;
            }
        }
    }
}

void Instance::draw(RendererHandle handle, RectVarHandle rect) {
    if (handle->id == 0) {
        handle->id = renders.size();
        renders.push_back({ handle, rect });
    } else {
        renders[handle->id].rect = rect;
    }
}
void Instance::cancelDraw(RendererBase& ref) {
    RendererBase* base = &ref;
    VERIFY(Ui, base->id != 0);

    Render back = renders.back();
    renders.pop_back();
    back.handle->id = base->id;
    base->id = 0;

    renders[back.handle->id] = back;
}
TextureState Instance::textureState(const RendererBase& ref) {
    const RendererBase* base = &ref;
    VERIFY(Ui, base->id != 0);
    Render& render = renders[base->id];
    return {
        .texture = atlas.view() + render.allocPosition,
        .dimensions = intRect(render.rect).dimensions(),
    };
}
void draw(RendererHandle handle, RectVarHandle rect) {
    instance()->draw(handle, rect);
}
void cancelDraw(RendererBase& base) {
    instance()->cancelDraw(base);
}
TextureState RendererBase::textureState() const {
    return instance()->textureState(*this);
}

IntVec Instance::allocateTexture(IntVec dimensions) {
    VERIFY(Ui, dimensions.x() <= atlas.dimensions.x());
    IntVec allocPos { atlasPadding, nextAtlasAllocationHeight + atlasPadding };
    nextAtlasAllocationHeight += atlasPadding + dimensions.y();
    int_t neededHeight = nextAtlasAllocationHeight + atlasPadding;
    if (neededHeight >= atlas.dimensions.y()) {
        AtlasHelper newAtlas = atlas.allocateNext();
        VERIFY(Ui, newAtlas.dimensions.x() == atlas.dimensions.x());
        std::copy_n(atlas.data, atlas.dimensions.x() * atlas.dimensions.y(), newAtlas.data);
        atlas = std::move(newAtlas);
    }
    return allocPos;
}
void Instance::deallocateTexture(IntRect) {
    VERIFY_NOT_REACHED(Ui);
}

void Instance::deallocateAllTextures() {
    nextAtlasAllocationHeight = 0;
    std::fill_n(atlas.data, atlas.dimensions.x() * atlas.dimensions.y(), 0);
}

float Instance::valueOf(VarHandle var) const { return varValues[var.id]; }
float VarHandle::value() const { return instance()->valueOf(*this); }

void EquationHandle::updateShift(Size shift) const {
    instance()->updateInhom(*this, (shiftFactor.value() * shift).withPayloadIf(shiftFactor.tag()));
}
void EquationHandle::updateEquation(ArrayView<const sparse::Entry> terms) const {
    instance()->setEquation(*this, terms);
}
EquationHandle EquationHandle::allocate() { return instance()->newEquation(); }
void EquationHandle::deallocate(EquationHandle handle) { instance()->removeEqn(handle); }

VarHandle VarHandle::allocate() { return instance()->newVar(); }
void VarHandle::deallocate(VarHandle handle) { instance()->deleteVariable(handle); }
RangeVarHandle RangeVarHandle::allocate() { return instance()->newRange(); }
void RangeVarHandle::deallocate(RangeVarHandle handle) { instance()->deleteVariable(handle); }
RectVarHandle RectVarHandle::allocate() { return instance()->newRect(); }
void RectVarHandle::deallocate(RectVarHandle handle) { instance()->deleteVariable(handle); }

RectVarHandle windowRect() { return instance()->windowRect; }

evt::Executor* coreExecutor() { return &instance()->coreExe; }
evt::Executor* stylingExecutor() { return &instance()->stylingExe; }
evt::Executor* renderingExecutor() { return &instance()->renderingExe; }

void StylableBase::invalidateStyleInternal(void (*computeFunc)(StylableBase*)) {
    onStyleDirty.send();
    [](StylableBase* thiz, void (*computeFunc)(StylableBase*)) -> evt::Script {
        co_await evt::OnExecutor(&instance()->styleComputationExe);
        computeFunc(thiz);
        thiz->isDirty = false;
    }(this, computeFunc);
}

}