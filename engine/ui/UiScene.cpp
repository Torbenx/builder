#include "UiScene.hpp"
#include "../util/log.hpp"
#include "../util/time.hpp"
#include <algorithm>

#define GET_UI_BUILDER(comp) scene->layout(comp)

UiSceneBuilder UiScene::layout(UiComponent* comp){
    ui_var_t base;
    if(comp){
        auto it = builders.find(comp);
        if(it == builders.end()){
            base = varCount;
            builders[comp] = base;
            varCount += 4;
            vars_to_eqns.resize(varCount);
        }else{
            base = it->second;
        }
    }else{
        base = 0;
    }
    UiSceneBuilder builder;
    builder.scene = this;
    builder.comp = comp;
    builder.base = base;
    return builder;
}

void UiScene::setParent(Rect rect){
    this->parent = rect;
}

void UiScene::exploreVar(ui_var_t var, ui_eqn_t origin){
    std::vector<ui_eqn_t>& eqns = vars_to_eqns[var];
    for(ui_eqn_t eq : eqns){
        if(eq == origin)continue;
        exploreEq(eq, var);
    }
}

void UiScene::exploreEq(ui_eqn_t eq, ui_var_t origin){
    if(eq & UI_EQN_NOT_SIMPLE_BIT){
        std::vector<ui_eqn_t>& eqns = svars_to_eqns[solutions[origin].svar];
        ui_eqn_t eqI = eq & ~UI_EQN_NOT_SIMPLE_BIT;
        if(std::find(eqns.begin(), eqns.end(), eqI) == eqns.end()){
            eqns.push_back(eqI);
        }
        auto where = std::find(markedEquations.begin(), markedEquations.end(), eq);
        if(where == markedEquations.end()){
            markedEquations.push_back(eq);
        }
    }else{
        UiSimpleEquation& equation = simpleEquations[eq];
        UiSolution& sol1 = solutions[equation.term1.var];
        UiSolution& sol2 = solutions[equation.term2.var];
        if(sol1.svar == UI_VAR_INVALID && sol2.svar == UI_VAR_INVALID){
            LOGD("this shoundn't happen");
            return;
        }
        if(sol1.svar != UI_VAR_INVALID && sol2.svar != UI_VAR_INVALID){
            LOGD("found loop, also shoundn't happen");
        }else{
            if(sol1.svar == UI_VAR_INVALID){
                sol1 = sol2;
                sol1.factor *= -equation.term2.factor; sol1.offset *= -equation.term2.factor;
                sol1.offset += equation.rhs.get().toGL(metrics);
                sol1.factor /= equation.term1.factor; sol1.offset /= equation.term1.factor;
                exploreVar(equation.term1.var, eq);
            }else{
                sol2 = sol1;
                sol2.factor *= -equation.term1.factor; sol2.offset *= -equation.term1.factor;
                sol2.offset += equation.rhs.get().toGL(metrics);
                sol2.factor /= equation.term2.factor; sol2.offset /= equation.term2.factor;
                exploreVar(equation.term2.var, eq);
            }
        }
    }
}

void UiScene::exploreVar2(ui_svar_t svar, ui_eqn_t origin){
    std::vector<ui_eqn_t>& eqns = svars_to_eqns[svar];
    for(ui_eqn_t eq : eqns){
        if(eq == origin)continue;
        exploreEq2(eq, svar);
    }
}

void UiScene::exploreEq2(ui_eqn_t eq, ui_svar_t origin){
    UiGeneralEquation& equation = generalEquations[eq];
    
    ui_svar_t min_svar = UI_VAR_INVALID;
    for(size_t i=0;i<equation.termCount;i++){
        UiEquationTerm& term = equation.terms[i];
        UiSolution& sol = solutions[term.var];
        if(sol.svar < UI_VAR_START)continue;
        if(sol.svar < min_svar)min_svar = sol.svar;
    }
    bool addEntry = (origin == min_svar);
    float rhs = equation.rhs.get().toGL(metrics);
    
    for(size_t i=0;i<equation.termCount;i++){
        UiEquationTerm& term = equation.terms[i];
        UiSolution& sol = solutions[term.var];
        if(sol.svar < UI_VAR_START){
            if(addEntry){
                rhs -= (values[sol.svar] * sol.factor + sol.offset) * term.factor;
            }
            continue;
        }
        auto where = std::find(solveVars.begin(), solveVars.end(), sol.svar);
        
        if(addEntry){
            UiMatrixEntry entry;
            entry.eqn = eqnCount;
            entry.var = where - solveVars.begin();
            entry.factor = sol.factor * term.factor;
            entries.push_back(entry);
            rhs -= sol.offset * term.factor;
        }
        
        if(where == solveVars.end()){
            solveVars.push_back(sol.svar);
        }
        
        exploreVar2(sol.svar, eq);
    }
    if(addEntry){
        eqnCount++;
        m_rhs.push_back(rhs);
    }
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
    
    solutions = (UiSolution*) malloc(varCount * sizeof(UiSolution));
    auto setIdentitySolution = [&](ui_var_t var){
        UiSolution& sol = solutions[var];
        sol.svar = var;
        sol.factor = 1.0f;
        sol.offset = 0.0f;
    };
    setIdentitySolution(UI_VAR_TOP);
    setIdentitySolution(UI_VAR_BOTTOM);
    setIdentitySolution(UI_VAR_LEFT);
    setIdentitySolution(UI_VAR_RIGHT);
    for(size_t i=UI_VAR_START;i<varCount;i++){
        solutions[i].svar = UI_VAR_INVALID;
    }
    
    //reduce
    std::list<ui_eqn_t> generalQueue;
    ui_svar_t svarCount = UI_VAR_START;
    svars_to_eqns.resize(svarCount);
    exploreVar(UI_VAR_TOP, UI_EQN_INVALID);
    exploreVar(UI_VAR_BOTTOM, UI_EQN_INVALID);
    exploreVar(UI_VAR_LEFT, UI_EQN_INVALID);
    exploreVar(UI_VAR_RIGHT, UI_EQN_INVALID);
    for(size_t i=0;i<markedEquations.size();i++){
        ui_eqn_t eq = markedEquations[i];
        UiGeneralEquation& equation = generalEquations[eq & ~UI_EQN_NOT_SIMPLE_BIT];
        for(size_t i=0;i<equation.termCount;i++){
            UiEquationTerm& term = equation.terms[i];
            UiSolution& sol = solutions[term.var];
            if(sol.svar == UI_VAR_INVALID){
                sol.svar = svarCount++;
                sol.factor = 1.0f;
                sol.offset = 0.0f;
                svars_to_eqns.resize(svarCount);
                svars_to_eqns[sol.svar].push_back(eq & ~UI_EQN_NOT_SIMPLE_BIT);
                exploreVar(term.var, eq);
            }
        }
    }
    
    values = (float*)malloc(svarCount * sizeof(float));
    values[UI_VAR_TOP] = parent.top;
    values[UI_VAR_BOTTOM] = parent.bottom;
    values[UI_VAR_LEFT] = parent.left;
    values[UI_VAR_RIGHT] = parent.right;
    for(ui_svar_t i=UI_VAR_START;i<svarCount;i++){
        values[i] = NAN;
    }
    
    for(;;){
        ui_svar_t start = UI_VAR_INVALID;
        for(ui_svar_t i=UI_VAR_START;i<svarCount;i++){
            if(std::isnan(values[i])){
                start = i;
                break;
            }
        }
        if(start == UI_VAR_INVALID)break;
        eqnCount = 0;
        solveVars.clear();
        m_rhs.clear();
        entries.clear();
        exploreVar2(start, UI_EQN_INVALID);
        //LOGD("solving %u equations for %u variables", eqnCount, solveVars.size());
        if(eqnCount == 1){
            float factor = 0.0f;
            for(auto it=entries.begin();it!=entries.end();it++){
                factor += it->factor;
            }
            values[solveVars[0]] = m_rhs[0] / factor;
        }
    }
    
    //set
    auto getSolution = [&](ui_var_t var){
        UiSolution& sol = solutions[var];
        if(sol.svar >= svarCount) return NAN;
        return sol.factor * values[sol.svar] + sol.offset;
    };
    for(auto it=builders.begin();it!=builders.end();it++){
        it->first->metrics = metrics;
        Rect rect = Rect::bltr(
            getSolution(it->second+UI_VAR_BOTTOM),
            getSolution(it->second+UI_VAR_LEFT),
            getSolution(it->second+UI_VAR_TOP),
            getSolution(it->second+UI_VAR_RIGHT)
        );
        if(std::isnan(rect.top)) LOGW("{%s}.top not determined",typeid(*it->first).name());
        if(std::isnan(rect.bottom)) LOGW("{%s}.bottom not determined",typeid(*it->first).name());
        if(std::isnan(rect.left)) LOGW("{%s}.left not determined",typeid(*it->first).name());
        if(std::isnan(rect.right)) LOGW("{%s}.right not determined",typeid(*it->first).name());
        it->first->rect = rect;
        it->first->updateRect();
    }
    free(solutions);
    free(values);
    return;
    
    
    
    
    /*for(;;){
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
                auto where = std::find(extraEquations.begin(), extraEquations.end(), eqI); //binary search
                if(where == extraEquations.end()){
                    extraEquations.push_back(eqI);
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
    
    //solve
    LOGD("solving %u equations for %u variables", generalEquations.size()+extraEquations.size(), linearSolveVarCount);
    
    if(linearSolveVarCount!=0){
        std::vector<ui_var_t> solveVars;
        std::list<UiMatrixEntry> entries;
        size_t eqnCount = 0;
        
        size_t position = 0;
        
        ui_var_t start_var = linearSolveVars.front();
        solveVars.push_back(start_var);
        LOGD("starting with %u", start_var);
        
        //not simple equations form at least a spanning tree
        while(position < solveVars.size()){
            ui_var_t var = solveVars[position];
            position += 1;
            std::vector<ui_eqn_t>& eqns = vars_to_eqns[var];
            for(ui_eqn_t eqI : eqns){
                if(eqI & UI_EQN_NOT_SIMPLE_BIT){
                    ui_eqn_t eqIndex = eqI & ~UI_EQN_NOT_SIMPLE_BIT;
                    UiGeneralEquation& eq = generalEquations[eqIndex];
                    for(size_t i=0;i<eq.termCount;i++){
                        UiEquationTerm& term = eq.terms[i];
                        if(term.var >= UI_VAR_START){
                            UiSolution& sol = solutions[term.var];
                            if(term.var == sol.var){
                                auto where = std::find(solveVars.begin(), solveVars.end(), term.var);
                                if(where == solveVars.end()){
                                    LOGD("queueing_%u from %u", term.var, eqIndex);
                                    solveVars.push_back(term.var);
                                }
                                UiMatrixEntry entry;
                                entry.eqn = eqnCount;
                                entry.var = where - solveVars.begin();
                                entry.factor = term.factor;
                                entries.push_back(entry);
                            }
                        }
                    }
                }else{
                    UiSimpleEquation& eq = simpleEquations[eqI];
                    UiMatrixEntry entry;
                    entry.eqn = eqnCount;
                    
                    UiSolution& sol1 = solutions[eq.term1.var];
                    auto where = std::find(solveVars.begin(), solveVars.end(), sol1.var);
                    if(where == solveVars.end()){
                        LOGD("queueing %u from %u", sol1.var, eqI);
                        solveVars.push_back(sol1.var);
                    }
                    entry.var = where - solveVars.begin();
                    entry.factor = eq.term1.factor;
                    entries.push_back(entry);
                    
                    UiSolution& sol2 = solutions[eq.term2.var];
                    where = std::find(solveVars.begin(), solveVars.end(), sol2.var);
                    if(where == solveVars.end()){
                        LOGD("queueing %u from %u", sol2.var, eqI);
                        solveVars.push_back(sol2.var);
                    }
                    entry.var = where - solveVars.begin();
                    entry.factor = eq.term2.factor;
                    entries.push_back(entry);
                }
                eqnCount++;
            }
        }
        LOGD("split into %u variables and %u equations", solveVars.size(), eqnCount);
        for(auto it=solveVars.begin();it!=solveVars.end();it++){
            printf("%u, ", *it);
        }
        printf("\n");
        for(auto it=linearSolveVars.begin();it!=linearSolveVars.end();it++){
            printf("%u, ", *it);
        }
        printf("\n");
        
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
        
        for(size_t j=0;j<size;j++){
            for(size_t k=0;k<size+1;k++){
                printf("%+f ", matrix[j*(size+1)+k]);
            }
            printf("\n");
        }
        printf("\n");
        
        //gaussian eliminiation
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
    }*/
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

UiSceneBuilder& UiSceneBuilder::above(UiComponent* other, TypedSize padding){
    UiSceneBuilder::addEquation(1, bottom(), -1, GET_UI_BUILDER(other).top(), padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::below(UiComponent* other, TypedSize padding){
    UiSceneBuilder::addEquation(-1, top(), 1, GET_UI_BUILDER(other).bottom(), padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::toLeftOf(UiComponent* other, TypedSize padding){
    UiSceneBuilder::addEquation(-1, right(), 1, GET_UI_BUILDER(other).left(), padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::toRightOf(UiComponent* other, TypedSize padding){
    UiSceneBuilder::addEquation(1, left(), -1, GET_UI_BUILDER(other).right(), padding.asXSize().lazy());
    return *this;
}


UiSceneBuilder& UiSceneBuilder::alignTop(UiComponent* other, TypedSize padding){
    UiSceneBuilder::addEquation(-1, top(), 1, GET_UI_BUILDER(other).top(), padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignBottom(UiComponent* other, TypedSize padding){
    UiSceneBuilder::addEquation(1, bottom(), -1, GET_UI_BUILDER(other).bottom(), padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignLeft(UiComponent* other, TypedSize padding){
    UiSceneBuilder::addEquation(1, left(), -1, GET_UI_BUILDER(other).left(), padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignRight(UiComponent* other, TypedSize padding){
    UiSceneBuilder::addEquation(-1, right(), 1, GET_UI_BUILDER(other).right(), padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignHorizontal(UiComponent* other, TypedSize padding){
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(1, left(), -1, ob.left(), padding.asXSize().lazy());
    UiSceneBuilder::addEquation(-1, right(), 1, ob.right(), padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignHorizontal(UiComponent* other, TypedSize paddingLeft, TypedSize paddingRight){
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(1, left(), -1, ob.left(), paddingLeft.asXSize().lazy());
    UiSceneBuilder::addEquation(-1, right(), 1, ob.right(), paddingRight.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignVertical(UiComponent* other, TypedSize padding){
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(-1, top(), 1, ob.top(), padding.asYSize().lazy());
    UiSceneBuilder::addEquation(1, bottom(), -1, ob.bottom(), padding.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::alignVertical(UiComponent* other, TypedSize paddingTop, TypedSize paddingBottom){
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(-1, top(), 1, ob.top(), paddingTop.asYSize().lazy());
    UiSceneBuilder::addEquation(1, bottom(), -1, ob.bottom(), paddingBottom.asYSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::align(UiComponent* other, TypedSize padding){
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(-1, top(), 1, ob.top(), padding.asYSize().lazy());
    UiSceneBuilder::addEquation(1, bottom(), -1, ob.bottom(), padding.asYSize().lazy());
    UiSceneBuilder::addEquation(1, left(), -1, ob.left(), padding.asXSize().lazy());
    UiSceneBuilder::addEquation(-1, right(), 1, ob.right(), padding.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::align(UiComponent* other, TypedSize paddingTop, TypedSize paddingLeft, TypedSize paddingBottom, TypedSize paddingRight){
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    UiSceneBuilder::addEquation(-1, top(), 1, ob.top(), paddingTop.asYSize().lazy());
    UiSceneBuilder::addEquation(1, bottom(), -1, ob.bottom(), paddingBottom.asYSize().lazy());
    UiSceneBuilder::addEquation(1, left(), -1, ob.left(), paddingLeft.asXSize().lazy());
    UiSceneBuilder::addEquation(-1, right(), 1, ob.right(), paddingRight.asXSize().lazy());
    return *this;
}


UiSceneBuilder& UiSceneBuilder::centerHorizontal(UiComponent* other, float fraction){
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    UiGeneralEquation eq;
    eq.termCount = 4;
    eq.terms = (UiEquationTerm*) malloc(eq.termCount * sizeof(UiEquationTerm));
    eq.terms[0].factor = -fraction;
    eq.terms[0].var = ob.left();
    eq.terms[1].factor = -(1.0f-fraction);
    eq.terms[1].var = ob.right();
    eq.terms[2].factor = fraction;
    eq.terms[2].var = left();
    eq.terms[3].factor = 1.0f-fraction;
    eq.terms[3].var = right();
    eq.rhs = TypedSize::zero.asXSize().lazy();
    ui_eqn_t eqId = scene->generalEquations.size() | UI_EQN_NOT_SIMPLE_BIT;
    scene->generalEquations.push_back(eq);
    scene->vars_to_eqns[ob.left()].push_back(eqId);
    scene->vars_to_eqns[ob.right()].push_back(eqId);
    scene->vars_to_eqns[left()].push_back(eqId);
    scene->vars_to_eqns[right()].push_back(eqId);
    return *this;
}

UiSceneBuilder& UiSceneBuilder::centerVertical(UiComponent* other, float fraction){
    UiSceneBuilder ob = GET_UI_BUILDER(other);
    UiGeneralEquation eq;
    eq.termCount = 4;
    eq.terms = (UiEquationTerm*) malloc(eq.termCount * sizeof(UiEquationTerm));
    eq.terms[0].factor = -fraction;
    eq.terms[0].var = ob.top();
    eq.terms[1].factor = -(1.0f-fraction);
    eq.terms[1].var = ob.bottom();
    eq.terms[2].factor = fraction;
    eq.terms[2].var = top();
    eq.terms[3].factor = 1.0f-fraction;
    eq.terms[3].var = bottom();
    eq.rhs = TypedSize::zero.asYSize().lazy();
    ui_eqn_t eqId = scene->generalEquations.size() | UI_EQN_NOT_SIMPLE_BIT;
    scene->generalEquations.push_back(eq);
    scene->vars_to_eqns[ob.top()].push_back(eqId);
    scene->vars_to_eqns[ob.bottom()].push_back(eqId);
    scene->vars_to_eqns[top()].push_back(eqId);
    scene->vars_to_eqns[bottom()].push_back(eqId);
    return *this;
}


UiSceneBuilder& UiSceneBuilder::width(TypedSize width){
    UiSceneBuilder::addEquation(1, right(), -1, left(), width.asXSize().lazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::height(TypedSize height){
    UiSceneBuilder::addEquation(1, top(), -1, bottom(), height.asYSize().lazy());
    return *this;
}


UiSceneBuilder& UiSceneBuilder::wrapContentWidth(){
    UiSceneBuilder::addEquation(1, right(), -1, left(), comp->getContentWidthLazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::wrapContentHeight(){
    UiSceneBuilder::addEquation(1, top(), -1, bottom(), comp->getContentHeightLazy());
    return *this;
}

UiSceneBuilder& UiSceneBuilder::wrapContent(){
    wrapContentWidth();
    wrapContentHeight();
    return *this;
}