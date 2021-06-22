#pragma once

#include "math/solver/LUDecomposition.hpp"

template<class ZeroTester>
class LinearProgramming {
public:
    using Matrix = SparseMatrix<float, uint32_t, ZeroTester>;
    LUPDecomposition<float, uint32_t, ZeroTester> basisLU;
    Matrix basis, extra;
    DenseVector<float, uint32_t> solution, multipliers, bound, basisObjective, extraObjective;
    uint32_t basisSize, extraSize, n, numConstraints, numVariables;
    uint32_t* constraints;
    SparseVector<float, uint32_t, ZeroTester> delta, deltaDual, deltaMultipliers;
    std::vector<bool> dirtyMultipliers;

    void doStuff() {
        relaxed();
        // dual.debug();
    }

    void relaxed() {
        basisSize = numVariables;
        extraSize = numConstraints;

        basis.setEmpty(basisSize);
        bound.resize(basisSize);
        solution.resize(basisSize);
        basisObjective.resize(basisSize);
        for (uint32_t i = 0; i < basisSize; i++) {
            basis.columns[i].pushBack({ 1.0f, i });
            bound[i] = 1.0f;
            solution[i] = 1.0f;
        }
        basisLU.u = basis;
        basisLU.decompose();

        extra.setEmpty(extraSize);
        extraObjective.resize(extraSize);
        multipliers.resize(extraSize);
        dirtyMultipliers.resize(extraSize);
        dirtyMultipliers.flip();
        for (uint32_t i = 0; i < numConstraints; i++) {
            for (uint32_t j = 0; j < n; j++) {
                extra.columns[i].pushBack({ 1.0f, constraints[i * n + j] });
            }
        }
        for (uint32_t i = 0; i < extraSize; i++) {
            extraObjective[i] = 0.5f * (n - 1) * n;
            multipliers[i] = -0.5f * (n - 1) * n;
        }


        while (true) {
            /*LOGD("basis:");
            basis.debug();
            basisObjective.debug();
            LOGD("extra:");
            extra.debug();
            extraObjective.debug();
            LOGD("solution:");
            solution.debug();
            LOGD("dual:");
            dual.debug();
            LOGD("bound:");
            bound.debug();
            LOGD("multipliers:");
            multipliers.debug();
            LOGD("score=%f", SparseOps::dot(basisObjective, solution));
            LOGD("");*/
            LogTimer timer;
            timer.disarm();
            ZeroTester tester;

            // pick enteringIndex
            uint32_t enteringIndex = extraSize;
            float max = 0.0f;
            for (uint32_t i = 0; i < extraSize; i++) {
                if (dirtyMultipliers[i]) {
                    max = -multipliers[i];
                    enteringIndex = i;
                    break;
                }
            }
            if (enteringIndex == extraSize) {
                if (generateConstrains()) {
                    // LOGD("done");
                    break;
                }
                continue;
            }
            timer();
            // pick leavingIndex
            delta = extra.columns[enteringIndex];
            basisLU.inverse(delta);

            uint32_t leavingIndex = basisSize;
            float min = std::numeric_limits<float>::infinity();
            float deltaValue;
            for (auto entry : delta) {
                float f = solution[entry.index] / entry.value;
                if (tester.is_positive(f) || (solution[entry.index] == 0 && tester.is_positive(entry.value))) {
                    if (f < min) {
                        min = f;
                        leavingIndex = entry.index;
                        deltaValue = entry.value;
                    }
                }
            }
            if (leavingIndex == basisSize) {
                LOGD("unbound");
                return;
            }

            solution.add(delta, -min);
            solution[leavingIndex] = min;

            // swap
            std::swap(basisObjective[leavingIndex], extraObjective[enteringIndex]);
            std::swap(basis.columns[leavingIndex], extra.columns[enteringIndex]);

            // update multipliers
            deltaDual.clear();
            deltaDual.pushBack({ max / deltaValue, leavingIndex });
            timer();
            basisLU.inverseTranspose(deltaDual);
            timer();
            //dual.add(deltaDual, 1.0f);
            SparseOps::multiply(deltaMultipliers, deltaDual, extra);
            multipliers[enteringIndex] = 0.0f;
            dirtyMultipliers[enteringIndex] = false;
            multipliers.add(deltaMultipliers, 1.0f);
            for (auto entry : deltaMultipliers) {
                dirtyMultipliers[entry.index] = tester.is_positive(-multipliers[entry.index]);
            }

            timer();
            uint32_t luIndex = basisLU.p[leavingIndex];
            basisLU.u.columns[luIndex] = basis.columns[leavingIndex];
            basisLU.replaceColumn(luIndex);
            // basisLU.u = basis;
            // basisLU.decompose();
        }
    }

    DenseVector<float, uint32_t> dual;
    bool generateConstrains() {
        ZeroTester tester;
        dual.resize(basisSize);
        basisLU.inverseTranspose(dual, basisObjective);

        std::pair<uint32_t, float> entries[n];
        bool allGood = true;
        for (uint32_t i = 0; i < numConstraints; i++) {
            for (uint32_t j = 0; j < n; j++) {
                entries[j].first = constraints[i * n + j];
                entries[j].second = entries[j].first < basisSize ? dual[entries[j].first] : 0.0f;
            }
            std::sort(entries, entries + n,
                [](const std::pair<uint32_t, float>& a, const std::pair<uint32_t, float>& b) {
                    return a.second < b.second;
                });
            uint32_t lastFailure = n;
            float sum = 0.0f;
            float maxDiff = 0.0f;
            for (uint32_t j = 0; j < n; j++) {
                sum += entries[j].second;
                float diff = 0.5f * j * (j + 1) - sum;
                if (tester.is_positive(diff - maxDiff)) {
                    maxDiff = diff;
                    lastFailure = j;
                }
            }
            if (lastFailure == n)
                continue;

            extraSize++;
            typename Matrix::Column& col = extra.columns.emplace_back();
            col.reserve(lastFailure);
            for (uint32_t k = 0; k <= lastFailure; k++) {
                col.pushBack({ 1.0f, entries[k].first });
            }
            col.sort();
            multipliers.resize(extraSize);
            multipliers[extraSize - 1] = SparseOps::dot(dual, col) - 0.5f * lastFailure * (lastFailure + 1);
            dirtyMultipliers.resize(extraSize);
            dirtyMultipliers[extraSize - 1] = tester.is_positive(-multipliers[extraSize - 1]);
            extraObjective.resize(extraSize);
            extraObjective[extraSize - 1] = 0.5f * lastFailure * (lastFailure + 1);
            allGood = false;
            // break;
        }
        return allGood;
    }
};
