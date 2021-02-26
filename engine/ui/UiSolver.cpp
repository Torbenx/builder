#include "UiSolver.hpp"
#include <algorithm>
#include <cmath>
#include <math.h>
#include "../util/log.hpp"

UiSolver::UiSolver(size_t fixedCount){
    this->fixedCount = fixedCount;
    fixed = (float*)malloc(fixedCount * sizeof(float));
    varCount = fixedCount;
}

void UiSolver::addEquation(float f1, ui_var_t v1, float f2, ui_var_t v2, LazyTypedSizeXY rhs){
    UiSimpleEquation eq;
    eq.term1.factor = f1;
    eq.term1.var = v1;
    eq.term2.factor = f2;
    eq.term2.var = v2;
    eq.rhs = rhs;
    ui_eqn_t eqnId = simpleEquations.size();
    simpleEquations.push_back(eq);
    vars_to_eqns[v1].push_back(eqnId);
    vars_to_eqns[v2].push_back(eqnId);
}

void UiSolver::solve(){
    freeSolution();
    
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
    for(ui_var_t var=0;var<fixedCount;var++){
        UiSolution& sol = solutions[var];
        sol.svar = var;
        sol.factor = 1.0f;
        sol.offset = 0.0f;
    }
    for(size_t i=fixedCount;i<varCount;i++){
        solutions[i].svar = UI_VAR_INVALID;
    }
    
    //reduce
    std::list<ui_eqn_t> generalQueue;
    ui_svar_t svarCount = fixedCount;
    svars_to_eqns.resize(svarCount);
    for(ui_var_t var=0;var<fixedCount;var++){
        exploreVar(var, UI_EQN_INVALID);
    }
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
    
    values = (float*)malloc((svarCount-fixedCount) * sizeof(float));
    for(size_t i=0;i<svarCount-fixedCount;i++){
        values[i] = NAN;
    }
    
    for(size_t i=0;i<svarCount;i++){
        printf("%u: ", i);
        for(size_t j=0;j<svars_to_eqns[i].size();j++){
            ui_eqn_t eq = svars_to_eqns[i][j];
            printf("%u, ", eq);
        }
        printf("\n");
    }
    
    for(;;){
        ui_svar_t start = UI_VAR_INVALID;
        for(size_t i=0;i<svarCount-fixedCount;i++){
            if(std::isnan(values[i])){
                start = i+fixedCount;
                break;
            }
        }
        if(start == UI_VAR_INVALID)break;
        eqnCount = 0;
        solveVars.clear();
        m_rhs.clear();
        entries.clear();
        solveVars.push_back(start); //make sure no one explore's it
        exploreVar2(start, UI_EQN_INVALID);
        LOGD("solving %u equations for %u variables", eqnCount, solveVars.size());
        if(eqnCount == 1){
            float factor = 0.0f;
            for(auto it=entries.begin();it!=entries.end();it++){
                factor += it->factor;
            }
            values[solveVars[0]-fixedCount] = m_rhs[0] / factor;
        }else{
            solveLinearSystem();
        }
    }
}

void UiSolver::exploreVar(ui_var_t var, ui_eqn_t origin){
    std::vector<ui_eqn_t>& eqns = vars_to_eqns[var];
    for(ui_eqn_t eq : eqns){
        if(eq == origin)continue;
        exploreEq(eq, var);
    }
}

void UiSolver::exploreEq(ui_eqn_t eq, ui_var_t origin){
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
            LOGD("system overdetermined!");
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

void UiSolver::exploreVar2(ui_svar_t svar, ui_eqn_t origin){
    LOGD("%u -> %u", origin, svar);
    std::vector<ui_eqn_t>& eqns = svars_to_eqns[svar];
    for(ui_eqn_t eq : eqns){
        if(eq == origin)continue;
        exploreEq2(eq, svar);
    }
}

void UiSolver::exploreEq2(ui_eqn_t eq, ui_svar_t origin){
    LOGD("%u => %u", origin, eq);
    UiGeneralEquation& equation = generalEquations[eq];
    
    ui_svar_t min_svar = UI_VAR_INVALID;
    for(size_t i=0;i<equation.termCount;i++){
        UiEquationTerm& term = equation.terms[i];
        UiSolution& sol = solutions[term.var];
        if(sol.svar < fixedCount)continue;
        if(sol.svar < min_svar)min_svar = sol.svar;
    }
    bool addEntry = (origin == min_svar);
    float rhs = equation.rhs.get().toGL(metrics);
    
    if(addEntry)
        LOGD("%u added!", eq);
    
    for(size_t i=0;i<equation.termCount;i++){
        UiEquationTerm& term = equation.terms[i];
        UiSolution& sol = solutions[term.var];
        if(sol.svar < fixedCount){
            if(addEntry){
                rhs -= (fixed[sol.svar] * sol.factor + sol.offset) * term.factor;
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
            exploreVar2(sol.svar, eq);
        }
    }
    if(addEntry){
        eqnCount++;
        m_rhs.push_back(rhs);
    }
}

void UiSolver::solveLinearSystem(){
    size_t size = solveVars.size();
    float matrix[size*(size+1)];
    for(size_t j=0;j<size;j++){
        for(size_t k=0;k<size;k++){
            matrix[j*(size+1)+k] = 0.0f;
        }
        matrix[j*(size+1)+size] = m_rhs[j];
    }
    for(UiMatrixEntry& entry : entries){
        matrix[entry.eqn*(size+1)+entry.var] += entry.factor;
    }
    
    for(size_t j=0;j<size;j++){
        for(size_t k=0;k<size+1;k++){
            printf("%+f ", matrix[j*(size+1)+k]);
        }
        printf("\n");
    }
    printf("\n");
    
    for(size_t i=0;i<size-1;i++){
        float max = 0.0f;
        size_t r = 0;
        for(size_t j=i;j<size;j++){
            if(abs(matrix[j*(size+1)+i]) > fabs(max)){
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
    
    for(size_t j=0;j<size;j++){
        for(size_t k=0;k<size+1;k++){
            printf("%+f ", matrix[j*(size+1)+k]);
        }
        printf("\n");
    }
    printf("\n");
    
    for(size_t i=0;i<size;i++){
        values[solveVars[i] - fixedCount] = matrix[i*(size+1)+size];
    }
}

float UiSolver::getSolution(ui_var_t var){
    UiSolution& sol = solutions[var];
    float value;
    if(sol.svar >= fixedCount){
        value = values[sol.svar - fixedCount];
    }else{
        value = fixed[sol.svar];
    }
    return sol.factor * value + sol.offset;
}

void UiSolver::clear(){
    for(UiGeneralEquation& eq : generalEquations){
        free(eq.terms);
    }
    generalEquations.clear();
    simpleEquations.clear();
    vars_to_eqns.clear();
    varCount = fixedCount;
}

void UiSolver::freeSolution(){
    if(solutions){
        free(solutions);
        solutions = nullptr;
    }
    if(values){
        free(values);
        values = nullptr;
    }
}

UiSolver::~UiSolver(){
    freeSolution();
    free(fixed);
}