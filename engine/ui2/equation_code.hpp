#pragma once

#include "containers/Array.hpp"
#include "handles.hpp"

namespace ui {

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

constexpr VarHandle::operator ui::StaticSum<1LL>() const { return { { { { id, 1.0f } } } }; }

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

struct ShiftedRangeVar {
    RangeVarHandle var;
    Range shift = {};
    bool negated = false;
};
struct StaticRangeEquation {
    // plus - minus = shift
    RangeVarHandle plus, minus;
    Range shift = {};
};
struct ShiftedRectVar {
    RectVarHandle var;
    Rect shift = {};
    bool negated = false;
};
struct StaticRectEquation {
    // plus - minus = shift
    RectVarHandle plus, minus;
    Rect shift = {};
    constexpr StaticRangeEquation x() const {
        return { plus.x(), minus.x(), shift.x() };
    }
    constexpr StaticRangeEquation y() const {
        return { plus.y(), minus.y(), shift.y() };
    }
};
namespace code {
    constexpr ShiftedRangeVar extendBy(const RangeVarHandle& var, const Range& shift) {
        return { var, shift, false };
    }
    constexpr ShiftedRangeVar shrinkBy(const RangeVarHandle& var, const Range& shift) {
        return { var, shift, true };
    }
    constexpr StaticRangeEquation operator==(const RangeVarHandle& left, const RangeVarHandle& right) {
        return { left, right };
    }
    constexpr StaticRangeEquation operator==(const ShiftedRangeVar& left, const RangeVarHandle& right) {
        return { left.negated ? left.var : right, left.negated ? right : left.var, left.shift };
    }
    constexpr StaticRangeEquation operator==(const RangeVarHandle& left, const ShiftedRangeVar& right) {
        return right == left;
    }

    constexpr ShiftedRectVar extendBy(const RectVarHandle& var, const Rect& shift) {
        return { var, shift, false };
    }
    constexpr ShiftedRectVar shrinkBy(const RectVarHandle& var, const Rect& shift) {
        return { var, shift, true };
    }
    constexpr StaticRectEquation operator==(const RectVarHandle& left, const RectVarHandle& right) {
        return { left, right };
    }
    constexpr StaticRectEquation operator==(const ShiftedRectVar& left, const RectVarHandle& right) {
        return { left.negated ? left.var : right, left.negated ? right : left.var, left.shift };
    }
    constexpr StaticRectEquation operator==(const RectVarHandle& left, const ShiftedRectVar& right) {
        return right == left;
    }
}

template<int_t N>
inline Equation& Equation::operator=(const StaticEquation<N>& eq) {
    if (!*this)
        (EquationHandle&)* this = allocate();
    shiftFactor = eq.shiftFactor;
    updateEquation(eq.equalsShift.terms);
    updateShift(eq.shift);
    return *this;
}

namespace code {
    constexpr auto makeLowEquation(const StaticRangeEquation& eq) {
        return eq.plus.low() == eq.minus.low() - shift(eq.shift.low());
    }
    constexpr auto makeHighEquation(const StaticRangeEquation& eq) {
        return eq.plus.high() == eq.minus.high() + shift(eq.shift.high());
    }
}
inline RangeEqn::RangeEqn(const StaticRangeEquation& eq)
    : m_low(code::makeLowEquation(eq)), m_high(code::makeHighEquation(eq)) { }

inline RangeEqn& RangeEqn::operator=(const StaticRangeEquation& eq) {
    low() = code::makeLowEquation(eq);
    high() = code::makeHighEquation(eq);
    return *this;
}

inline RectEqn::RectEqn(const StaticRectEquation& eq)
    : m_x(eq.x()), m_y(eq.y()) { }
inline RectEqn& RectEqn::operator=(const StaticRectEquation& eq) {
    x() = eq.x();
    y() = eq.y();
    return *this;
}

constexpr StaticSum<2> RangeVarHandle::extend() const {
    using namespace code;
    return high() - low();
}
constexpr StaticSum<2> RectVarHandle::width() const { return x().extend(); }
constexpr StaticSum<2> RectVarHandle::height() const { return y().extend(); }

}