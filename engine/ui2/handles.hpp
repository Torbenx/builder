#pragma once

#include "Rect.hpp"
#include "containers/ArrayView.hpp"
#include "event/Script.hpp"

namespace sparse {
struct Entry;
}

namespace ui {

template<int_t N>
struct StaticSum;

struct VarHandle {
    static constexpr int_t STATIC_SUM_SIZE = 1;
    static VarHandle allocate();
    static void deallocate(VarHandle);
    uint32_t id = 0;
    constexpr explicit operator bool() const { return id != 0; }
    constexpr operator StaticSum<1>() const;
    float value() const;
};
struct RangeVarHandle {
    static RangeVarHandle allocate();
    static void deallocate(RangeVarHandle);
    uint32_t baseId = 0;
    constexpr const VarHandle low() const { return { baseId }; }
    constexpr const VarHandle high() const { return { baseId + 1 }; }
    constexpr explicit operator bool() const { return baseId != 0; }
    constexpr StaticSum<2> extend() const;
};
struct RectVarHandle {
    uint32_t baseId = 0;
    static RectVarHandle allocate();
    static void deallocate(RectVarHandle);
    constexpr const RangeVarHandle x() const { return { baseId }; }
    constexpr const RangeVarHandle y() const { return { baseId + 2 }; }
    constexpr const RangeVarHandle axis(Axis axis) const { return axis == Axis::X ? x() : y(); }
    constexpr const VarHandle left() const { return x().low(); }
    constexpr const VarHandle right() const { return x().high(); }
    constexpr const VarHandle bottom() const { return y().low(); }
    constexpr const VarHandle top() const { return y().high(); }
    constexpr explicit operator bool() const { return baseId != 0; }
    constexpr StaticSum<2> width() const;
    constexpr StaticSum<2> height() const;
};

RectVarHandle screen();

struct Var : VarHandle {
    Var()
        : VarHandle(allocate()) { }
    Var(Var&& other)
        : VarHandle(other) { (VarHandle&)other = {}; }
    Var& operator=(Var&& other) {
        if (*this)
            deallocate(*this);
        (VarHandle&)* this = other;
        (VarHandle&)other = {};
        return *this;
    }
    ~Var() {
        if (*this)
            deallocate(*this);
    }
};
struct RangeVar : RangeVarHandle {
    RangeVar()
        : RangeVarHandle(allocate()) { }
    RangeVar(RangeVar&& other)
        : RangeVarHandle(other) { (RangeVarHandle&)other = {}; }
    RangeVar& operator=(RangeVar&& other) {
        if (*this)
            deallocate(*this);
        (RangeVarHandle&)* this = other;
        (RangeVarHandle&)other = {};
        return *this;
    }
    ~RangeVar() {
        if (*this)
            deallocate(*this);
    }
};
struct RectVar : RectVarHandle {
    RectVar()
        : RectVarHandle(allocate()) { }
    RectVar(RectVar&& other)
        : RectVarHandle(other) { (RectVarHandle&)other = {}; }
    RectVar& operator=(RectVar&& other) {
        if (*this)
            deallocate(*this);
        (RectVarHandle&)* this = other;
        (RectVarHandle&)other = {};
        return *this;
    }
    ~RectVar() {
        if (*this)
            deallocate(*this);
    }
};

template<int_t N>
struct StaticEquation;
struct StaticRangeEquation;
struct StaticRectEquation;

struct EquationHandle {
    static EquationHandle allocate();
    static void deallocate(EquationHandle);
    uint32_t id = 0;
    TagFloat<1> shiftFactor { 1.0f, 0 };

    void updateShift(Size shift) const;
    void updateEquation(ArrayView<const sparse::Entry>) const;
};

struct Equation : EquationHandle {
    Equation() = default;
    Equation(Equation&& other)
        : EquationHandle(other) {
        (EquationHandle&)other = {};
    }
    template<int_t N>
    Equation(const StaticEquation<N>& eq) {
        *this = eq;
    }
    Equation& operator=(Equation&& other) {
        if (*this)
            deallocate(*this);
        (EquationHandle&)* this = other;
        (EquationHandle&)other = {};
        return *this;
    }
    template<int_t N>
    Equation& operator=(const StaticEquation<N>&);
    ~Equation() {
        if (*this)
            deallocate(*this);
    }
    constexpr explicit operator bool() const { return id != 0; }
};
struct RangeEqn {
    Equation m_low, m_high;
    constexpr RangeEqn() = default;
    RangeEqn(RangeEqn&&) = default;
    RangeEqn(const StaticRangeEquation&);
    RangeEqn& operator=(RangeEqn&&) = default;
    RangeEqn& operator=(const StaticRangeEquation&);
    void updateShift(const Range& shift) const {
        low().updateShift(shift.low());
        high().updateShift(shift.high());
    }
    constexpr Equation& low() { return m_low; }
    constexpr const Equation& low() const { return m_low; }
    constexpr Equation& high() { return m_high; }
    constexpr const Equation& high() const { return m_high; }
};
struct RectEqn {
    RangeEqn m_x, m_y;
    constexpr RectEqn() = default;
    RectEqn(RectEqn&&) = default;
    RectEqn(const StaticRectEquation& eq);
    RectEqn& operator=(RectEqn&&) = default;
    RectEqn& operator=(const StaticRectEquation& eq);
    void updateShift(const Rect& shift) const {
        x().updateShift(shift.x());
        y().updateShift(shift.y());
    }
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

evt::Executor* coreExecutor();
evt::Executor* stylingExecutor();
evt::Executor* renderingExecutor();

}