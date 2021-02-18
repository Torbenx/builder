#include "UiScene.hpp"
#include "../util/log.hpp"

#define GET_UI_BUILDER(comp) ((UiScene*)scene)->layout(comp)

UiSceneBuilder& UiScene::layout(UiComponent& comp){
    auto builderIt = builders.find(&comp);
    if(builderIt == builders.end()){
        UiSceneBuilder& builder = builders[&comp];
        builder.scene = this;
        builder.comp = &comp;
        builder.topV = maxVarId++;
        builder.bottomV = maxVarId++;
        builder.leftV = maxVarId++;
        builder.rightV = maxVarId++;
        return builder;
    }
    return builderIt->second;
}

void UiScene::setParent(Rect rect){
    this->parent = rect;
}

void UiScene::build(){
    float solved[maxVarId];
    for(size_t i=0;i<maxVarId;i++){
        solved[i] = NAN;
    }
    solved[PARENT_TOP_VAR_ID] = parent.top;
    solved[PARENT_BOTTOM_VAR_ID] = parent.bottom;
    solved[PARENT_LEFT_VAR_ID] = parent.left;
    solved[PARENT_RIGHT_VAR_ID] = parent.right;
    bool run = true;
    while(run){
        run = false;
        for(UiEquation eq : equations){
            float rhs = eq.rhs.get().toGL(metrics);
            ui_var_t solveFor = INVALID_UI_VAR_ID;
            float solveFor_factor;
            for(size_t i=0; i<eq.termCount; i++){
                UiEquationTerm term = eq.terms[i];
                float s = solved[term.var];
                if(std::isnan(s)){
                    if(solveFor == INVALID_UI_VAR_ID){
                        solveFor = term.var;
                        solveFor_factor = term.factor;
                    }else{
                        solveFor = INVALID_UI_VAR_ID;
                        break;
                    }
                }else{
                    rhs -= term.factor * s;
                }
            }
            if(solveFor == INVALID_UI_VAR_ID){
                continue;
            }
            /*if(eq.termCount == 2){
                LOGD("(%f)*[%d] + (%f)*[%d] = %f", eq.terms[0].factor, eq.terms[0].var, eq.terms[1].factor, eq.terms[1].var, eq.rhs.get().toGL(metrics));
            }
            LOGD("  ==> [%d] = %f / %f\n", solveFor, rhs, solveFor_factor);*/
            solved[solveFor] = rhs / solveFor_factor;
            run = true;
        }
    }
    /*for(size_t i=0;i<maxVarId;i++){
        LOGD("%lld = %f", i, solved[i]);
    }*/
    for(auto it=builders.begin();it!=builders.end();it++){
        it->first->metrics = metrics;
        it->first->rect = Rect::bltr(
            solved[it->second.bottomV],
            solved[it->second.leftV],
            solved[it->second.topV],
            solved[it->second.rightV]
        );
        it->first->updateRect();
    }
}

void UiScene::clear(){
    for(UiEquation eq : equations){
        free(eq.terms);
    }
    equations.clear();
    builders.clear();
    maxVarId = VAR_ID_START;
}


void UiSceneBuilder::addEquation(float f1, ui_var_t v1, float f2, ui_var_t v2, LazyTypedSizeXY rhs){
    UiEquation eq;
    eq.termCount = 2;
    eq.terms = (UiEquationTerm*) malloc(2 * sizeof(UiEquationTerm));
    eq.terms[0].factor = f1;
    eq.terms[0].var = v1;
    eq.terms[1].factor = f2;
    eq.terms[1].var = v2;
    eq.rhs = rhs;
    ((UiScene*)scene)->equations.push_back(eq);
}

UiSceneBuilder& UiSceneBuilder::above(UiComponent& other, TypedSize padding){
    UiSceneBuilder::addEquation(1, bottomV, -1, GET_UI_BUILDER(other).topV, padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::below(UiComponent& other, TypedSize padding){
    UiSceneBuilder::addEquation(-1, topV, 1, GET_UI_BUILDER(other).bottomV, padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::toLeftOf(UiComponent& other, TypedSize padding){
    UiSceneBuilder::addEquation(-1, rightV, 1, GET_UI_BUILDER(other).leftV, padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::toRightOf(UiComponent& other, TypedSize padding){
    UiSceneBuilder::addEquation(1, leftV, -1, GET_UI_BUILDER(other).rightV, padding.asXSize().lazy());
    return *this;
}


UiSceneBuilder& UiSceneBuilder::alignTop(UiComponent& other, TypedSize padding){
    UiSceneBuilder::addEquation(-1, topV, 1, GET_UI_BUILDER(other).topV, padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignBottom(UiComponent& other, TypedSize padding){
    UiSceneBuilder::addEquation(1, bottomV, -1, GET_UI_BUILDER(other).bottomV, padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignLeft(UiComponent& other, TypedSize padding){
    UiSceneBuilder::addEquation(1, leftV, -1, GET_UI_BUILDER(other).leftV, padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignRight(UiComponent& other, TypedSize padding){
    UiSceneBuilder::addEquation(-1, rightV, 1, GET_UI_BUILDER(other).rightV, padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignHorizontal(UiComponent& other, TypedSize padding){
    UiSceneBuilder& ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(1, leftV, -1, ob.leftV, padding.asXSize().lazy());
    UiSceneBuilder::addEquation(-1, rightV, 1, ob.rightV, padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignHorizontal(UiComponent& other, TypedSize paddingLeft, TypedSize paddingRight){
    UiSceneBuilder& ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(1, leftV, -1, ob.leftV, paddingLeft.asXSize().lazy());
    UiSceneBuilder::addEquation(-1, rightV, 1, ob.rightV, paddingRight.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignVertical(UiComponent& other, TypedSize padding){
    UiSceneBuilder& ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(-1, topV, 1, ob.topV, padding.asYSize().lazy());
    UiSceneBuilder::addEquation(1, bottomV, -1, ob.bottomV, padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignVertical(UiComponent& other, TypedSize paddingTop, TypedSize paddingBottom){
    UiSceneBuilder& ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(-1, topV, 1, ob.topV, paddingTop.asYSize().lazy());
    UiSceneBuilder::addEquation(1, bottomV, -1, ob.bottomV, paddingBottom.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::align(UiComponent& other, TypedSize padding){
    UiSceneBuilder& ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(-1, topV, 1, ob.topV, padding.asYSize().lazy());
    UiSceneBuilder::addEquation(1, bottomV, -1, ob.bottomV, padding.asYSize().lazy());
    UiSceneBuilder::addEquation(1, leftV, -1, ob.leftV, padding.asXSize().lazy());
    UiSceneBuilder::addEquation(-1, rightV, 1, ob.rightV, padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::align(UiComponent& other, TypedSize paddingTop, TypedSize paddingLeft, TypedSize paddingBottom, TypedSize paddingRight){
    UiSceneBuilder& ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(-1, topV, 1, ob.topV, paddingTop.asYSize().lazy());
    UiSceneBuilder::addEquation(1, bottomV, -1, ob.bottomV, paddingBottom.asYSize().lazy());
    UiSceneBuilder::addEquation(1, leftV, -1, ob.leftV, paddingLeft.asXSize().lazy());
    UiSceneBuilder::addEquation(-1, rightV, 1, ob.rightV, paddingRight.asXSize().lazy());
    return *this;
}


UiSceneBuilder& UiSceneBuilder::alignParentTop(TypedSize padding){
    UiSceneBuilder::addEquation(-1, topV, 1, PARENT_TOP_VAR_ID, padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignParentBottom(TypedSize padding){
    UiSceneBuilder::addEquation(1, bottomV, -1, PARENT_BOTTOM_VAR_ID, padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignParentLeft(TypedSize padding){
    UiSceneBuilder::addEquation(1, leftV, -1, PARENT_LEFT_VAR_ID, padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignParentRight(TypedSize padding){
    UiSceneBuilder::addEquation(-1, rightV, 1, PARENT_RIGHT_VAR_ID, padding.asXSize().lazy());
    return *this;
}


UiSceneBuilder& UiSceneBuilder::centerHorizontal(float fraction){
    
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerVertical(float fraction){
    
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerHorizontalInside(UiComponent& other, float fraction){
    
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerVerticalInside(UiComponent& other, float fraction){
    
    return *this;
}


UiSceneBuilder& UiSceneBuilder::width(TypedSize width){
    UiSceneBuilder::addEquation(1, rightV, -1, leftV, width.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::height(TypedSize height){
    UiSceneBuilder::addEquation(1, topV, -1, bottomV, height.asYSize().lazy());
    return *this;
}


UiSceneBuilder& UiSceneBuilder::wrapContentWidth(){
    UiSceneBuilder::addEquation(1, rightV, -1, leftV, comp->getContentWidthLazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::wrapContentHeight(){
    UiSceneBuilder::addEquation(1, topV, -1, bottomV, comp->getContentHeightLazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::wrapContent(){
    wrapContentWidth();
    wrapContentHeight();
    return *this;
}

UiSceneBuilder& UiSceneBuilder::clear(){
    
    return *this;
}
