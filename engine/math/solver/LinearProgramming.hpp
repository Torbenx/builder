#pragma once

#include "math/solver/LUDecomposition.hpp"
#include <cmath>
#include <cstring>

template<uint32_t N, class ZeroTester = BasicZeroTester>
class LinearProgramming {
public:
    using Matrix = SparseMatrix<float, uint32_t, ZeroTester>;
    LUPDecomposition<float, uint32_t, ZeroTester> basisLU;
    Matrix basis, extra;
    DenseVector<float, uint32_t> solution, multipliers, bound, basisObjective, extraObjective, dual;
    uint32_t basisSize = 0, extraSize = 0, numConstraints = 0;
    std::vector<uint32_t> constraints, output;
    SparseVector<float, uint32_t, ZeroTester> delta, delta2, deltaDual, deltaMultipliers;
    BitSet dirtyMultipliers;

    void addVariable() {
        basisSize++;
        basis.columns.emplace_back().pushBack({ 1.0f, basisSize - 1 });
        bound.resize(basisSize);
        bound[basisSize - 1] = 1.0f;
        solution.resize(basisSize);
        solution[basisSize - 1] = 1.0f;
        basisLU.identityExpandByOne();
        basisObjective.resize(basisSize);
        output.resize(basisSize);
        dual.resize(basisSize);
    }

    void addConstraint(uint32_t* constraint) {
        ZeroTester tester;
        size_t offset = constraints.size();
        constraints.resize(offset + N);
        numConstraints++;
        std::memcpy(&constraints[offset], constraint, N * sizeof(uint32_t));
        float multiplier = -0.5f * N * (N - 1);
        for (uint32_t i = 0; i < N; i++) {
            multiplier += dual[constraint[i]];
        }
        addConstraint_internal(constraint, N, multiplier);
        dirtyMultipliers.setBitTo(extraSize - 1, tester.is_positive(-multiplier));
    }

    template<typename T>
    void addConstraint_internal(T&& constraint, uint32_t m, float multiplier) {
        extraSize++;
        typename Matrix::Column& col = extra.columns.emplace_back();
        col.reserve(m);
        for (uint32_t k = 0; k < m; k++) {
            col.pushBack({ 1.0f, constraint[k] });
        }
        col.sort();
        multipliers.resize(extraSize);
        multipliers[extraSize - 1] = multiplier;
        extraObjective.resize(extraSize);
        extraObjective[extraSize - 1] = 0.5f * m * (m - 1);
    }

    void solve() {
        relaxed();
        for (uint32_t i = 0; i < basisSize; i++) {
            output[i] = std::ceil(dual[i]);
        }
    }

    void relaxed() {
        ZeroTester tester;

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

            // pick enteringIndex
            uint32_t enteringIndex = dirtyMultipliers.nextSetBit(0, extraSize);
            if (enteringIndex == extraSize) {
                if (generateConstrains()) {
                    break;
                }
                continue;
            }

            // pick leavingIndex
            delta = extra.columns[enteringIndex];
            basisLU.inverseL(delta);
            delta2 = delta;
            basisLU.inverse_internal(delta);

            uint32_t leavingIndex = basisSize;
            float min = std::numeric_limits<float>::infinity();
            float deltaValue;
            for (auto entry : delta) {
                if (tester.is_positive(entry.value)) {
                    float f = solution[entry.index] / entry.value;
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
            deltaDual.pushBack({ -multipliers[enteringIndex] / deltaValue, leavingIndex });
            basisLU.inverseTranspose(deltaDual);
            //dual.add(deltaDual, 1.0f);
            SparseOps::multiply(deltaMultipliers, deltaDual, extra);
            multipliers[enteringIndex] = 0.0f;
            dirtyMultipliers.clearBit(enteringIndex);
            multipliers.add(deltaMultipliers, 1.0f);
            for (auto entry : deltaMultipliers) {
                dirtyMultipliers.setBitTo(entry.index, tester.is_positive(-multipliers[entry.index]));
            }

            uint32_t luIndex = basisLU.p[leavingIndex];
            std::swap(basisLU.u.columns[luIndex], delta2);
            basisLU.replaceColumn_internal(luIndex);
            // basisLU.u = basis;
            // basisLU.decompose();
        }
    }

    struct ConstraintEntry {
        uint32_t index;
        float value;
        operator uint32_t() { return index; }
    };

    bool generateConstrains() {
        ZeroTester tester;
        dual.resize(basisSize);
        basisLU.inverseTranspose(dual, basisObjective);

        ConstraintEntry entries[N];
        bool allGood = true;
        for (uint32_t i = 0; i < numConstraints; i++) {
            for (uint32_t j = 0; j < N; j++) {
                entries[j].index = constraints[i * N + j];
                entries[j].value = dual[entries[j].index];
            }
            std::sort(entries, entries + N,
                [](const ConstraintEntry& a, const ConstraintEntry& b) {
                    return a.value < b.value;
                });
            uint32_t lastFailure = N;
            float sum = 0.0f;
            float maxDiff = 0.0f;
            for (uint32_t j = 0; j < N; j++) {
                sum += entries[j].value;
                float diff = 0.5f * j * (j + 1) - sum;
                if (tester.is_positive(diff - maxDiff)) {
                    maxDiff = diff;
                    lastFailure = j;
                }
            }
            if (lastFailure == N)
                continue;

            addConstraint_internal((ConstraintEntry*)entries, lastFailure + 1, -maxDiff);
            dirtyMultipliers.setBit(extraSize - 1);

            allGood = false;
            // break;
        }
        return allGood;
    }
};
