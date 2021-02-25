#ifndef HEADER_UI_UISCENE_H
#define HEADER_UI_UISCENE_H

#include "UiComponent.hpp"
#include <map>
#include <vector>
#include <list>

//match Rect
#define UI_VAR_TOP 0
#define UI_VAR_BOTTOM 1
#define UI_VAR_LEFT 2
#define UI_VAR_RIGHT 3
#define UI_VAR_START 4
#define UI_VAR_INVALID 0xffffffff

typedef unsigned int ui_var_t;
typedef ui_var_t ui_svar_t;

#define UI_EQN_NOT_SIMPLE_BIT 0x80000000
#define UI_EQN_INVALID 0xffffffff

typedef unsigned int ui_eqn_t;

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

struct UiSolution{
    // = var * factor + offset
    ui_svar_t svar;
    float factor;
    float offset;
};

struct UiMatrixEntry{
    size_t eqn;
    size_t var;
    float factor;
};

class UiSceneBuilder; 

class UiScene{
    public:
        ScreenMetrics metrics;
        std::map<UiComponent*, ui_var_t> builders;
        std::vector<UiGeneralEquation> generalEquations;
        std::vector<UiSimpleEquation> simpleEquations;
        std::vector<std::vector<ui_eqn_t>> vars_to_eqns;
        ui_var_t varCount = UI_VAR_START;
        
        Rect parent = Rect::bltr(-1.0f, -1.0f, 1.0f, 1.0f);
        void setParent(Rect rect);
        UiSceneBuilder layout(UiComponent* comp);
        void build();
        void clear();
    private:
        UiSolution *solutions;
        std::vector<ui_eqn_t> markedEquations;
        std::vector<std::vector<ui_eqn_t>> svars_to_eqns;
        std::vector<ui_svar_t> solveVars;
        std::vector<UiMatrixEntry> entries;
        size_t eqnCount;
        std::vector<float> m_rhs;
        float *values;
        
        void exploreVar(ui_var_t var, ui_eqn_t origin);
        void exploreEq(ui_eqn_t eq, ui_var_t origin);
        void exploreVar2(ui_svar_t var, ui_eqn_t origin);
        void exploreEq2(ui_eqn_t eq, ui_svar_t origin);
};

class UiSceneBuilder{
    public:
        UiScene* scene;
        UiComponent* comp;
        ui_var_t base;
        
        ui_var_t top(){return base+UI_VAR_TOP;}
        ui_var_t bottom(){return base+UI_VAR_BOTTOM;}
        ui_var_t left(){return base+UI_VAR_LEFT;}
        ui_var_t right(){return base+UI_VAR_RIGHT;}
        
        UiSceneBuilder& above(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& below(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& toLeftOf(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& toRightOf(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        
        UiSceneBuilder& alignTop(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignBottom(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignLeft(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignRight(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignHorizontal(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignHorizontal(UiComponent* other, TypedSize paddingLeft, TypedSize paddingRight);
        UiSceneBuilder& alignVertical(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& alignVertical(UiComponent* other, TypedSize paddingTop, TypedSize paddingBottom);
        UiSceneBuilder& align(UiComponent* other=nullptr, TypedSize padding=TypedSize::zero);
        UiSceneBuilder& align(UiComponent* other, TypedSize paddingTop, TypedSize paddingLeft, TypedSize paddingBottom, TypedSize paddingRight);
        
        UiSceneBuilder& centerHorizontal(UiComponent* other=nullptr, float fraction=0.5f);
        UiSceneBuilder& centerVertical(UiComponent* other=nullptr, float fraction=0.5f);
        
        UiSceneBuilder& width(TypedSize width);
        UiSceneBuilder& height(TypedSize height);
        
        UiSceneBuilder& wrapContentWidth();
        UiSceneBuilder& wrapContentHeight();
        UiSceneBuilder& wrapContent();
        
    private:
        void addEquation(float f1, ui_var_t v1, float f2, ui_var_t v2, LazyTypedSizeXY rhs);
        void centerHorizontalInternal(ui_var_t otherLeft, ui_var_t otherRight, float fraction);
        void centerVerticalInternal(ui_var_t otherTop, ui_var_t otherBottom, float fraction);
};

#endif