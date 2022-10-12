#pragma once

#include "containers/DynArray.hpp"
#include "containers/IdAllocator.hpp"
#include "event/Window.hpp"
#include "handles.hpp"
#include "math/sparse/Vector.hpp"
#include "platform/thread_local_storage.hpp"
#include "Renderer.hpp"
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

class Instance : public evt::Window {
public:
    Instance(const evt::WindowBackend* window, evt::ExecutorImpl* coreExe, AtlasAllocator* atlasAlloc);
    ~Instance();

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

    float valueOf(VarHandle var) const;
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

    evt::ExecutorImpl* coreExe = nullptr;
    evt::dep::Source outputComplete;
    evt::dep::Source layoutComplete;
    void doTick(evt::Scheduler);
};

inline Instance* instance() { return (Instance*)plt::getThreadLocalPtr<plt::TLS_UI_INSTANCE_INDEX>(); }
inline void setInstance(Instance* instance) { plt::setThreadLocalPtr<plt::TLS_UI_INSTANCE_INDEX>(instance); }

}