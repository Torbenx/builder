#include "Layout.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "util/log.hpp"

namespace ui {

Layout::Layout() {
    using namespace ui::literals;
    screenBox = newRect();
    screenEqn.left() = makeEquation({ { screenBox.left().id, 1.0f } });
    screenEqn.right() = makeEquation({ { screenBox.right().id, 1.0f } });
    screenEqn.bottom() = makeEquation({ { screenBox.bottom().id, 1.0f } });
    screenEqn.top() = makeEquation({ { screenBox.top().id, 1.0f } });
}

Layout::~Layout() {
    removeEqn(screenEqn);
    deleteVariable(screenBox);
}

uint32_t Layout::allocateVariableIds(int_t count) {
    uint32_t id = nextVarId;
    nextVarId += count;
    varValues.resize(nextVarId);
    return id;
}

Variable Layout::newVar() { return { allocateVariableIds(1) }; }
RangeVar Layout::newRange() { return { allocateVariableIds(2) }; }
RectVar Layout::newRect() { return { allocateVariableIds(4) }; }

Equation Layout::makeEquation(std::initializer_list<sparse::Entry> terms, Size inhom) {
    Equation eq = { eqnIdAlloc.allocateId() };
    if (inhomTyped.size() <= eq.id)
        inhomTyped.resize(eq.id + 1);
    if (inhomValues.size() <= eq.id)
        inhomValues.resize(eq.id + 1);
    if (eq.id == matrix.size())
        matrix.appendColumn({});

    sparse::Vector vec = sparse::allocate(terms.size());
    std::copy(terms.begin(), terms.end(), vec.begin());
    vec.resizeTo(terms.size());
    vec.sort();
    matrix[eq.id] = std::move(vec);

    updateInhom(eq, inhom);

    return eq;
}

Equation Layout::align(Variable leftBottom, Variable rightTop, Size inhom) {
    return makeEquation({ { leftBottom.id, -1.0f }, { rightTop.id, 1.0f } }, inhom);
}
RangeEqn Layout::align(RangeVar inner, RangeVar outer, Range inhom) {
    return {
        align(outer.low(), inner.low(), inhom.low()),
        align(inner.high(), outer.high(), inhom.low())
    };
}
RectEqn Layout::align(RectVar inner, RectVar outer, Rect inhom) {
    return {
        align(inner.x(), outer.x(), inhom.x()),
        align(inner.y(), outer.y(), inhom.y()),
    };
}

void Layout::updateInhom(Equation eq, Size inhom) {
    inhomTyped[eq.id] = inhom;
}
void Layout::updateInhom(RangeEqn eq, Range inhom) {
    updateInhom(eq.low(), inhom.low());
    updateInhom(eq.high(), inhom.high());
}
void Layout::updateInhom(RectEqn eq, Rect inhom) {
    updateInhom(eq.x(), inhom.x());
    updateInhom(eq.y(), inhom.y());
}

void Layout::removeEqn(Equation eqn) {
    matrix[eqn.id].resizeTo(0);
    eqnIdAlloc.freeId(eqn.id);
}
void Layout::removeEqn(RangeEqn eqn) {
    removeEqn(eqn.low());
    removeEqn(eqn.high());
}
void Layout::removeEqn(RectEqn eqn) {
    removeEqn(eqn.x());
    removeEqn(eqn.y());
}
void Layout::deleteVariable(Variable) {
    VERIFY_NOT_REACHED(Ui);
}
void Layout::deleteVariable(RangeVar) {
    VERIFY_NOT_REACHED(Ui);
}
void Layout::deleteVariable(RectVar) {
    VERIFY_NOT_REACHED(Ui);
}

void Layout::doLayout(int_t width, int_t height, float ptScale) {
    VDUMP(width);
    VDUMP(height);

    updateInhom(screenEqn, { { 0_px, width * 1_px }, { 0_px, height * 1_px } });
    Size::asPixels(inhomTyped.data(), inhomValues.data(), inhomTyped.size(), ptScale, 1.0f);

    sparse::LUDecomposition lu;
    lu.u = matrix.copy();
    int_t varCount = nextVarId;
    lu.decompose(varCount);
    lu.inverseTranspose(inhomValues.data(), varValues.data());
}

}