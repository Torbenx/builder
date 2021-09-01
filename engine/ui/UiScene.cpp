#include "ui/UiScene.hpp"
#include "util/log.hpp"
#include "util/time.hpp"
#include <algorithm>

#define GET_UI_BUILDER(comp) scene->layout(comp)

UiScene::UiScene() {
    solver.fixed = new float[4];
    solver.fixedCount = 4;
    solver.fixed_to_eqns.resize(4);
}

UiScene::~UiScene() {
    VERIFY(UI, components.empty());
    delete[] solver.fixed;
}

UiSceneBuilder UiScene::layout(UiComponent* comp) {
    if (comp) {
        auto where = std::find(components.begin(), components.end(), comp);
        if (where == components.end()) {
            comp->base = solver.varCount;
            solver.varCount += 4;
            solver.vars_to_eqns.resize(solver.varCount);
            components.push_back(comp);
        }
    }
    UiSceneBuilder builder;
    builder.scene = this;
    builder.comp = comp;
    return builder;
}

void UiScene::setParent(Rect rect) {
    this->parent = rect;
}

void UiScene::build(GAllocator& allocator) {

    solver.fixed[UI_VAR_TOP] = parent.top;
    solver.fixed[UI_VAR_BOTTOM] = parent.bottom;
    solver.fixed[UI_VAR_LEFT] = parent.left;
    solver.fixed[UI_VAR_RIGHT] = parent.right;

    solver.metrics = metrics;
    solver.solve();

    for (auto* comp : components) {
        Rect rect = Rect::bltr(
            solver.getSolution(comp->base + UI_VAR_BOTTOM),
            solver.getSolution(comp->base + UI_VAR_LEFT),
            solver.getSolution(comp->base + UI_VAR_TOP),
            solver.getSolution(comp->base + UI_VAR_RIGHT));
        comp->rect = rect;
        comp->updateRect(metrics, allocator);
    }

    solver.freeSolution();
}

void UiScene::clear(GAllocator& allocator) {
    for (auto* comp : components) {
        comp->detach(allocator);
    }
    solver.clear();
    components.clear();
}

UiSceneBuilder& UiSceneBuilder::above(UiComponent* other, TypedSize padding) {
    scene->solver.addEquation(1, bottom(), -1, GET_UI_BUILDER(other).top(), padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::below(UiComponent* other, TypedSize padding) {
    scene->solver.addEquation(-1, top(), 1, GET_UI_BUILDER(other).bottom(), padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::toLeftOf(UiComponent* other, TypedSize padding) {
    scene->solver.addEquation(-1, right(), 1, GET_UI_BUILDER(other).left(), padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::toRightOf(UiComponent* other, TypedSize padding) {
    scene->solver.addEquation(1, left(), -1, GET_UI_BUILDER(other).right(), padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignTop(UiComponent* other, TypedSize padding) {
    scene->solver.addEquation(-1, top(), 1, GET_UI_BUILDER(other).top(), padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignBottom(UiComponent* other, TypedSize padding) {
    scene->solver.addEquation(1, bottom(), -1, GET_UI_BUILDER(other).bottom(), padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignLeft(UiComponent* other, TypedSize padding) {
    scene->solver.addEquation(1, left(), -1, GET_UI_BUILDER(other).left(), padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignRight(UiComponent* other, TypedSize padding) {
    scene->solver.addEquation(-1, right(), 1, GET_UI_BUILDER(other).right(), padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignHorizontal(UiComponent* other, TypedSize padding) {
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    scene->solver.addEquation(1, left(), -1, ob.left(), padding.asXSize().lazy());
    scene->solver.addEquation(-1, right(), 1, ob.right(), padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignHorizontal(UiComponent* other, TypedSize paddingLeft, TypedSize paddingRight) {
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    scene->solver.addEquation(1, left(), -1, ob.left(), paddingLeft.asXSize().lazy());
    scene->solver.addEquation(-1, right(), 1, ob.right(), paddingRight.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignVertical(UiComponent* other, TypedSize padding) {
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    scene->solver.addEquation(-1, top(), 1, ob.top(), padding.asYSize().lazy());
    scene->solver.addEquation(1, bottom(), -1, ob.bottom(), padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignVertical(UiComponent* other, TypedSize paddingTop, TypedSize paddingBottom) {
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    scene->solver.addEquation(-1, top(), 1, ob.top(), paddingTop.asYSize().lazy());
    scene->solver.addEquation(1, bottom(), -1, ob.bottom(), paddingBottom.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::align(UiComponent* other, TypedSize padding) {
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    scene->solver.addEquation(-1, top(), 1, ob.top(), padding.asYSize().lazy());
    scene->solver.addEquation(1, bottom(), -1, ob.bottom(), padding.asYSize().lazy());
    scene->solver.addEquation(1, left(), -1, ob.left(), padding.asXSize().lazy());
    scene->solver.addEquation(-1, right(), 1, ob.right(), padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::align(UiComponent* other, TypedSize paddingTop, TypedSize paddingLeft, TypedSize paddingBottom, TypedSize paddingRight) {
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    scene->solver.addEquation(-1, top(), 1, ob.top(), paddingTop.asYSize().lazy());
    scene->solver.addEquation(1, bottom(), -1, ob.bottom(), paddingBottom.asYSize().lazy());
    scene->solver.addEquation(1, left(), -1, ob.left(), paddingLeft.asXSize().lazy());
    scene->solver.addEquation(-1, right(), 1, ob.right(), paddingRight.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerHorizontal(UiComponent* other, float fraction) {
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    UiGeneralEquation eq;
    eq.termCount = 4;
    eq.terms = new UiEquationTerm[eq.termCount];
    eq.terms[0].factor = -(1.0f - fraction);
    eq.terms[0].var = ob.left();
    eq.terms[1].factor = -fraction;
    eq.terms[1].var = ob.right();
    eq.terms[2].factor = 1.0f - fraction;
    eq.terms[2].var = left();
    eq.terms[3].factor = fraction;
    eq.terms[3].var = right();
    eq.rhs = TypedSize::zero.asXSize().lazy();
    ui_eqn_t eqId = scene->solver.generalEquations.size() | UI_EQN_NOT_SIMPLE_BIT;
    scene->solver.generalEquations.push_back(eq);
    scene->solver.addEq(ob.left(), eqId);
    scene->solver.addEq(ob.right(), eqId);
    scene->solver.addEq(left(), eqId);
    scene->solver.addEq(right(), eqId);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerVertical(UiComponent* other, float fraction) {
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    UiGeneralEquation eq;
    eq.termCount = 4;
    eq.terms = new UiEquationTerm[eq.termCount];
    eq.terms[0].factor = -fraction;
    eq.terms[0].var = ob.top();
    eq.terms[1].factor = -(1.0f - fraction);
    eq.terms[1].var = ob.bottom();
    eq.terms[2].factor = fraction;
    eq.terms[2].var = top();
    eq.terms[3].factor = 1.0f - fraction;
    eq.terms[3].var = bottom();
    eq.rhs = TypedSize::zero.asYSize().lazy();
    ui_eqn_t eqId = scene->solver.generalEquations.size() | UI_EQN_NOT_SIMPLE_BIT;
    scene->solver.generalEquations.push_back(eq);
    scene->solver.addEq(ob.top(), eqId);
    scene->solver.addEq(ob.bottom(), eqId);
    scene->solver.addEq(top(), eqId);
    scene->solver.addEq(bottom(), eqId);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::width(TypedSize width) {
    scene->solver.addEquation(1, right(), -1, left(), width.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::height(TypedSize height) {
    scene->solver.addEquation(1, top(), -1, bottom(), height.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::wrapContentWidth() {
    scene->solver.addEquation(1, right(), -1, left(), comp->getContentWidthLazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::wrapContentHeight() {
    scene->solver.addEquation(1, top(), -1, bottom(), comp->getContentHeightLazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::wrapContent() {
    wrapContentWidth();
    wrapContentHeight();
    return *this;
}
