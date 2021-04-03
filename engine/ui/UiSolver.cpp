#include "ui/UiSolver.hpp"
#include "util/log.hpp"
#include <algorithm>
#include <cmath>
#include <math.h>

UiSolver::UiSolver(){
    varCount = 0;
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
    addEq(v1, eqnId);
    addEq(v2, eqnId);
}

void UiSolver::addEq(ui_var_t var, ui_eqn_t eq){
    if(var & UI_VAR_CONST_BIT){
        fixed_to_eqns[var & ~UI_VAR_CONST_BIT].push_back(eq);
    }else{
        vars_to_eqns[var].push_back(eq);
    }
}

void UiSolver::solve(){
    freeSolution();

    /*for(ui_var_t i=0;var<fixedCount;var++){
        printf("%u: ", i);
        for(size_t j=0;j<fixed_to_eqns[i].size();j++){
            ui_eqn_t eq = fixed_to_eqns[i][j];
            if(eq & UI_EQN_NOT_SIMPLE_BIT){
                printf("%u; ", eq & ~UI_EQN_NOT_SIMPLE_BIT);
            }else{
                printf("%u, ", eq);
            }
        }
        printf("\n");
    }
    for(ui_var_t i=0;i<varCount;i++){
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

    solutions = new UiSolution[varCount];
    for(size_t i=0;i<varCount;i++){
        solutions[i].svar = UI_VAR_INVALID;
    }

    markedEquations.clear();
    for(ui_var_t var=0;var<fixedCount;var++){
        std::vector<ui_eqn_t>& eqns = fixed_to_eqns[var];
        for(ui_eqn_t eq : eqns){
            if(eq & UI_EQN_NOT_SIMPLE_BIT){
                markedEquations.push_back(eq & ~UI_EQN_NOT_SIMPLE_BIT);
            }else{
                exploreSimpleEq(eq, var | UI_VAR_CONST_BIT);
            }
        }
    }

    svarCount = 0;
    eqnExplored.clear();
    eqnExplored.resize(generalEquations.size());

    for(size_t i=0;i<markedEquations.size();i++){
        svarStart = svarCount;
        m_rhs.clear();
        entries.clear();

        exploreGeneralEq(markedEquations[i]);

        size_t variables = svarCount - svarStart;
        //LOGD("Solving %u equations for %u variables", m_rhs.size(), variables);
        if(variables == 0){
            //nothing to do
        }else if(variables == 1){
            float factor = 0.0f;
            for(auto it=entries.begin();it!=entries.end();it++){
                factor += it->factor;
            }
            values.push_back(m_rhs[0] / factor);
        }else{
            solveLinearSystem(variables);
        }
    }
}

void UiSolver::exploreVar(ui_var_t var, ui_eqn_t origin){
    std::vector<ui_eqn_t>& eqns = vars_to_eqns[var];
    for(ui_eqn_t eq : eqns){
        if(eq == origin)continue;
        if(eq & UI_EQN_NOT_SIMPLE_BIT){
            if(solutions[var].svar & UI_VAR_CONST_BIT){
                markedEquations.push_back(eq & ~UI_EQN_NOT_SIMPLE_BIT);
            }else{
                exploreGeneralEq(eq & ~UI_EQN_NOT_SIMPLE_BIT);
            }
        }else{
            exploreSimpleEq(eq, var);
        }
    }
}

void UiSolver::exploreSimpleEq(ui_eqn_t eq, ui_var_t sourceVar){
    UiSimpleEquation& equation = simpleEquations[eq];
    ui_var_t targetVar;
    float sourceFactor, targetFactor;
    if(equation.term1.var == sourceVar){
        sourceFactor = equation.term1.factor;
        targetVar = equation.term2.var;
        targetFactor = equation.term2.factor;
    }else{
        //equation.term2.var == sourceVar
        sourceFactor = equation.term2.factor;
        targetVar = equation.term1.var;
        targetFactor = equation.term1.factor;
    }
    if(targetVar & UI_VAR_CONST_BIT){
        LOGW("system overdetermined, target %x is fixed! (ignoring eq %x)", targetVar, eq);
        return;
    }
    UiSolution& target = solutions[targetVar];
    if(target.svar != UI_VAR_INVALID){
        LOGW("system overdetermined, target %x already solved! (ignoring eq %x)", targetVar, eq);
        return;
    }
    UiSolution source;
    if(sourceVar & UI_VAR_CONST_BIT){
        source.svar = sourceVar;
        source.factor = 1.0f;
        source.offset = 0.0f;
    }else{
        source = solutions[sourceVar];
    }
    target = source;

    target.factor *= -sourceFactor;
    target.offset *= -sourceFactor;

    target.offset += equation.rhs.get().toGL(metrics);

    target.factor /= targetFactor;
    target.offset /= targetFactor;

    exploreVar(targetVar, eq);
}

void UiSolver::exploreGeneralEq(ui_eqn_t eq){
    if(eqnExplored[eq]){
        return;
    }
    eqnExplored[eq] = true;

    UiGeneralEquation& equation = generalEquations[eq];
    size_t eqnIndex = m_rhs.size();
    m_rhs.push_back(0.0f);

    float rhs = equation.rhs.get().toGL(metrics);
    for(size_t i=0;i<equation.termCount;i++){
        UiEquationTerm& term = equation.terms[i];
        if(term.var & UI_VAR_CONST_BIT){
            rhs -= fixed[term.var & ~UI_VAR_CONST_BIT] * term.factor;
            continue;
        }
        UiSolution& sol = solutions[term.var];
        if(sol.svar & UI_VAR_CONST_BIT){
            rhs -= (fixed[sol.svar & ~UI_VAR_CONST_BIT] * sol.factor + sol.offset) * term.factor;
            continue;
        }
        if(sol.svar == UI_VAR_INVALID){
            sol.svar = svarCount++;
            sol.factor = 1.0f;
            sol.offset = 0.0f;
            exploreVar(term.var, eq | UI_EQN_NOT_SIMPLE_BIT);
        }
        UiMatrixEntry entry;
        entry.eqn = eqnIndex;
        entry.var = sol.svar - svarStart;
        entry.factor = sol.factor * term.factor;
        entries.push_back(entry);
        rhs -= sol.offset * term.factor;
    }
    m_rhs[eqnIndex] = rhs;
}

void UiSolver::solveLinearSystem(size_t size){
    std::vector<float> matrix(size*(size+1));

    auto print_matrix = [&](){
        for(size_t j=0;j<size;j++){
            for(size_t k=0;k<size+1;k++){
                printf("%+f ", matrix[j*(size+1)+k]);
            }
            printf("\n");
        }
        printf("\n");
    };

    for(size_t j=0;j<size;j++){
        for(size_t k=0;k<size;k++){
            matrix[j*(size+1)+k] = 0.0f;
        }
        matrix[j*(size+1)+size] = m_rhs[j];
    }
    for(UiMatrixEntry& entry : entries){
        matrix[entry.eqn*(size+1)+entry.var] += entry.factor;
    }
    //print_matrix();

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
    //print_matrix();

    for(size_t i=0;i<size;i++){
        values.push_back(matrix[i*(size+1)+size]);
    }
}

float UiSolver::getSolution(ui_var_t var){
    if(var & UI_VAR_CONST_BIT){
        return fixed[var & ~UI_VAR_CONST_BIT];
    }
    UiSolution& sol = solutions[var];
    float value;
    if(sol.svar & UI_VAR_CONST_BIT){
        value = fixed[sol.svar & ~UI_VAR_CONST_BIT];
    }else if(sol.svar < svarCount){
        value = values[sol.svar];
    }else{
        return NAN;
    }
    return sol.factor * value + sol.offset;
}

void UiSolver::clear(){
    for(UiGeneralEquation& eq : generalEquations){
        delete eq.terms;
    }
    generalEquations.clear();
    simpleEquations.clear();
    vars_to_eqns.clear();
    fixed_to_eqns.clear();
    fixed_to_eqns.resize(fixedCount);
    varCount = 0;
}

void UiSolver::freeSolution(){
    if(solutions){
        delete solutions;
        solutions = nullptr;
    }
    values.clear();
}

UiSolver::~UiSolver(){
    freeSolution();
}
