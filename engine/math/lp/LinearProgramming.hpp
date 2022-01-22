#pragma once

namespace sparse {

class LinearProgramming {
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
    std::vector<float> dual;
    Inverse inverse;

    // Adds a variable, thats violating the KKT conditions, to the basis.
    // column: the already used constaints involving the new variable
    // returns: dx, the amount each variable decreases, when the new variable
    //          increases by 1. This includes the new variable itself, hence the
    //          last entry is always -1.
    Vector newVariable(VectorView column) {
        return inverse.preInvertColumn(column);
    }

    // dx: the vector returned by newVariable()
    // newX: value of the new variable
    // constraint: the constraint limiting the increase in x
    // oldS: the old value of s for this variable. Since the variable previously
    //       violated the KKT conditions this should always be negative.
    void completeVariable(Vector dx, float newX, Vector constraint, float oldS) {
        uint32_t newIndex = inverse.size();

        Entry lastEntry = constraint[constraint.size() - 1];
        float corner = 0.0f;
        if (lastEntry.index == newIndex) {
            corner = lastEntry.value;
            constraint.shrikeBy(1);
        }

        float invF = inverse.preInvertCornerWithColumn(constraint, dx, corner);
        Vector ddual = inverse.preInvertRow(constraint);

        float newDual = -oldS * invF;
        dual.resize(newIndex + 1);
        addTo(dual.data(), ddual, -newDual);

        inverse.expandPreInverted(dx, ddual, invF);
    }
};

}
