#pragma once

#include "Renderer.hpp"
#include "containers/DynArray.hpp"
#include "containers/IdAllocator.hpp"
#include "event/Timeline.hpp"
#include "event/Window.hpp"
#include "handles.hpp"
#include "math/sparse/Vector.hpp"
#include "platform/thread_local_storage.hpp"
#include "util/types.hpp"
#include <memory>
#include <vector>

namespace ui {

struct AtlasAllocator {
    virtual IntVec nextAtlasSize(IntVec prevSize) = 0;
};

struct RenderOutputUtil {
    std::byte* output = nullptr;
    static int_t outputSize(int_t classIndex) {
        return RendererBase::record(classIndex).renderDataSizeAlign8 + sizeof(IntRect);
    }
    RenderOutputUtil() = default;
    RenderOutputUtil(std::byte* output)
        : output(output) { }
    void* renderDataPtr() {
        return output + sizeof(IntRect);
    }
    IntRect& atlasRect() {
        return *(IntRect*)output;
    }
};

class Instance {
public:
    Instance(evt::Timeline* timeline, const evt::Source<evt::WindowStateUpdate>& stateUpdate, const evt::Source<evt::ResizeEvent>& resizeEvent, AtlasAllocator* atlasAlloc);
    ~Instance();

    evt::Timeline* timeline = nullptr;
    const evt::Source<evt::WindowStateUpdate>& windowState;

    IdAllocator<uint32_t> eqnIdAlloc;
    std::vector<Size> inhomTyped;
    std::vector<float> inhomValues;
    sparse::Matrix matrix;
    EquationHandle newEquation();
    void setEquation(EquationHandle, ArrayView<const sparse::Entry>);
    void updateInhom(EquationHandle, Size inhom);
    void removeEqn(EquationHandle);

    uint32_t nextVarId = 4;
    std::vector<float> varValues;
    uint32_t allocateVariableIds(int_t count);
    VarHandle newVar();
    RangeVarHandle newRange();
    RectVarHandle newRect();
    void deleteVariable(VarHandle);
    void deleteVariable(RangeVarHandle);
    void deleteVariable(RectVarHandle);

    void resize();
    void updateLayout(bool clearTextures);
    void rebuildOutput();

    float valueOf(VarHandle var) const { return varValues[var.id]; }
    void dumpVarValues();
    void dumpShiftValues();
    void dumpShiftSizes();
    static IntRect intRect(RectVarHandle var, const float* varValues) {
        return {
            IntRange { (int)varValues[var.left().id], (int)varValues[var.right().id] },
            IntRange { (int)varValues[var.bottom().id], (int)varValues[var.top().id] }
        };
    }
    IntRect intRect(RectVarHandle var) const { return intRect(var, varValues.data()); }

    RectVarHandle windowRect;
    EquationHandle windowEquations[4] = {};
    IntVec windowDimensions;
    float ptScale = 1.0f;

    AtlasAllocator* atlasAlloc = nullptr;
    IntVec atlasDimensions;
    const int atlasPadding = 16;
    int nextAtlasAllocationHeight = 0;
    IntVec allocateTexture(IntVec dimensions);
    void deallocateTexture(IntRect region);
    void deallocateAllTextures();

    int_t outputLayers = 1;
    std::vector<uint32_t> outputBlockMap;
    std::vector<IntVec> outputOffsets;

    struct Render {
        RendererHandle handle;
        RectVarHandle rect;
        IntVec allocPosition = {};
        uint32_t lastOutputGen = 0;
        uint32_t outputOffsetBytes = 0;
    };
    std::vector<Render> renders { 1 };
    stx::DynArray<std::vector<uintptr_t>> rendererUpdates;
    stx::DynArray<std::vector<std::byte>> incrementalRenderOutput;
    uint32_t outputGeneration = 1;
    std::byte* findRenderOutput(const Render& render);
    std::byte* allocateRenderOutput(Render& render);
    void draw(RendererHandle, RectVarHandle rect);
    void cancelDraw(RendererHandle);

    stx::DynArray<std::vector<StyleConsumerBase*>> styleConsumerUpdates;

    void doTick();
};

DECLARE_THREAD_LOCAL_GETSET(Instance*, instance, setInstance, UI_INSTANCE)

inline const evt::Source<evt::WindowStateUpdate>& stateUpdate() { return instance()->windowState; }
inline evt::WindowState state() { return stateUpdate().currentState; }
inline const evt::BinaryState& initialized() { return stateUpdate().initialized; }
inline const evt::BinaryState& visible() { return stateUpdate().visible; }
inline const evt::BinaryState& foreground() { return stateUpdate().foreground; }
inline const evt::BinaryState& destroyed() { return stateUpdate().destroyed; }

inline RectVarHandle windowRect() { return instance()->windowRect; }
inline evt::Executor* executor() { return instance()->timeline->executor(); }

inline float VarHandle::value() const { return instance()->valueOf(*this); }

inline void EquationHandle::updateShift(Size shift) const {
    instance()->updateInhom(*this, (shiftFactor.value() * shift).withPayloadIf(shiftFactor.tag()));
}
inline void EquationHandle::updateEquation(ArrayView<const sparse::Entry> terms) const {
    instance()->setEquation(*this, terms);
}
inline EquationHandle EquationHandle::allocate() { return instance()->newEquation(); }
inline void EquationHandle::deallocate(EquationHandle handle) { instance()->removeEqn(handle); }

inline VarHandle VarHandle::allocate() { return instance()->newVar(); }
inline void VarHandle::deallocate(VarHandle handle) { instance()->deleteVariable(handle); }
inline RangeVarHandle RangeVarHandle::allocate() { return instance()->newRange(); }
inline void RangeVarHandle::deallocate(RangeVarHandle handle) { instance()->deleteVariable(handle); }
inline RectVarHandle RectVarHandle::allocate() { return instance()->newRect(); }
inline void RectVarHandle::deallocate(RectVarHandle handle) { instance()->deleteVariable(handle); }

inline void draw(RendererHandle handle, RectVarHandle rect) {
    instance()->draw(handle, rect);
}
inline void cancelDraw(RendererHandle handle) {
    instance()->cancelDraw(handle);
}

}