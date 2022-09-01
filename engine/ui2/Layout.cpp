#include "Layout.hpp"
#include "util/log.hpp"

namespace ui {

Layout::~Layout() { }

Box Layout::newBox() {
    uint32_t id = varIdAlloc.allocateId() * 4;
    return { id };
}
Variable Layout::newVar() {
    return { newBox().baseId };
}

Equation Layout::align(Variable leftBottom, Variable rightTop) {
    Equation eq = { eqnIdAlloc.allocateId() };
    if (inhomTyped.size() <= eq.id)
        inhomTyped.resize(eq.id + 1);
    if (inhomValues.size() <= eq.id)
        inhomValues.resize(eq.id + 1);
    if (eq.id == mat.size())
        mat.appendColumn(sparse::allocateEmpty(4));

    mat[eq.id].appendEntry({ leftBottom.id, 1.0f });
    mat[eq.id].appendEntry({ rightTop.id, -1.0f });
    mat[eq.id].sort();

    return eq;
}
BoxEqn Layout::align(Box inner, Box outer) {
    return {
        align(outer.left(), inner.left()),
        align(inner.right(), outer.right()),
        align(outer.bottom(), inner.bottom()),
        align(inner.top(), outer.top()),
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
    mat[eqn.id].resizeTo(0);
    eqnIdAlloc.freeId(eqn.id);
}
void Layout::removeEqn(BoxEqn eqn) {
    removeEqn(eqn.left);
    removeEqn(eqn.right);
    removeEqn(eqn.bottom);
    removeEqn(eqn.top);
}
void Layout::deleteBox(Box box) {
    VERIFY(Ui, box.baseId % 4 == 0);
    varIdAlloc.freeId(box.baseId / 4);
}
void Layout::deleteVariable(Variable var) {
    deleteBox({ var.id });
}

}