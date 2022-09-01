#pragma once

#include "Inverse.hpp"
#include <vector>

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
public:
    class NewVariable {
    public:
        // The index of the new variable.
        uint32_t index = MAX_INDEX;

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
        float newDualValue() { return -dualFactor; }

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

        float invF = std::numeric_limits<float>::quiet_NaN();
        Vector invConstraint;
    };
    Inverse inverse;

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
        var.invF = -1.0f / var.deltaX.findValue(var.index);
        var.dualFactor = var.oldS * var.invF;
    }

    // deltaX, deltaDual and newDual must be set
    void completeVariable(NewVariable var) {
        if (var.index == inverse.inverse.size())
            inverse.expandPreInverted(std::move(var.deltaX), std::move(var.invConstraint), var.invF);
        else
            inverse.replaceColumnPreInverted(var.index, std::move(var.deltaX));
    }
};

class UnderConstraintLinearProgram : public LinearProgram {
private:
    struct LimitingFactor {
        union {
            uint32_t variableIndex;
            uint32_t constraintIndex;
        };
        float newXValue = std::numeric_limits<float>::infinity();
        bool isVariable;

        static LimitingFactor variable(uint32_t index, float newX) {
            LimitingFactor r;
            r.variableIndex = index;
            r.newXValue = newX;
            r.isVariable = true;
            return r;
        }
        static LimitingFactor constraint(uint32_t index, float newX) {
            LimitingFactor r;
            r.constraintIndex = index;
            r.newXValue = newX;
            r.isVariable = false;
            return r;
        }
    };

public:
    std::vector<float> q;
    std::vector<uint32_t> internalIndices;
    Matrix A, transposeA;
    std::vector<float> x;
    std::vector<uint32_t> userIndices;

    uint32_t addConstraint(float b) {
        uint32_t index = q.size();
        VERIFY(Sparse, internalIndices.size() == index);
        VERIFY(Sparse, transposeA.size() == index);
        q.push_back(b);
        internalIndices.push_back(MAX_INDEX);
        transposeA.appendColumn(allocateEmpty());

        /*A.columns.reserve(index + 1);
        x.reserve(index + 1);
        userIndices.reserve(index + 1);*/

        return index;
    }

    void debug() {
        log<Tag::Sparse>("x:");
        for (int_t i = 0; i < x.size(); i++) {
            log<Tag::Sparse>("{}: {}", i, x[i]);
        }
    }

    std::array<uint64_t, 8> clocks {};
    int_t iterations = 0;

    Vector subOptimalVariable(Vector column, float oldS) {
        // column.debug();
        // clocks[0] += __rdtsc();
        auto var = newVariable();
        Vector internalColumn = translateToInternalIndices(column);
        var.oldS = oldS;
        var.column = internalColumn;
        computeDeltaX(var);

        // clocks[1] += __rdtsc();

        Vector deltaConstraint = add(A.multiply(var.deltaX), column, -1.0f);
        // clocks[2] += __rdtsc();
        auto limit = findLimitingFactor(var.deltaX, deltaConstraint);
        // log("newX={} limited by {} {}", limit.newXValue, limit.isVariable ? "variable" : "constraint", limit.isVariable ? limit.variableIndex : limit.constraintIndex);

        // clocks[3] += __rdtsc();

        if (limit.isVariable) {
            var.index = limit.variableIndex;
            limitingVariable(var);

            for (Entry e : A[limit.variableIndex]) {
                transposeA[e.index].removeIndex(limit.variableIndex);
            }
        } else {
            var.constraint = transposeA[limit.constraintIndex];
            var.constraintCoefficient = column.findValue(limit.constraintIndex);
            limitingConstraint(var);

            VERIFY(Sparse, A.size() == var.index);
            VERIFY(Sparse, x.size() == var.index);
            VERIFY(Sparse, userIndices.size() == var.index);
            A.appendColumn({});
            x.push_back(0.0f);
            userIndices.push_back(limit.constraintIndex);
            internalIndices[limit.constraintIndex] = var.index;
        }

        // clocks[4] += __rdtsc();

        for (Entry e : column) {
            Vector& oldRow = transposeA[e.index];
            Vector newRow = oldRow.copy(1);
            newRow.add({ var.index, e.value });
            oldRow = std::move(newRow);
        }
        A[var.index] = std::move(column);
        addTo(x.data(), var.deltaX, -limit.newXValue);
        x[var.index] = limit.newXValue;
        addTo(q.data(), deltaConstraint, limit.newXValue);

        // clocks[5] += __rdtsc();

        Vector dualUpdate = computeDualUpdate(var, limit);
        // clocks[6] += __rdtsc();
        completeVariable(std::move(var));
        // clocks[7] += __rdtsc();
        iterations += 1;
        return dualUpdate;
    }

private:
    Vector computeDualUpdate(NewVariable& var, LimitingFactor limit) {
        return entrywiseTransformOutOfPlace(
            var.deltaDual,
            [&](Entry e) -> Entry {
                return { userIndices[e.index], var.dualFactor * e.value };
            },
            1,
            [&](Vector& v) {
                if (!limit.isVariable)
                    v.appendEntry({ limit.constraintIndex, var.newDualValue() });
            });
    }

    LimitingFactor findLimitingFactor(VectorView deltaX, VectorView deltaConstraint) {
        float minVariable = std::numeric_limits<float>::infinity();
        uint32_t minVariableIndex;
        for (Entry e : deltaX) {
            if (!(e.value > EPSILON))
                continue;
            float newX = x[e.index] / e.value;
            if (newX + EPSILON < minVariable) {
                minVariable = newX;
                minVariableIndex = e.index;
            }
        }

        float minConstraint = std::numeric_limits<float>::infinity();
        uint32_t minConstraintIndex;
        for (Entry e : deltaConstraint) {
            if (!(e.value < -EPSILON))
                continue;
            float newX = -q[e.index] / e.value;
            if (newX + EPSILON < minConstraint) {
                minConstraint = newX;
                minConstraintIndex = e.index;
            }
        }

        LimitingFactor l;
        l.isVariable = minVariable <= minConstraint;
        l.newXValue = l.isVariable ? minVariable : minConstraint;
        if (l.isVariable)
            l.variableIndex = minVariableIndex;
        else
            l.constraintIndex = minConstraintIndex;
        VERIFY(Sparse, std::isfinite(l.newXValue));
        return l;
    }

    Vector translateToInternalIndices(VectorView in) {
        return entrywiseTransformOutOfPlace(in, [&](Entry e) -> Entry {
            return { internalIndices[e.index], e.value };
        });
    }
};

}
