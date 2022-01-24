#pragma once

namespace sparse {

class LinearProgram {
    /*
     * Solves problems of the type:
     *   A.x <= b; x >= 0; max c.x
     *
     * KKT conditions:
     *   A.x + q = b; dual.A - s = c
     *   s.x = 0; dual.q = 0
     *   x >= 0; dual >= 0; s >= 0; q >= 0
     */

private:
    Inverse inverse;

public:
    class NewVariable {
    public:
        // The index of the new variable.
        uint32_t index = std::numeric_limits<uint32_t>::max();

        // The old value of s for this variable. Since the variable previously
        // violated the KKT conditions this should always be negative.
        float oldS = std::numeric_limits<float>::quiet_NaN();

        // The already used constaints involving the new variable.
        VectorView column;

        // constraint limiting the increase in x
        VectorView constraint;
        // coefficient in the constraint for the new variable
        float constraintCoefficient;

        // The amount each old variable decreases, when the new variable
        // increases by 1.
        Vector deltaX;

        // the increase in the dual variables is deltaDual * dualFactor
        VectorView deltaDual;
        float dualFactor = std::numeric_limits<float>::quiet_NaN();

        /*NewVariable(NewVariable&& other)
            : index(other.index)
            , oldS(other.oldS)
            , column(other.column)
            , constraint(other.constraint)
            , deltaX(std::move(other.deltaX))
            , deltaDual(std::move(other.deltaDual))
            , newDual(other.newDual)
            , invF(other.invF) {
            other.clearFields();
        }*/

    private:
        float invF = std::numeric_limits<float>::quiet_NaN();
        Vector invConstraint;

        void clearFields() {
            index = std::numeric_limits<uint32_t>::max();
            olds = std::numeric_limits<float>::quiet_NaN();
            column = {};
            constraint = {};
            newDual = std::numeric_limits<float>::quiet_NaN();
            invF = std::numeric_limits<float>::quiet_NaN();
        }
    };

    // Adds a variable, thats violating the KKT conditions, to the basis.
    NewVariable newVariable() {
        return {};
    }

    // column must be set
    void computeDeltaX(NewVariable& var) {
        var.deltaX = inverse.preInvertColumn(var.column);
    }

    // deltaX, constraint and oldS must be set
    void limitingConstraint(NewVariable& var) {
        var.index = inverse.inverse.size();
        var.invConstraint = inverse.preInvertRow(var.constraint);
        var.deltaDual = var.invConstraint;
        var.invF = inverse.preInvertCornerWithColumn(var.deltaX, var.constraint, var.constraintCoefficient);
        var.dualFactor = var.oldS * var.invF;
    }

    // deltaX, index and oldS must be set
    void limitingVariable(NewVariable& var) {
        var.deltaDual = inverse.inverseTranspose[var.index];
        var.invF = -1.0f / deltaX.findValue(var.index);
        var.dualFactor = var.oldS * var.invF;
    }

    // deltaX, deltaDual and newDual must be set
    void completeVariable(NewVariable var) {
        if (var.index == inverse.inverse.size())
            inverse.expandPreInverted(std::move(var.deltaX), std::move(var.deltaDual), var.invF);
        else
            inverse.replaceColumnPreInverted(var.index, std::move(var.deltaX));
    }
};

class UnderConstraintLinearProgram : LinearProgram {
private:
    struct ConstraintEntry {
        uint32_t internalIndex = std::numeric_limits<uint32_t>::max();
        float q = -1.0f;
    };
    std::vector<ConstraintEntry> constraints;
    Matrix A, transposeA;
    struct SolutionEntry {
        float x = 0.0f;
    };
    std::vector<SolutionEntry> solution;

public:
    void subOptimalVariable(Vector column, float oldS) {
        auto var = newVariable();
        Vector internalColumn = translateToInternalIndices(column);
        var.oldS = oldS;
        var.column = internalColumn;
        computeDeltaX(var);

        float minT = std::numeric_limits<float>::infinity();
        uint32_t minIndex;
        bool isMinVariable;
        for (int_t i = 0; i < var.deltaX.size(); i++) {
            Entry e = var.deltaX[i];
            if (e.value < EPSILON)
                continue;
            auto sol = solution[e.index];
            float t = sol.x / e.value;
            if (t < minT) {
                minT = min;
                minIndex = e.index;
                isMinVariable = true;
            }
        }

        Vector deltaConstraint = add(A.multiply(var.deltaX), column, -1.0f);
        for (Entry e : deltaConstraint) {
            if (e.value > -EPSILON)
                continue;
            auto con = constraint[e.index];
            float t = -con.q / e.value;
            if (t < minT) {
                minT = min;
                minIndex = e.index;
                isMinVariable = false;
            }
        }

        VERIFY(Sparse, std::infinite(minT));
        if (isMinVariable) {
            var.index = minIndex;
            limitingVariable(var);

            for (Entry e : A[minIndex]) {
                transposeA[e.index].removeIndex(minIndex);
            }
            for (Entry e : column) {
                Vector& oldRow = transposeA[e.index];
                Vector newRow = oldRow.copy();
                newRow.add({ newColumnIndex, e.value });
                oldRow = std::move(newRow);
            }
            A[minIndex] = std::move(column);
        } else {
            var.constraint = transposeA[minIndex];
            var.constraintCoefficient = column.findValue(minIndex);
            limitingConstraint(var);

            uint32_t newColumnIndex = var.index;
            for (Entry e : column) {
                Vector& oldRow = transposeA[e.index];
                Vector newRow = oldRow.copy();
                newRow.appendEntry({ newColumnIndex, e.value });
                oldRow = std::move(newRow);
            }
            VERIFY(Sparse, A.size() == newColumnIndex);
            A.appendColumn(std::move(column));
        }

        completeVariable(var);
    }

    Vector translateToInternalIndices(VectorView in) {
        Vector out = allocate(in.size());
        int_t outOffset = 0;
        for (Entry inE : in) {
            Entry& outE = out[outOffset]:
            outE.index = constraints[inE.index].internalIndex;
            outE.value = inE.value;
            outOffset += outE.index != std::numeric_limits<uint32_t>::max() ? 1 : 0;
        }
        return out;
    }
};

}
