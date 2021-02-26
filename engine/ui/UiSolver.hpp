#ifndef HEADER_UI_UISOLVER_H
#define HEADER_UI_UISOLVER_H

#include <map>
#include <vector>
#include <list>
#include "TypedSize.hpp"

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

class UiSolver{
    public:
        float *fixed;
        std::vector<UiGeneralEquation> generalEquations;
        std::vector<UiSimpleEquation> simpleEquations;
        std::vector<std::vector<ui_eqn_t>> vars_to_eqns;
        ui_var_t varCount;
        ScreenMetrics metrics;
        
        UiSolver(size_t fixedVars);
        void addEquation(float f1, ui_var_t v1, float f2, ui_var_t v2, LazyTypedSizeXY rhs);
        void solve();
        float getSolution(ui_var_t var);
        void clear();
        void freeSolution();
        ~UiSolver();
        
    private:
        UiSolution *solutions = nullptr;
        float *values = nullptr;
        size_t fixedCount;
        
        std::vector<ui_eqn_t> markedEquations;
        std::vector<std::vector<ui_eqn_t>> svars_to_eqns;
        std::vector<ui_svar_t> solveVars;
        std::vector<UiMatrixEntry> entries;
        size_t eqnCount;
        std::vector<float> m_rhs;
        
        void exploreVar(ui_var_t var, ui_eqn_t origin);
        void exploreEq(ui_eqn_t eq, ui_var_t origin);
        void exploreVar2(ui_svar_t var, ui_eqn_t origin);
        void exploreEq2(ui_eqn_t eq, ui_svar_t origin);
        void solveLinearSystem();
};

#endif