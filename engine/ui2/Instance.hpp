#pragma once

#include "containers/ArrayView.hpp"
#include "containers/IdAllocator.hpp"
#include "math/sparse/Vector.hpp"
#include "platform/thread_local_storage.hpp"
#include "handles.hpp"
#include "render.hpp"
#include "util/types.hpp"
#include <memory>
#include <vector>

namespace ui {

class Instance {
public:
    Instance(AtlasAllocator* atlasAlloc);
    ~Instance();

    VarHandle newVar();
    RangeVarHandle newRange();
    RectVarHandle newRect();
    EquationHandle newEquation();
    void setEquation(EquationHandle, ArrayView<const sparse::Entry>);
    void updateInhom(EquationHandle, Size inhom);
    void removeEqn(EquationHandle);
    void deleteVariable(VarHandle);
    void deleteVariable(RangeVarHandle);
    void deleteVariable(RectVarHandle);

    void resize(IntVec windowSize, float ptScale);
    void update();

    void draw(RendererHandle handle, RectVarHandle rect);
    void cancelDraw(RendererBase&);
    TextureState textureState(const RendererBase&);

    RectVarHandle screen() const { return screenBox; }

    float valueOf(VarHandle var) const;
    void dumpVarValues();
    void dumpShiftValues();
    void dumpShiftSizes();
    IntRect intRect(RectVarHandle var) const { return intRect(var, varValues.data()); }
    void debug(RectVarHandle box) const {
        log("Box: {}, {} - {}, {}", varValues[box.left().id], varValues[box.bottom().id], varValues[box.right().id], varValues[box.top().id]);
    }

private:
    struct Render {
        RendererHandle handle;
        RectVarHandle rect;
        IntVec allocPosition = {};
    };

    IdAllocator<uint32_t> eqnIdAlloc;
    uint32_t nextVarId = 4;
    std::vector<Size> inhomTyped;
    std::vector<float> inhomValues;
    std::vector<float> varValues;
    sparse::Matrix matrix;
    RectVarHandle screenBox;
    EquationHandle screenEquations[4] = {};
    IntVec screenDimensions;
    float ptScale = 1.0f;
    std::vector<Render> renders { 1 };

    uint32_t allocateVariableIds(int_t count);
    static IntRect intRect(RectVarHandle var, const float* varValues) {
        return {
            IntRange { (int)varValues[var.left().id], (int)varValues[var.right().id] },
            IntRange { (int)varValues[var.bottom().id], (int)varValues[var.top().id] }
        };
    }

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

    std::vector<uint32_t> outputBlockMap;

public:
    int_t outputLayers = 1;
    std::vector<IntVec> outputOffsets;
    TextureView atlasView() const { return atlas.view(); }
};

inline Instance* instance() { return (Instance*)plt::getThreadLocalPtr<plt::TLS_UI_INSTANCE_INDEX>(); }
inline void setInstance(Instance* instance) { plt::setThreadLocalPtr<plt::TLS_UI_INSTANCE_INDEX>(instance); }

}