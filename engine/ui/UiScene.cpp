#include "UiScene.hpp"
#include "../util/log.hpp"
#include "../util/time.hpp"

#define GET_UI_BUILDER(comp) scene->layout(comp)

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
        
        vars_to_eqns.resize(maxVarId);
        
        return builder;
    }
    return builderIt->second;
}

void UiScene::setParent(Rect rect){
    this->parent = rect;
}

void UiScene::build(){
    //uint64_t build_start = getNanos();
    float solved[maxVarId];
    for(size_t i=0;i<maxVarId;i++){
        solved[i] = NAN;
    }
    solved[PARENT_TOP_VAR_ID] = parent.top;
    solved[PARENT_BOTTOM_VAR_ID] = parent.bottom;
    solved[PARENT_LEFT_VAR_ID] = parent.left;
    solved[PARENT_RIGHT_VAR_ID] = parent.right;
    
    std::list<ui_eqn_t> marked;
    auto markEquations = [&](ui_var_t var, ui_eqn_t ignore1=INVALID_UI_EQN_ID, ui_eqn_t ignore2=INVALID_UI_EQN_ID){
        std::vector<ui_eqn_t>& eqns = vars_to_eqns[var];
        for(ui_eqn_t eq : eqns){
            if(eq != ignore1 && eq != ignore2) marked.push_back(eq);
        }
    };
    
    markEquations(PARENT_TOP_VAR_ID);
    markEquations(PARENT_BOTTOM_VAR_ID);
    markEquations(PARENT_LEFT_VAR_ID);
    markEquations(PARENT_RIGHT_VAR_ID);
    
    while(!marked.empty()){
        ui_eqn_t eqId = marked.front();
        marked.pop_front();
        if(eqId & EQN_ID_NOT_SIMPLE_BIT){
            UiEquation& eq = equations[eqId & ~EQN_ID_NOT_SIMPLE_BIT];
            float rhs = eq.rhs.get().toGL(metrics);
            std::vector<UiEquationTerm> terms;
            for(size_t j=0;j<eq.termCount;j++){
                float s = solved[eq.terms[j].var];
                if(std::isnan(s)){
                    terms.push_back(eq.terms[j]);
                }else{
                    rhs -= eq.terms[j].factor * s;
                }
            }
            if(terms.size() == 0){
                LOGD("not simple %u already solved or system overdetermined", eqId & ~EQN_ID_NOT_SIMPLE_BIT);
            }else if(terms.size() == 1){
                solved[terms[0].var] = rhs / terms[0].factor;
                markEquations(eq.terms[0].var, eqId);
            }else if(terms.size() == 2){
                std::vector<ui_eqn_t>& eqns = vars_to_eqns[terms[0].var];
                for( ui_eqn_t eqId2 : eqns ){
                    if(eqId2 != eqId){
                        if(eqId2 & EQN_ID_NOT_SIMPLE_BIT){
                            LOGW("%u solvable but not implemented", eqId);
                        }else{
                            UiSimpleEquation& eq2 = simpleEquations[eqId2];
                            float m21,m22;
                            if(eq2.term1.var == terms[1].var){
                                m21 = eq2.term2.factor;
                                m22 = eq2.term1.factor;
                            }else if(eq2.term2.var == terms[1].var){
                                m21 = eq2.term1.factor;
                                m22 = eq2.term2.factor;
                            }else{
                                continue;
                            }
                            float rhs2 = eq2.rhs.get().toGL(metrics);
                            float m11 = terms[0].factor;
                            float m12 = terms[1].factor;
                            //carmers's rule
                            float det = m11*m22 - m21*m12;
                            float s1 = (rhs*m22 - rhs2*m12) / det;
                            float s2 = (m11*rhs2 - m21*rhs) / det;
                            solved[terms[0].var] = s1;
                            solved[terms[1].var] = s2;
                            markEquations(terms[0].var, eqId, eqId2);
                            markEquations(terms[1].var, eqId, eqId2);
                        }
                    }
                }
            }else{
                LOGW("cant solve %u", eqId);
            }
        }else{
            UiSimpleEquation& eq = simpleEquations[eqId];
            float rhs = eq.rhs.get().toGL(metrics);
            float s1 = solved[eq.term1.var];
            float s2 = solved[eq.term2.var];
            if(std::isnan(s1)){
                s1 = (rhs - eq.term2.factor * s2) / eq.term1.factor;
                solved[eq.term1.var] = s1;
                markEquations(eq.term1.var, eqId);
            }else if(std::isnan(s2)){
                s2 = (rhs - eq.term1.factor * s1) / eq.term2.factor;
                solved[eq.term2.var] = s2;
                markEquations(eq.term2.var, eqId);
            }else{
                LOGD("%u already solved or system overdetermined", eqId);
            }
        }
    }
    
    //uint64_t build_end = getNanos();
    
    for(auto it=builders.begin();it!=builders.end();it++){
        it->first->metrics = metrics;
        Rect rect = Rect::bltr(
            solved[it->second.bottomV],
            solved[it->second.leftV],
            solved[it->second.topV],
            solved[it->second.rightV]
        );
        if(std::isnan(rect.top)) LOGW("{%s}.top not determined",typeid(*it->first).name());
        if(std::isnan(rect.bottom)) LOGW("{%s}.bottom not determined",typeid(*it->first).name());
        if(std::isnan(rect.left)) LOGW("{%s}.left not determined",typeid(*it->first).name());
        if(std::isnan(rect.right)) LOGW("{%s}.right not determined",typeid(*it->first).name());
        it->first->rect = rect;
        it->first->updateRect();
    }
    
    //uint64_t setting_end = getNanos();
    //LOGD("building: %f ms, setting: %f ms", (build_end - build_start) / 1000000.0, (setting_end - build_end) / 1000000.0);
}

void UiScene::clear(){
    for(UiEquation eq : equations){
        free(eq.terms);
    }
    equations.clear();
    simpleEquations.clear();
    builders.clear();
    vars_to_eqns.clear();
    maxVarId = VAR_ID_START;
}


void UiSceneBuilder::addEquation(float f1, ui_var_t v1, float f2, ui_var_t v2, LazyTypedSizeXY rhs){
    UiSimpleEquation eq;
    eq.term1.factor = f1;
    eq.term1.var = v1;
    eq.term2.factor = f2;
    eq.term2.var = v2;
    eq.rhs = rhs;
    ui_eqn_t eqnId = scene->simpleEquations.size();
    scene->simpleEquations.push_back(eq);
    scene->vars_to_eqns[v1].push_back(eqnId);
    scene->vars_to_eqns[v2].push_back(eqnId);
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
    UiEquation eq;
    eq.termCount = 4;
    eq.terms = (UiEquationTerm*) malloc(eq.termCount * sizeof(UiEquationTerm));
    eq.terms[0].factor = -1.0f;
    eq.terms[0].var = PARENT_LEFT_VAR_ID;
    eq.terms[1].factor = -1.0f;
    eq.terms[1].var = PARENT_RIGHT_VAR_ID;
    eq.terms[2].factor = fraction;
    eq.terms[2].var = leftV;
    eq.terms[3].factor = fraction;
    eq.terms[3].var = rightV;
    eq.rhs = TypedSize::zero.asXSize().lazy();
    ui_eqn_t eqId = scene->equations.size() | EQN_ID_NOT_SIMPLE_BIT;
    scene->equations.push_back(eq);
    scene->vars_to_eqns[PARENT_LEFT_VAR_ID].push_back(eqId);
    scene->vars_to_eqns[PARENT_RIGHT_VAR_ID].push_back(eqId);
    scene->vars_to_eqns[leftV].push_back(eqId);
    scene->vars_to_eqns[rightV].push_back(eqId);
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