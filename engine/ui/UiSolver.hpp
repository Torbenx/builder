#ifndef HEADER_UI_UISOLVER_H
#define HEADER_UI_UISOLVER_H

#include <map>
#include <vector>
#include <list>
#include "TypedSize.hpp"

#define UI_VAR_INVALID 0x7fffffff

typedef signed int ui_var_t;

typedef ui_var_t ui_svar_t;

#define UI_EQN_NOT_SIMPLE_BIT 0x80000000
#define UI_EQN_INVALID 0xffffffff

typedef signed int ui_eqn_t;

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
        float fixedCount;
        
        std::vector<UiGeneralEquation> generalEquations;
        std::vector<UiSimpleEquation> simpleEquations;
        ui_var_t varCount;
        ScreenMetrics metrics;

        UiSolver();
        void addEquation(float f1, ui_var_t v1, float f2, ui_var_t v2, LazyTypedSizeXY rhs);
        void addEq(ui_var_t var, ui_eqn_t eq);
        void solve();
        float getSolution(ui_var_t var);
        void clear();
        void freeSolution();
        ~UiSolver();

    private:
        UiSolution *solutions = nullptr;
        std::vector<float> values;

        std::vector<ui_eqn_t> markedEquations;
        size_t svarCount;
        size_t svarStart;
        std::vector<bool> eqnExplored;
        std::vector<UiMatrixEntry> entries;
        std::vector<float> m_rhs;

        void exploreVar(ui_var_t var, ui_eqn_t origin);
        void exploreEq(ui_eqn_t eq, ui_var_t origin);
        void exploreSimpleEq(ui_eqn_t eq, ui_var_t origin);
        void exploreGeneralEq(ui_eqn_t eq);
        void solveLinearSystem(size_t size);
        
    public:
    
        std::vector<std::vector<ui_eqn_t>> vars_to_eqns;
        std::vector<std::vector<ui_eqn_t>> fixed_to_eqns;
};

#endif
