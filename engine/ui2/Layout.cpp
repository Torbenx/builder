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

void Layout::doLayout(IntVec windowSize, float ptScale) {
    VDUMP(windowSize.x());
    VDUMP(windowSize.y());
    framebuffer = Texture(windowSize);

    updateInhom(screenEqn, { { 0_px, windowSize.x() * 1_px }, { 0_px, windowSize.y() * 1_px } });
    Size::asPixels(inhomTyped.data(), inhomValues.data(), inhomTyped.size(), ptScale, 1.0f);

    sparse::LUDecomposition lu;
    lu.u = matrix.copy();
    int_t varCount = nextVarId;
    lu.decompose(varCount);
    std::vector<float> newVarValues(varCount);
    lu.inverseTranspose(inhomValues.data(), newVarValues.data());

    std::vector<int_t> textureUpdates;
    for (int_t i = 1; i < (int_t)renders.size(); i++) {
        auto& render = renders[i];
        IntRect oldRect = intRect(render.rect, varValues.data());
        IntRect newRect = intRect(render.rect, newVarValues.data());
        if (newRect.dimensions() == oldRect.dimensions())
            continue;
        if (render.allocPosition != IntVec {}) {
            deallocateTexture({ BottomLeft(render.allocPosition), Dimensions(oldRect.dimensions()) });
            render.allocPosition = {};
        }
        textureUpdates.push_back(i);
    }

    varValues = std::move(newVarValues);

    for (int_t updateId : textureUpdates) {
        auto& render = renders[updateId];
        IntRect rect = intRect(render.rect);
        render.allocPosition = allocateTexture(rect.dimensions());
        render.handle.update(atlas.view() + render.allocPosition, rect.dimensions());
    }
}

void Layout::doDraw() {
    uint32_t background_color = 0xff'ff'ff'ff;
    for (int_t j = 0; j < framebuffer.dimensions.y(); j++) {
        for (int_t i = 0; i < framebuffer.dimensions.x(); i++) {
            framebuffer.view()[j][i] = background_color >> 8;
        }
    }
    for (int_t k = 1; k < (int_t)renders.size(); k++) {
        auto& render = renders[k];
        IntRect rect = intRect(render.rect);
        for (int_t j = 0; j < rect.height(); j++) {
            for (int_t i = 0; i < rect.width(); i++) {
                (framebuffer.view() + rect.bottomLeft())[j][i] = (atlas.view() + render.allocPosition)[j][i] >> 8;
            }
        }
    }
}

void Layout::draw(RendererHandle handle, RectVar rect) {
    if (handle->id == 0) {
        handle->id = renders.size();
        renders.push_back({ handle, rect });
    } else {
        renders[handle->id].rect = rect;
    }
}

void Layout::cancelDraw(RendererBase& ref) {
    RendererBase* base = &ref;
    VERIFY(Ui, base->id != 0);

    Render back = renders.back();
    renders.pop_back();
    back.handle->id = base->id;
    base->id = 0;

    renders[back.handle->id] = back;
}

IntVec Layout::allocateTexture(IntVec dimensions) {
    const int atlasWidth = 16384;
    VERIFY(Ui, dimensions.x() <= atlasWidth);
    IntVec allocPos { atlasPadding, nextAtlasAllocationHeight + atlasPadding };
    nextAtlasAllocationHeight += atlasPadding + dimensions.y();
    int_t neededHeight = nextAtlasAllocationHeight + atlasPadding;
    if (neededHeight >= atlas.dimensions.y()) {
        Texture newAtlas { { atlasWidth, (int)neededHeight } };
        std::copy_n(atlas.data, atlas.dimensions.x() * atlas.dimensions.y(), newAtlas.data);
        atlas = std::move(newAtlas);
    }
    return allocPos;
}

void Layout::deallocateTexture(IntRect) {
    VERIFY_NOT_REACHED(Ui);
}

}