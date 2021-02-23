#ifndef HEADER_UI_UISCENE_H
#define HEADER_UI_UISCENE_H

#include "UiComponent.hpp"
#include <map>
#include <vector>
#include <list>

#define UI_VAR_PARENT_TOP 0
#define UI_VAR_PARENT_BOTTOM 1
#define UI_VAR_PARENT_LEFT 2
#define UI_VAR_PARENT_RIGHT 3
#define UI_VAR_START 4
#define UI_VAR_INVALID 0xffffffff

typedef unsigned int ui_var_t;

#define UI_EQN_NOT_SIMPLE_BIT 0x80000000
#define UI_EQN_INVALID 0xffffffff

typedef unsigned int ui_eqn_t;

struct UiSolution{
    // = var * factor + offset
    ui_var_t var;
    float factor;
    float offset;
};

struct UiEquationTerm{
    float factor;
    ui_var_t var;
};

struct UiGeneralEquation{
    UiEquationTerm *terms;
    size_t termCount;
    LazyTypedSizeXY rhs;
};

struct UiSimpleEquation{
    UiEquationTerm term1;
    UiEquationTerm term2;
    LazyTypedSizeXY rhs;
};

class UiSceneBuilder; 

class UiScene{
    public:
        ScreenMetrics metrics;
        std::map<UiComponent*, UiSceneBuilder> builders;
        std::vector<UiGeneralEquation> generalEquations;
        std::vector<UiSimpleEquation> simpleEquations;
        std::vector<std::vector<ui_eqn_t>> vars_to_eqns;
        ui_var_t varCount = UI_VAR_START;
        
        Rect parent = Rect::bltr(-1.0f, -1.0f, 1.0f, 1.0f);
        void setParent(Rect rect);
        UiSceneBuilder& layout(UiComponent& comp);
        void build();
        void clear();
};

class UiSceneBuilder{
    public:
        UiScene* scene;
        UiComponent* comp;
        ui_var_t topV, bottomV, leftV, rightV;
        
        UiSceneBuilder& above(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& below(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& toLeftOf(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& toRightOf(UiComponent& other, TypedSize padding=TypedSize::zero);
        
        UiSceneBuilder& alignTop(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignBottom(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignLeft(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignRight(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignHorizontal(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignHorizontal(UiComponent& other, TypedSize paddingLeft, TypedSize paddingRight);
        UiSceneBuilder& alignVertical(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignVertical(UiComponent& other, TypedSize paddingTop, TypedSize paddingBottom);
        UiSceneBuilder& align(UiComponent& other, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& align(UiComponent& other, TypedSize paddingTop, TypedSize paddingLeft, TypedSize paddingBottom, TypedSize paddingRight);
        
        UiSceneBuilder& alignParentTop(TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignParentBottom(TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignParentLeft(TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignParentRight(TypedSize padding=TypedSize::zero);
        
        UiSceneBuilder& centerHorizontal(float fraction=0.5f);
        UiSceneBuilder& centerVertical(float fraction=0.5f);
        UiSceneBuilder& centerHorizontalInside(UiComponent& other, float fraction=0.5f);
        UiSceneBuilder& centerVerticalInside(UiComponent& other, float fraction=0.5f);
        
        UiSceneBuilder& width(TypedSize width);
        UiSceneBuilder& height(TypedSize height);
        
        UiSceneBuilder& wrapContentWidth();
        UiSceneBuilder& wrapContentHeight();
        UiSceneBuilder& wrapContent();
        
    private:
        void addEquation(float f1, ui_var_t v1, float f2, ui_var_t v2, LazyTypedSizeXY rhs);
};

#endif