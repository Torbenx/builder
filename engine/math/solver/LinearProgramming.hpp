#pragma once

#include "math/solver/LUDecomposition.hpp"

template<class ZeroTester>
class LinearProgramming {
public:
    using Matrix = SparseMatrix<float, uint32_t, ZeroTester>;
    LUPDecomposition<float, uint32_t, ZeroTester> basisLU;
    Matrix basis, extra;
    Permutation<uint32_t> permutation;
    DenseVector<float, uint32_t> solution, dual, multipliers, basisObjective, bound, extraObjective, slack;
    uint32_t basisSize, extraSize;

    void doStuff() {
        basisSize = basis.columns.size();
        extraSize = extra.columns.size();
        permutation.setIdentity(basisSize + extraSize);
        basisLU.u = basis;
        basisLU.decompose();
        dual.resize(basisSize);
        multipliers.resize(extraSize);

        while (true) {
            LOGD("basis:");
            basis.debug();
            LOGD("extra:");
            extra.debug();

            // calculate multipliers
            basisLU.inverseTranspose(dual, basisObjective);
            LOGD("dual:");
            dual.debug();
            SparseOps::multiply(multipliers, dual, extra);
            multipliers.add(extraObjective, -1.0f);
            LOGD("multipliers:");
            multipliers.debug();

            // pick enteringIndex
            uint32_t enteringIndex = extraSize;
            float max = 0.0f;
            for (uint32_t i = 0; i < extraSize; i++) {
                if (multipliers[i] > max) {
                    max = multipliers[i];
                    enteringIndex = i;
                }
            }
            if (enteringIndex == extraSize) {
                LOGD("done");
                return;
            }
            LOGD("enteringIndex=%u", enteringIndex);

            // pick leavingIndex
            typename Matrix::Column delta = extra.columns[enteringIndex];
            basisLU.inverseUnsorted(delta);
            uint32_t leavingIndex = basisSize;
            float min = std::numeric_limits<float>::infinity();
            for (auto entry : delta) {
                if (entry.value > 0.0f) {
                    float f = solution[entry.index] / entry.value;
                    if (f < min) {
                        min = f;
                        leavingIndex = entry.index;
                    }
                }
            }
            if (leavingIndex == basisSize) {
                LOGD("unbound");
                return;
            }
            LOGD("leavingIndex=%u", leavingIndex);

            // swap
            permutation.swap(leavingIndex, basisSize + enteringIndex);
            std::swap(basisObjective[leavingIndex], extraObjective[enteringIndex]);
            std::swap(basis.columns[leavingIndex], extra.columns[enteringIndex]);
            basisLU.replaceColumn(leavingIndex, basis.columns[leavingIndex]);

            solution.add(delta, -min);
            solution[leavingIndex] = min;
            LOGD("solution:");
            solution.debug();
        }
    }
};
