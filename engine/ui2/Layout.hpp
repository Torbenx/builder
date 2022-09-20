#pragma once

#include "BigStyle.hpp"
#include "Rect.hpp"
#include "containers/Array.hpp"
#include "containers/IdAllocator.hpp"
#include "math/sparse/Vector.hpp"
#include "platform/thread_local_storage.hpp"
#include "util/types.hpp"
#include <memory>
#include <vector>

namespace ui {

struct TextureView {
    struct LineView {
        uint32_t* data = nullptr;
        constexpr uint32_t& operator[](int_t i) const { return data[i]; }
    };
    uint32_t* data = nullptr;
    int_t stride = 0;
    constexpr LineView operator[](int_t j) const { return { data + j * stride }; }
    constexpr TextureView shift(const IntVec& shift) const {
        return { data + shift.y() * stride + shift.x(), stride };
    }
};
constexpr TextureView operator+(const TextureView& view, const IntVec& shift) {
    return view.shift(shift);
}
constexpr TextureView operator+(const IntVec& shift, const TextureView& view) {
    return view.shift(shift);
}
constexpr TextureView operator-(const TextureView& view, const IntVec& shift) {
    return view.shift(-shift);
}

struct TextureHandle {
    uint32_t* data = nullptr;
    IntVec dimensions;
    constexpr TextureView view() const { return { data, dimensions.x() }; }
    constexpr explicit operator bool() const { return data != nullptr; }
};
struct AtlasAllocator {
    virtual TextureHandle allocateAtlas(IntVec prevSize) = 0;
    virtual void deallocateAtlas(TextureHandle texture) = 0;
};

struct RendererBase {
    uint32_t id = 0;
};
template<typename T>
concept Renderer = std::derived_from<T, RendererBase> && requires(T& render, TextureView view, IntVec dimensions) { render.updateTexture(view, dimensions); };

template<int_t N>
struct StaticSum {
    static constexpr int_t STATIC_SUM_SIZE = N;
    stx::Array<sparse::Entry, N> terms;
};
template<int_t N>
struct ShiftedStaticSum : StaticSum<N> {
    Size shift = 0_px;
    float shiftFactor = 1.0f;
};
template<int_t N>
struct StaticEquation {
    StaticSum<N> equalsShift;
    Size shift = 0_px;
    TagFloat<1> shiftFactor { 1.0f, 0 };
};

template<typename In>
concept StaticSummable = In::STATIC_SUM_SIZE >= 0 && std::is_convertible_v<In, StaticSum<In::STATIC_SUM_SIZE>> && !std::is_same_v<In, ShiftedStaticSum<In::STATIC_SUM_SIZE>>;

namespace code {
    template<StaticSummable In>
    constexpr StaticSum<In::STATIC_SUM_SIZE> operator*(const In& l, float f) {
        StaticSum<In::STATIC_SUM_SIZE> out = l;
        for (auto& term : out.terms)
            term.value *= f;
        return out;
    }
    template<StaticSummable In>
    constexpr StaticSum<In::STATIC_SUM_SIZE> operator*(float f, const In& in) {
        return in * f;
    }
    template<StaticSummable In>
    constexpr StaticSum<In::STATIC_SUM_SIZE> operator/(const In& in, float f) {
        return in * (1.0f / f);
    }

    template<int_t N>
    constexpr ShiftedStaticSum<N> operator*(const ShiftedStaticSum<N>& in, float f) {
        return { (const StaticSum<N>&)in * f, in.shift, in.shiftFactor * f };
    }
    template<int_t N>
    constexpr ShiftedStaticSum<N> operator*(float f, const ShiftedStaticSum<N>& in) { return in * f; }
    template<int_t N>
    constexpr ShiftedStaticSum<N> operator/(const ShiftedStaticSum<N>& in, float f) { return in * (1.0f / f); }

    template<int_t N1, int_t N2>
    constexpr StaticSum<N1 + N2> operator+(const StaticSum<N1>& in1, const StaticSum<N2>& in2) {
        StaticSum<N1 + N2> out;
        std::copy_n(in1.terms.data(), N1, out.terms.data());
        std::copy_n(in2.terms.data(), N2, out.terms.data() + N1);
        return out;
    }
    template<StaticSummable In1, StaticSummable In2>
    constexpr StaticSum<In1::STATIC_SUM_SIZE + In2::STATIC_SUM_SIZE> operator+(const In1& in1, const In2& in2) {
        return operator+<In1::STATIC_SUM_SIZE, In2::STATIC_SUM_SIZE>(in1, in2);
    }
    template<StaticSummable In1, StaticSummable In2>
    constexpr StaticSum<In1::STATIC_SUM_SIZE + In2::STATIC_SUM_SIZE> operator-(const In1& in1, const In2& in2) {
        return operator+<In1::STATIC_SUM_SIZE, In2::STATIC_SUM_SIZE>(in1, in2 * -1.0f);
    }

    template<StaticSummable In1, int_t N2>
    constexpr ShiftedStaticSum<In1::STATIC_SUM_SIZE + N2> operator+(const In1& in1, const ShiftedStaticSum<N2>& in2) {
        return { in1 + (const StaticSum<N2>&)in2, in2.shift, in2.shiftFactor };
    }
    template<int_t N1, StaticSummable In2>
    constexpr ShiftedStaticSum<N1 + In2::STATIC_SUM_SIZE> operator+(const ShiftedStaticSum<N1>& in1, const In2& in2) {
        return { (const StaticSum<N1>&)in1 + in2, in1.shift, in1.shiftFactor };
    }
    template<StaticSummable In1, int_t N2>
    constexpr ShiftedStaticSum<In1::STATIC_SUM_SIZE + N2> operator-(const In1& in1, const ShiftedStaticSum<N2>& in2) {
        return { in1 - (const StaticSum<N2>&)in2, in2.shift, -in2.shiftFactor };
    }
    template<int_t N1, StaticSummable In2>
    constexpr ShiftedStaticSum<N1 + In2::STATIC_SUM_SIZE> operator-(const ShiftedStaticSum<N1>& in1, const In2& in2) {
        return { (const StaticSum<N1>&)in1 - in2, in1.shift, in1.shiftFactor };
    }

    constexpr ShiftedStaticSum<0> shift(Size shift = 0_px) {
        return { {}, shift };
    }

    template<StaticSummable In1, StaticSummable In2>
    constexpr StaticEquation<In1::STATIC_SUM_SIZE + In2::STATIC_SUM_SIZE> operator==(const In1& in1, const In2& in2) {
        return { .equalsShift = in2 - in1 };
    }
    template<StaticSummable In1, StaticSummable In2>
    constexpr StaticEquation<In1::STATIC_SUM_SIZE + In2::STATIC_SUM_SIZE> operator||(const In1& in1, const In2& in2) {
        return { .equalsShift = in2 - in1, .shiftFactor { 1.0f, 1 } };
    }
    template<int_t N1, StaticSummable In2>
    constexpr StaticEquation<N1 + In2::STATIC_SUM_SIZE> operator==(const ShiftedStaticSum<N1>& in1, const In2& in2) {
        return { .equalsShift = in2 - (const StaticSum<N1>&)in1, .shift = in1.shift, .shiftFactor { in1.shiftFactor, 0 } };
    }
    template<int_t N1, StaticSummable In2>
    constexpr StaticEquation<N1 + In2::STATIC_SUM_SIZE> operator||(const ShiftedStaticSum<N1>& in1, const In2& in2) {
        return { .equalsShift = in2 - (const StaticSum<N1>&)in1, .shift = in1.shift, .shiftFactor { in1.shiftFactor, 1 } };
    }
    template<StaticSummable In1, int_t N2>
    constexpr StaticEquation<In1::STATIC_SUM_SIZE + N2> operator==(const In1& in1, const ShiftedStaticSum<N2>& in2) {
        return { .equalsShift = (const StaticSum<N2>&)in2 - in1, .shift = in2.shift, .shiftFactor { -in2.shiftFactor, 0 } };
    }
    template<StaticSummable In1, int_t N2>
    constexpr StaticEquation<In1::STATIC_SUM_SIZE + N2> operator||(const In1& in1, const ShiftedStaticSum<N2>& in2) {
        return { .equalsShift = (const StaticSum<N2>&)in2 - in1, .shift = in2.shift, .shiftFactor { -in2.shiftFactor, 1 } };
    }
}

struct VarHandle {
    static constexpr int_t STATIC_SUM_SIZE = 1;
    uint32_t id = 0;
    constexpr explicit operator bool() const { return id != 0; }
    constexpr operator StaticSum<1>() const { return { { { { id, 1.0f } } } }; }
    float value() const;
};
struct RangeVarHandle {
    uint32_t baseId = 0;
    constexpr const VarHandle low() const { return { baseId }; }
    constexpr const VarHandle high() const { return { baseId + 1 }; }
    constexpr explicit operator bool() const { return baseId != 0; }
};
struct RectVarHandle {
    uint32_t baseId = 0;
    constexpr const RangeVarHandle x() const { return { baseId }; }
    constexpr const RangeVarHandle y() const { return { baseId + 2 }; }
    constexpr const RangeVarHandle axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    constexpr const VarHandle left() const { return x().low(); }
    constexpr const VarHandle right() const { return x().high(); }
    constexpr const VarHandle bottom() const { return y().low(); }
    constexpr const VarHandle top() const { return y().high(); }
    constexpr explicit operator bool() const { return baseId != 0; }
};

struct EquationHandle {
    uint32_t id = 0;
    TagFloat<1> shiftFactor { 1.0f, 0 };

    void updateShift(Size shift) const;
};

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
    void doDraw();
    IntRect intRect(RectVarHandle var) const { return intRect(var, varValues.data()); }

    RectVarHandle screen() const { return screenBox; }
    float valueOf(VarHandle var) const { return varValues[var.id]; }
    void dumpVarValues();
    void dumpShiftValues();
    void dumpShiftSizes();
    void debug(RectVarHandle box) const {
        log("Box: {}, {} - {}, {}", varValues[box.left().id], varValues[box.bottom().id], varValues[box.right().id], varValues[box.top().id]);
    }

    template<Renderer T>
    void draw(T& renderer, RectVarHandle rect) {
        RendererHandle handle;
        handle.base = (RendererBase*)&renderer;
        handle.updatePtr = [](RendererBase* base, TextureView view, IntVec dimensions) {
            static_cast<T*>(base)->updateTexture(view, dimensions);
        };
        draw(handle, rect);
    }
    void cancelDraw(RendererBase&);

private:
    struct RendererHandle {
        RendererBase* base = nullptr;
        void (*updatePtr)(RendererBase*, TextureView, IntVec) = nullptr;
        RendererBase* operator->() const { return base; }
        void update(TextureView view, IntVec dimensions) const { updatePtr(base, view, dimensions); }
    };
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
    void draw(RendererHandle, RectVarHandle);
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

inline float VarHandle::value() const { return instance()->valueOf(*this); }

inline void EquationHandle::updateShift(Size shift) const {
    instance()->updateInhom(*this, (shiftFactor.value() * shift).withPayloadIf(shiftFactor.tag()));
}

struct Var : VarHandle {
    Var()
        : VarHandle(instance()->newVar()) { }
    Var(Var&& other)
        : VarHandle(other) { (VarHandle&)other = {}; }
    Var& operator=(Var&& other) {
        if (*this)
            instance()->deleteVariable(*this);
        (VarHandle&)* this = other;
        (VarHandle&)other = {};
        return *this;
    }
    ~Var() {
        if (*this)
            instance()->deleteVariable(*this);
    }
};
struct RangeVar : RangeVarHandle {
    RangeVar()
        : RangeVarHandle(instance()->newRange()) { }
    RangeVar(RangeVar&& other)
        : RangeVarHandle(other) { (RangeVarHandle&)other = {}; }
    RangeVar& operator=(RangeVar&& other) {
        if (*this)
            instance()->deleteVariable(*this);
        (RangeVarHandle&)* this = other;
        (RangeVarHandle&)other = {};
        return *this;
    }
    ~RangeVar() {
        if (*this)
            instance()->deleteVariable(*this);
    }
};
struct RectVar : RectVarHandle {
    RectVar()
        : RectVarHandle(instance()->newRect()) { }
    RectVar(RectVar&& other)
        : RectVarHandle(other) { (RectVarHandle&)other = {}; }
    RectVar& operator=(RectVar&& other) {
        if (*this)
            instance()->deleteVariable(*this);
        (RectVarHandle&)* this = other;
        (RectVarHandle&)other = {};
        return *this;
    }
    ~RectVar() {
        if (*this)
            instance()->deleteVariable(*this);
    }
};

struct Equation : EquationHandle {
    Equation() = default;
    Equation(Equation&& other)
        : EquationHandle(other) {
        (EquationHandle&)other = {};
    }
    template<int_t N>
    Equation(const StaticEquation<N>& eq) 
        : EquationHandle(instance()->newEquation()) {
        shiftFactor = eq.shiftFactor;
        instance()->setEquation(*this, eq.equalsShift.terms);
        updateShift(eq.shift);
    }
    Equation& operator=(Equation&& other) {
        if (*this)
            instance()->removeEqn(*this);
        (EquationHandle&)* this = other;
        (EquationHandle&)other = {};
        return *this;
    }
    ~Equation() {
        if (*this)
            instance()->removeEqn(*this);
    }
    constexpr explicit operator bool() const { return id != 0; }
};

struct RangeEqn {
    Equation m_low, m_high;
    constexpr Equation& low() { return m_low; }
    constexpr const Equation& low() const { return m_low; }
    constexpr Equation& high() { return m_high; }
    constexpr const Equation& high() const { return m_high; }
};
struct RectEqn {
    RangeEqn m_x, m_y;
    constexpr RangeEqn& x() { return m_x; }
    constexpr const RangeEqn& x() const { return m_x; }
    constexpr RangeEqn& y() { return m_y; }
    constexpr const RangeEqn& y() const { return m_y; }
    constexpr RangeEqn& axis(Axis axis) { return axis == Axis::X ? x() : y(); }
    constexpr const RangeEqn& axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    constexpr Equation& left() { return x().low(); }
    constexpr const Equation& left() const { return x().low(); }
    constexpr Equation& right() { return x().high(); }
    constexpr const Equation& right() const { return x().high(); }
    constexpr Equation& bottom() { return y().low(); }
    constexpr const Equation& bottom() const { return y().low(); }
    constexpr Equation& top() { return y().high(); }
    constexpr const Equation& top() const { return y().high(); }
};

/*struct UiElement {
    virtual void setStyle(BigStyle) { }
    virtual ~UiElement() { }
};

struct LayoutBox : UiElement {
    Rect padding;
    RectVar outerBox, innerBox;
    RectEqn paddingEqn;
    LayoutBox() {
        paddingEqn = instance()->align(innerBox, outerBox);
    }
    ~LayoutBox() {
        instance()->removeEqn(paddingEqn);
    }
    void updatePaddings() { instance()->updateInhom(paddingEqn, padding); }
    void setStyle(BigStyle) override { }
};*/

}