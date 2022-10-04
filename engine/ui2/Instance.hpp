#pragma once

#include "event/Window.hpp"
#include "containers/DynArray.hpp"
#include "containers/IdAllocator.hpp"
#include "handles.hpp"
#include "math/sparse/Vector.hpp"
#include "platform/thread_local_storage.hpp"
#include "render.hpp"
#include "util/types.hpp"
#include <memory>
#include <vector>

namespace ui {

class LayoutStylableBase;

class Instance  : public evt::Window {
public:
    Instance(const evt::WindowBackend* window, AtlasAllocator* atlasAlloc);
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

    void draw(RendererHandle handle, RectVarHandle rect);
    void cancelDraw(RendererBase&);
    TextureState textureState(const RendererBase&);

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

    struct Render {
        RendererHandle handle;
        RectVarHandle rect;
        IntVec allocPosition = {};
    };
    std::vector<Render> renders { 1 };

    struct AtlasHelper : TextureHandle {
        AtlasAllocator* allocator = nullptr;
        AtlasHelper(AtlasAllocator* allocator)
            : allocator(allocator) { }
        AtlasHelper(AtlasAllocator* allocator, IntVec prevSize)
            : AtlasHelper(allocator) {
            (TextureHandle&)* this = allocator->allocateAtlas(prevSize);
        }
        AtlasHelper(AtlasHelper&& other)
            : TextureHandle(other), allocator(other.allocator) {
            (TextureHandle&)other = {};
        }
        ~AtlasHelper() {
            maybeDeallocate();
        }
        AtlasHelper& operator=(AtlasHelper&& other) {
            maybeDeallocate();
            allocator = other.allocator;
            (TextureHandle&)* this = other;
            (TextureHandle&)other = {};
            return *this;
        }

        void maybeDeallocate() {
            if (*this)
                allocator->deallocateAtlas(*this);
        }
        AtlasHelper allocateNext() const {
            return { allocator, dimensions };
        }
    };
    AtlasHelper atlas;
    const int atlasPadding = 16;
    int nextAtlasAllocationHeight = 0;
    IntVec allocateTexture(IntVec dimensions);
    void deallocateTexture(IntRect region);
    void deallocateAllTextures();

    int_t outputLayers = 1;
    std::vector<uint32_t> outputBlockMap;
    std::vector<IntVec> outputOffsets;
    TextureView atlasView() const { return atlas.view(); }

    stx::DynArray<std::vector<LayoutStylableBase*>> layoutStylableUpdates;

    evt::ExecutorImpl coreExe;
    evt::ExecutorImpl styleComputationExe;
    evt::ExecutorImpl stylingExe;
    evt::ExecutorImpl renderingExe;
    void doTick();
};

inline Instance* instance() { return (Instance*)plt::getThreadLocalPtr<plt::TLS_UI_INSTANCE_INDEX>(); }
inline void setInstance(Instance* instance) { plt::setThreadLocalPtr<plt::TLS_UI_INSTANCE_INDEX>(instance); }

}