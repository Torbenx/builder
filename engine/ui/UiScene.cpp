#include "UiScene.hpp"
#include "../util/log.hpp"
#include "../util/time.hpp"
#include <algorithm>

#define GET_UI_BUILDER(comp) scene->layout(comp)

UiSceneBuilder& UiScene::layout(UiComponent& comp){
    auto builderIt = builders.find(&comp);
    if(builderIt == builders.end()){
        UiSceneBuilder& builder = builders[&comp];
        builder.scene = this;
        builder.comp = &comp;
        builder.topV = varCount++;
        builder.bottomV = varCount++;
        builder.leftV = varCount++;
        builder.rightV = varCount++;
        
        vars_to_eqns.resize(varCount);
        
        return builder;
    }
    return builderIt->second;
}

void UiScene::setParent(Rect rect){
    this->parent = rect;
}

void UiScene::build(){
    /*for(size_t i=0;i<varCount;i++){
        printf("%u: ", i);
        for(size_t j=0;j<vars_to_eqns[i].size();j++){
            ui_eqn_t eq = vars_to_eqns[i][j];
            if(eq & UI_EQN_NOT_SIMPLE_BIT){
                printf("%u; ", eq & ~UI_EQN_NOT_SIMPLE_BIT);
            }else{
                printf("%u, ", eq);
            }
        }
        printf("\n");
    }*/
    
    UiSolution solutions[varCount];
    auto setIdentitySolution = [&](ui_var_t var){
        UiSolution& sol = solutions[var];
        sol.var = var;
        sol.factor = 1.0f;
        sol.offset = 0.0f;
    };
    setIdentitySolution(UI_VAR_PARENT_TOP);
    setIdentitySolution(UI_VAR_PARENT_BOTTOM);
    setIdentitySolution(UI_VAR_PARENT_LEFT);
    setIdentitySolution(UI_VAR_PARENT_RIGHT);
    for(size_t i=UI_VAR_START;i<varCount;i++){
        solutions[i].var = UI_VAR_INVALID;
    }
    
    size_t linearSolveVarCount = 0;
    std::vector<ui_var_t> linearSolveVars;
    std::vector<ui_eqn_t> extraEquations;
    
    //reduce
    std::vector<bool> processedGeneralEquations(generalEquations.size());
    std::list<ui_eqn_t> simpleQueue;
    std::list<ui_eqn_t> generalQueue;
    auto queueEquations = [&](ui_var_t var, ui_eqn_t ignore=UI_EQN_INVALID){
        //LOGD("queueing for %u",var);
        std::vector<ui_eqn_t>& eqns = vars_to_eqns[var];
        for(ui_eqn_t eq : eqns){
            if(eq != ignore){
                if(eq & UI_EQN_NOT_SIMPLE_BIT){
                    generalQueue.push_back(eq);
                    //LOGD("queueing general %u",eq & ~UI_EQN_NOT_SIMPLE_BIT);
                }else{
                    simpleQueue.push_back(eq);
                    //LOGD("queueing simple %u",eq);
                }
            }
        }
    };
    queueEquations(UI_VAR_PARENT_TOP);
    queueEquations(UI_VAR_PARENT_BOTTOM);
    queueEquations(UI_VAR_PARENT_LEFT);
    queueEquations(UI_VAR_PARENT_RIGHT);
    
    for(;;){
        if(!simpleQueue.empty()){
            ui_eqn_t eqI = simpleQueue.front();
            simpleQueue.pop_front();
            UiSimpleEquation& eq = simpleEquations[eqI];
            //LOGD("%u: [%u]*(%f) + [%u]*(%f) = %f", eqI, eq.term1.var, eq.term1.factor, eq.term2.var, eq.term2.factor, eq.rhs.get().toGL(metrics));
            UiSolution& sol1 = solutions[eq.term1.var];
            UiSolution& sol2 = solutions[eq.term2.var];
            if(sol1.var == UI_VAR_INVALID && sol2.var == UI_VAR_INVALID){
                LOGD("this shoundn't happen");
                continue;
            }
            if(sol1.var != UI_VAR_INVALID && sol2.var != UI_VAR_INVALID){
                //found loop
                //LOGD("found loop %u, %u", sol1.var, sol2.var);
                auto where = std::lower_bound(extraEquations.begin(), extraEquations.end(), eqI); //binary search
                if(where == extraEquations.end() || *where != eqI){
                    extraEquations.insert(where, eqI);
                }
            }else{
                if(sol1.var == UI_VAR_INVALID){
                    sol1 = sol2;
                    sol1.factor *= -eq.term2.factor;
                    sol1.offset *= -eq.term2.factor;
                    sol1.offset += eq.rhs.get().toGL(metrics);
                    sol1.factor /= eq.term1.factor;
                    sol1.offset /= eq.term1.factor;
                    queueEquations(eq.term1.var, eqI);
                }else{
                    sol2 = sol1;
                    sol2.factor *= -eq.term1.factor;
                    sol2.offset *= -eq.term1.factor;
                    sol2.offset += eq.rhs.get().toGL(metrics);
                    sol2.factor /= eq.term2.factor;
                    sol2.offset /= eq.term2.factor;
                    queueEquations(eq.term2.var, eqI);
                }
            }
        }else{
            if(generalQueue.empty()){
                break;
            }
            ui_eqn_t eqIndex = generalQueue.front() & ~UI_EQN_NOT_SIMPLE_BIT;
            generalQueue.pop_front();
            if(processedGeneralEquations[eqIndex]){
                continue;
            }
            processedGeneralEquations[eqIndex] = true;
            
            UiGeneralEquation& eq = generalEquations[eqIndex];
            for(size_t i=0;i<eq.termCount;i++){
                UiEquationTerm& term = eq.terms[i];
                UiSolution& sol = solutions[term.var];
                if(sol.var == UI_VAR_INVALID){
                    sol.var = term.var;
                    sol.factor = 1.0f;
                    sol.offset = 0.0f;
                    queueEquations(term.var, eqIndex | UI_EQN_NOT_SIMPLE_BIT);
                    linearSolveVarCount++;
                    linearSolveVars.push_back(term.var);
                }
            }
        }
    }
    
    //reuse indentity entries as lookup table
    solutions[UI_VAR_PARENT_TOP].factor = parent.top;
    solutions[UI_VAR_PARENT_BOTTOM].factor = parent.bottom;
    solutions[UI_VAR_PARENT_LEFT].factor = parent.left;
    solutions[UI_VAR_PARENT_RIGHT].factor = parent.right;
    auto getSolution = [&](ui_var_t var){
        UiSolution& sol = solutions[var];
        if(sol.var == var) return sol.factor;
        if(sol.var >= varCount) return NAN;
        UiSolution& sol2 = solutions[sol.var];
        return sol2.factor * sol.factor + sol.offset;
    };
    
    //solve
    LOGD("solving %u equations for %u variables", generalEquations.size()+extraEquations.size(), linearSolveVarCount);
    size_t size = linearSolveVarCount;
    float matrix[(size+1)*size];
    std::sort(linearSolveVars.begin(), linearSolveVars.end());
    size_t i=0;
    for(;i<generalEquations.size();i++){
        UiGeneralEquation& eq = generalEquations[i];
        for(size_t j=0;j<size;j++){
            matrix[i*(size+1)+j] = 0.0f;
        }
        float rhs = eq.rhs.get().toGL(metrics);
        for(size_t j=0;j<eq.termCount;j++){
            UiEquationTerm& term = eq.terms[j];
            auto where = std::lower_bound(linearSolveVars.begin(), linearSolveVars.end(), term.var);
            if(where != linearSolveVars.end() && *where == term.var){
                matrix[i*(size+1)+(where-linearSolveVars.begin())] = term.factor;
            }else{
                rhs -= term.factor * getSolution(term.var);
            }
        }
        matrix[i*(size+1)+size] = rhs;
    }
    for(int ii=0;i<size;i++,ii++){
        UiSimpleEquation& eq = simpleEquations[extraEquations[ii]];
        for(size_t j=0;j<size;j++){
            matrix[i*(size+1)+j] = 0.0f;
        }
        float rhs = eq.rhs.get().toGL(metrics);
        //always hits
        UiSolution& sol1 = solutions[eq.term1.var];
        auto where = std::lower_bound(linearSolveVars.begin(), linearSolveVars.end(), sol1.var);
        matrix[i*(size+1)+(where-linearSolveVars.begin())] = eq.term1.factor * sol1.factor;
        rhs -= eq.term1.factor * sol1.offset;
        
        UiSolution& sol2 = solutions[eq.term2.var];
        where = std::lower_bound(linearSolveVars.begin(), linearSolveVars.end(), sol2.var);
        matrix[i*(size+1)+(where-linearSolveVars.begin())] = eq.term2.factor * sol2.factor;
        rhs -= eq.term2.factor * sol2.offset;
        
        matrix[i*(size+1)+size] = rhs;
    }
    
    /*for(size_t j=0;j<size;j++){
        for(size_t k=0;k<size+1;k++){
            printf("%+f ", matrix[j*(size+1)+k]);
        }
        printf("\n");
    }
    printf("\n");*/
    
    //gaussian eliminiation
    if(size!=0){
        for(size_t i=0;i<size-1;i++){
            float max = 0.0f;
            size_t r = 0;
            for(size_t j=i;j<size;j++){
                if(abs(matrix[j*(size+1)+i]) > abs(max)){
                    max = matrix[j*(size+1)+i];
                    r = j;
                }
            }
            float inv_max = 1.0f / max;
            for(size_t j=0;j<size;j++){
                if(j == r) continue;
                float f = matrix[j*(size+1)+i];
                for(size_t k=0;k<size+1;k++){
                    matrix[j*(size+1)+k] = matrix[j*(size+1)+k] - matrix[r*(size+1)+k]*f*inv_max;
                }
            }
            for(size_t k=0;k<size+1;k++){
                float temp = matrix[i*(size+1)+k];
                matrix[i*(size+1)+k] = matrix[r*(size+1)+k];
                matrix[r*(size+1)+k] = temp;
            }
            for(size_t k=0;k<size+1;k++){
                matrix[i*(size+1)+k] *= inv_max;
            }
        }
        float invf = 1.0f / matrix[(size-1)*(size+1)+(size-1)];
        for(size_t k=0;k<size+1;k++){
            matrix[(size-1)*(size+1)+k] *= invf;
        }
        for(size_t i=size-1;i!=-1;i--){
            for(size_t j=0;j<size;j++){
                if(j == i)continue;
                float f = matrix[j*(size+1)+i];
                for(size_t k=0;k<size+1;k++){
                    matrix[j*(size+1)+k] -= matrix[i*(size+1)+k] * f;
                }
            }
        }
        for(size_t i=0;i<size;i++){
            solutions[linearSolveVars[i]].factor = matrix[i*(size+1)+size];
        }
    }
    
    //set
    for(auto it=builders.begin();it!=builders.end();it++){
        it->first->metrics = metrics;
        Rect rect = Rect::bltr(
            getSolution(it->second.bottomV),
            getSolution(it->second.leftV),
            getSolution(it->second.topV),
            getSolution(it->second.rightV)
        );
        if(std::isnan(rect.top)) LOGW("{%s}.top not determined",typeid(*it->first).name());
        if(std::isnan(rect.bottom)) LOGW("{%s}.bottom not determined",typeid(*it->first).name());
        if(std::isnan(rect.left)) LOGW("{%s}.left not determined",typeid(*it->first).name());
        if(std::isnan(rect.right)) LOGW("{%s}.right not determined",typeid(*it->first).name());
        it->first->rect = rect;
        it->first->updateRect();
    }
}

void UiScene::clear(){
    for(UiGeneralEquation& eq : generalEquations){
        free(eq.terms);
    }
    generalEquations.clear();
    simpleEquations.clear();
    builders.clear();
    vars_to_eqns.clear();
    varCount = UI_VAR_START;
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
    //LOGD("adding equation %u", eqnId);
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
    UiSceneBuilder::addEquation(-1, topV, 1, UI_VAR_PARENT_TOP, padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignParentBottom(TypedSize padding){
    UiSceneBuilder::addEquation(1, bottomV, -1, UI_VAR_PARENT_BOTTOM, padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignParentLeft(TypedSize padding){
    UiSceneBuilder::addEquation(1, leftV, -1, UI_VAR_PARENT_LEFT, padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignParentRight(TypedSize padding){
    UiSceneBuilder::addEquation(-1, rightV, 1, UI_VAR_PARENT_RIGHT, padding.asXSize().lazy());
    return *this;
}


void UiSceneBuilder::centerHorizontalInternal(ui_var_t otherLeft, ui_var_t otherRight, float fraction){
    UiGeneralEquation eq;
    eq.termCount = 4;
    eq.terms = (UiEquationTerm*) malloc(eq.termCount * sizeof(UiEquationTerm));
    eq.terms[0].factor = -1.0f;
    eq.terms[0].var = otherLeft;
    eq.terms[1].factor = -1.0f;
    eq.terms[1].var = otherRight;
    eq.terms[2].factor = fraction;
    eq.terms[2].var = leftV;
    eq.terms[3].factor = fraction;
    eq.terms[3].var = rightV;
    eq.rhs = TypedSize::zero.asXSize().lazy();
    ui_eqn_t eqId = scene->generalEquations.size() | UI_EQN_NOT_SIMPLE_BIT;
    scene->generalEquations.push_back(eq);
    scene->vars_to_eqns[otherLeft].push_back(eqId);
    scene->vars_to_eqns[otherRight].push_back(eqId);
    scene->vars_to_eqns[leftV].push_back(eqId);
    scene->vars_to_eqns[rightV].push_back(eqId);
}

void UiSceneBuilder::centerVerticalInternal(ui_var_t otherTop, ui_var_t otherBottom, float fraction){
    UiGeneralEquation eq;
    eq.termCount = 4;
    eq.terms = (UiEquationTerm*) malloc(eq.termCount * sizeof(UiEquationTerm));
    eq.terms[0].factor = -1.0f;
    eq.terms[0].var = otherTop;
    eq.terms[1].factor = -1.0f;
    eq.terms[1].var = otherBottom;
    eq.terms[2].factor = fraction;
    eq.terms[2].var = topV;
    eq.terms[3].factor = fraction;
    eq.terms[3].var = bottomV;
    eq.rhs = TypedSize::zero.asYSize().lazy();
    ui_eqn_t eqId = scene->generalEquations.size() | UI_EQN_NOT_SIMPLE_BIT;
    scene->generalEquations.push_back(eq);
    scene->vars_to_eqns[otherTop].push_back(eqId);
    scene->vars_to_eqns[otherBottom].push_back(eqId);
    scene->vars_to_eqns[topV].push_back(eqId);
    scene->vars_to_eqns[bottomV].push_back(eqId);
}

UiSceneBuilder& UiSceneBuilder::centerHorizontal(float fraction){
    centerHorizontalInternal(UI_VAR_PARENT_LEFT, UI_VAR_PARENT_RIGHT, fraction);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerVertical(float fraction){
    centerVerticalInternal(UI_VAR_PARENT_TOP, UI_VAR_PARENT_BOTTOM, fraction);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerHorizontalInside(UiComponent& other, float fraction){
    UiSceneBuilder& ob = GET_UI_BUILDER(other);
    centerHorizontalInternal(ob.leftV, ob.rightV, fraction);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerVerticalInside(UiComponent& other, float fraction){
    UiSceneBuilder& ob = GET_UI_BUILDER(other);
    centerHorizontalInternal(ob.topV, ob.bottomV, fraction);
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