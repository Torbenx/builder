#include "Layout.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "util/log.hpp"

namespace ui {

Layout::Layout() {
    screenBox = newBox();
    screenEqn.left = makeEquation({ { screenBox.left().id, 1.0f } }, Size::fraction(0.0f).inX());
    screenEqn.right = makeEquation({ { screenBox.right().id, 1.0f } }, Size::fraction(1.0f).inX());
    screenEqn.bottom = makeEquation({ { screenBox.bottom().id, 1.0f } }, Size::fraction(0.0f).inY());
    screenEqn.top = makeEquation({ { screenBox.top().id, 1.0f } }, Size::fraction(1.0f).inY());
}

Layout::~Layout() {
    removeEqn(screenEqn);
    deleteBox(screenBox);
}

uint32_t Layout::allocateVariableIds(int_t count) {
    uint32_t id = nextVarId;
    nextVarId += count;
    varValues.resize(nextVarId);
    return id;
}

Box Layout::newBox() {
    return { allocateVariableIds(4) };
}
Variable Layout::newVar() {
    return { allocateVariableIds(1) };
}

Equation Layout::makeEquation(std::initializer_list<sparse::Entry> terms, TypedSize inhom) {
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

Equation Layout::align(Variable leftBottom, Variable rightTop, TypedSize inhom) {
    return makeEquation({ { leftBottom.id, -1.0f }, { rightTop.id, 1.0f } }, inhom);
}
BoxEqn Layout::align(Box inner, Box outer, Rect inhom) {
    return {
        align(outer.left(), inner.left(), inhom.left),
        align(inner.right(), outer.right(), inhom.right),
        align(outer.bottom(), inner.bottom(), inhom.bottom),
        align(inner.top(), outer.top(), inhom.top),
    };
}

void Layout::updateInhom(Equation eq, TypedSize inhom) {
    inhomTyped[eq.id] = inhom;
}
void Layout::updateInhom(BoxEqn eq, Rect inhom) {
    updateInhom(eq.left, inhom.left);
    updateInhom(eq.right, inhom.right);
    updateInhom(eq.bottom, inhom.bottom);
    updateInhom(eq.top, inhom.top);
}

void Layout::removeEqn(Equation eqn) {
    matrix[eqn.id].resizeTo(0);
    eqnIdAlloc.freeId(eqn.id);
}
void Layout::removeEqn(BoxEqn eqn) {
    removeEqn(eqn.left);
    removeEqn(eqn.right);
    removeEqn(eqn.bottom);
    removeEqn(eqn.top);
}
void Layout::deleteBox(Box) {
    VERIFY_NOT_REACHED(Ui);
}
void Layout::deleteVariable(Variable) {
    VERIFY_NOT_REACHED(Ui);
}

void Layout::resolveTypedSizes() {
    int_t inhomSize = inhomTyped.size();
    for (int_t i = 0; i < inhomSize; i++)
        inhomValues[i] = metrics().pixels(inhomTyped[i]);
}

void Layout::doLayout(ScreenMetrics newMetrics) {
    m_metrics = newMetrics;
    VDUMP(metrics().width());
    VDUMP(metrics().height());
    resolveTypedSizes();
    sparse::LUDecomposition lu;
    lu.u = matrix.copy();
    int_t varCount = nextVarId;
    lu.decompose(varCount);
    lu.inverseTranspose(inhomValues.data(), varValues.data());
}

}