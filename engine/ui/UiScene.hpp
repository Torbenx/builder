#ifndef HEADER_UI_UISCENE_H
#define HEADER_UI_UISCENE_H

#include "UiComponent.hpp"
#include <map>
#include <vector>
#include <list>

#define INVALID_UI_VAR_ID 0xffffff

#define PARENT_TOP_VAR_ID 0
#define PARENT_BOTTOM_VAR_ID 1
#define PARENT_LEFT_VAR_ID 2
#define PARENT_RIGHT_VAR_ID 3
#define VAR_ID_START 4

typedef unsigned int ui_var_t;

struct UiEquationTerm{
    float factor;
    ui_var_t var;
};

struct UiEquation{
    UiEquationTerm *terms;
    size_t termCount;
    LazyTypedSizeXY rhs;
};

struct SimpleUiEquation{
    UiEquationTerm term1;
    UiEquationTerm term2;
    LazyTypedSizeXY rhs;
};

struct UiVariable{
    std::vector<UiEquation*> appearsIn;
};

class UiSceneBuilder{
    public:
        void *scene;
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
        
        UiSceneBuilder& clear();
    private:
        void addEquation(float f1, ui_var_t v1, float f2, ui_var_t v2, LazyTypedSizeXY rhs);
};

class UiScene{
    public:
        ScreenMetrics metrics;
        std::map<UiComponent*, UiSceneBuilder> builders;
        std::list<UiEquation> equations;
        ui_var_t maxVarId = VAR_ID_START;
        
        Rect parent = Rect::bltr(-1.0f, -1.0f, 1.0f, 1.0f);
        void setParent(Rect rect);
        UiSceneBuilder& layout(UiComponent& comp);
        void build();
        void clear();
};

#endif