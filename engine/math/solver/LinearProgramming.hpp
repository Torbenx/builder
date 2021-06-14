#pragma once

#include "math/solver/LUDecomposition.hpp"

template<class ZeroTester>
class LinearProgramming {
public:
    using Matrix = SparseMatrix<float, uint32_t, ZeroTester>;
    LUPDecomposition<float, uint32_t, ZeroTester> basisLU;
    Matrix basis, extra;
    Permutation<uint32_t> permutation;
    DenseVector<float, uint32_t> solution, dual, multipliers, bound, basisObjective, extraObjective, negativeObjective;
    uint32_t basisSize, extraSize, n, numConstraints;
    bool removingNegatives = false;
    uint32_t* constraints;

    void doStuff() {
        basisSize = basis.columns.size();
        extraSize = extra.columns.size();
        permutation.setIdentity(extraSize + basisSize);
        basisLU.u = basis;
        basisLU.decompose();
        dual.resize(basisSize);
        multipliers.resize(extraSize);
        negativeObjective.resize(basisSize);

        while (true) {
            //LOGD("basis:");
            //basis.debug();
            //basisObjective.debug();
            //LOGD("extra:");
            //extra.debug();
            //extraObjective.debug();

            // calculate multipliers
            basisLU.inverseTranspose(dual, removingNegatives ? negativeObjective : basisObjective);
            //LOGD("dual:");
            //dual.debug();
            SparseOps::multiply(multipliers, dual, extra);
            if (!removingNegatives) {
                multipliers.add(extraObjective, -1.0f);
            }
            //LOGD("multipliers:");
            //multipliers.debug();

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
                if (removingNegatives) {
                    LOGD("couldn't remove negatives!");
                    return;
                }
                std::pair<uint32_t, float> entries[n];
                bool allGood = true;
                for (uint32_t i = 0; i < numConstraints; i++) {
                    for (uint32_t j = 0; j < n; j++) {
                        entries[j].first = permutation[constraints[i * n + j]];
                        entries[j].second = entries[j].first >= extraSize ? solution[entries[j].first - extraSize] : 0.0f;
                    }
                    std::sort(entries, entries + n,
                        [](const std::pair<uint32_t, float>& a, const std::pair<uint32_t, float>& b) {
                            return a.second < b.second;
                        });
                    uint32_t lastFailure = n;
                    for (uint32_t j = 0; j < n; j++) {
                        //std::cout << "{" << entries[j].first << ", " << entries[j].second << "}\n";
                        if (entries[j].second - j < 0.0f)
                            lastFailure = j;
                    }
                    if (lastFailure == n)
                        continue;

                    basisSize++;
                    float sum = 0.0f;
                    for (uint32_t k = 0; k <= lastFailure; k++) {
                        sum += k;
                        if (entries[k].first < extraSize) {
                            extra.columns[entries[k].first].pushBack({ 1.0f, basisSize - 1 });
                        } else {
                            basis.columns[entries[k].first - extraSize].pushBack({ 1.0f, basisSize - 1 });
                        }
                    }
                    bound.resize(basisSize);
                    bound[basisSize - 1] = sum;
                    solution.resize(basisSize);
                    solution[basisSize - 1] = -sum;
                    basis.columns.emplace_back().pushBack({ -1.0f, basisSize - 1 });
                    dual.resize(basisSize);
                    basisObjective.resize(basisSize);
                    basisLU.u = basis;
                    basisLU.decompose();
                    permutation.expandBy(1);
                    negativeObjective.resize(basisSize);
                    negativeObjective[basisSize - 1] = -1.0f;
                    removingNegatives = true;
                    allGood = false;
                }
                if (allGood) {
                    //LOGD("done");
                    break;
                }
                continue;
            }
            //LOGD("enteringIndex=%u", enteringIndex);

            // pick leavingIndex
            typename Matrix::Column delta = extra.columns[enteringIndex];
            //LOGD("entering:");
            //delta.debug();
            // basisLU.l.debug();
            // basisLU.u.debug();
            basisLU.inverseUnsorted(delta);
            delta.sort();
            //LOGD("delta:");
            //delta.debug();

            uint32_t leavingIndex = basisSize;
            float min = std::numeric_limits<float>::infinity();
            for (auto entry : delta) {
                float f = solution[entry.index] / entry.value;
                if (f > 0.0f || (solution[entry.index] == 0 && entry.value > 0.0f)) {
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
            //LOGD("min=%f", min);
            //LOGD("leavingIndex=%u", leavingIndex);

            // swap
            permutation.swap(extraSize + leavingIndex, enteringIndex);
            std::swap(basisObjective[leavingIndex], extraObjective[enteringIndex]);
            std::swap(basis.columns[leavingIndex], extra.columns[enteringIndex]);
            basisLU.replaceColumn(leavingIndex, basis.columns[leavingIndex]);
            //basisLU.u = basis;
            //basisLU.decompose();

            solution.add(delta, -min);
            solution[leavingIndex] = min;
            //LOGD("solution (replaced):");
            //solution.debug();

            //LOGD("score=%f", SparseOps::dot(basisObjective, solution));
            if (removingNegatives) {
                if (SparseOps::dot(negativeObjective, solution) <= 0.0f) {
                    removingNegatives = false;
                    negativeObjective.setEmpty();
                }
            }
        }
    }
};
