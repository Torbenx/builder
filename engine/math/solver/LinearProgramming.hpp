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
    uint32_t basisSize, extraSize, n, numConstraints, numVariables;
    bool removingNegatives;
    uint32_t* constraints;

    void doStuff() {
        relaxed();
        DenseVector<float, uint32_t> sol(numVariables);
        for (uint32_t i = 0; i < numVariables; i++) {
            uint32_t j = permutation[i];
            if (j >= extraSize) {
                sol[i] = std::ceil(solution[j - extraSize]);
            } else {
                sol[i] = 0.0f;
            }
        }
        sol.debug();
    }

    void relaxed() {
        basisSize = numConstraints;
        extraSize = numVariables;
        permutation.setIdentity(extraSize + basisSize);

        basis.setEmpty(basisSize);
        float f = 0.5f * (n - 1) * n;
        bound.resize(basisSize);
        negativeObjective.resize(basisSize);
        solution.resize(basisSize);
        basisObjective.resize(basisSize);
        for (uint32_t i = 0; i < basisSize; i++) {
            basis.columns[i].pushBack({ -1.0f, i });
            bound[i] = f;
            negativeObjective[i] = -1.0f;
            solution[i] = -f;
        }
        removingNegatives = true;
        basisLU.u = basis;
        basisLU.decompose();

        extra.setEmpty(extraSize);
        extraObjective.resize(extraSize);
        for (uint32_t i = 0; i < numConstraints; i++) {
            for (uint32_t j = 0; j < n; j++) {
                extra.columns[constraints[i * n + j]].pushBack({ 1.0f, i });
            }
        }
        for (uint32_t i = 0; i < extraSize; i++) {
            extraObjective[i] = 1.0f;
        }

        dual.resize(basisSize);
        multipliers.resize(extraSize);

        while (true) {
            LOGD("basis:");
            basis.debug();
            basisObjective.debug();
            LOGD("extra:");
            extra.debug();
            extraObjective.debug();

            // calculate multipliers
            basisLU.inverseTranspose(dual, removingNegatives ? negativeObjective : basisObjective);
            // LOGD("dual:");
            // dual.debug();
            SparseOps::multiply(multipliers, dual, extra);
            if (!removingNegatives) {
                multipliers.add(extraObjective, -1.0f);
            }
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
                    float sum = 0.0f;
                    for (uint32_t j = 0; j < n; j++) {
                        // std::cout << "{" << entries[j].first << ", " << entries[j].second << "}\n";
                        sum += entries[j].second;
                        if (sum - (j * (j + 1) >> 1) < 0.0f) {
                            lastFailure = j;
                        }
                    }
                    if (lastFailure == n)
                        continue;

                    basisSize++;
                    for (uint32_t k = 0; k <= lastFailure; k++) {
                        if (entries[k].first < extraSize) {
                            extra.columns[entries[k].first].pushBack({ 1.0f, basisSize - 1 });
                        } else {
                            basis.columns[entries[k].first - extraSize].pushBack({ 1.0f, basisSize - 1 });
                        }
                    }
                    bound.resize(basisSize);
                    bound[basisSize - 1] = (lastFailure * (lastFailure + 1)) >> 1;
                    solution.resize(basisSize);
                    solution[basisSize - 1] = -(float)((lastFailure * (lastFailure + 1)) >> 1);
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
                    // break;
                }
                if (allGood) {
                    // LOGD("done");
                    break;
                }
                continue;
            }
            // LOGD("enteringIndex=%u", enteringIndex);

            // pick leavingIndex
            typename Matrix::Column delta = extra.columns[enteringIndex];
            // LOGD("entering:");
            // delta.debug();
            //  basisLU.l.debug();
            //  basisLU.u.debug();
            basisLU.inverseUnsorted(delta);
            delta.sort();
            // LOGD("delta:");
            // delta.debug();

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
            // LOGD("min=%f", min);
            // LOGD("leavingIndex=%u", leavingIndex);

            // swap
            permutation.swap(extraSize + leavingIndex, enteringIndex);
            std::swap(basisObjective[leavingIndex], extraObjective[enteringIndex]);
            std::swap(basis.columns[leavingIndex], extra.columns[enteringIndex]);
            uint32_t luIndex = basisLU.p[leavingIndex];
            basisLU.u.columns[luIndex] = basis.columns[leavingIndex];
            basisLU.replaceColumn(luIndex);
            // basisLU.u = basis;
            // basisLU.decompose();

            solution.add(delta, -min);
            solution[leavingIndex] = min;
            LOGD("solution (replaced):");
            solution.debug();

            LOGD("score=%f", SparseOps::dot(basisObjective, solution));
            if (removingNegatives) {
                bool allPositive = true;
                for (uint32_t i = 0; i < basisSize; i++) {
                    if (solution[i] >= 0.0f) {
                        negativeObjective[i] = 0.0f;
                    } else {
                        allPositive = false;
                    }
                }
                if (allPositive) {
                    removingNegatives = false;
                }
            }
        }
    }
};
