#pragma once

#include "math/solver/LUDecomposition.hpp"

template<class ZeroTester = BasicZeroTester>
class LinearProgramming {
public:
    using Matrix = SparseMatrix<float, uint32_t, ZeroTester>;
    LUPDecomposition<float, uint32_t, ZeroTester> basisLU;
    Matrix basis, extra;
    DenseVector<float, uint32_t> solution, multipliers, bound, basisObjective, extraObjective, dual;
    uint32_t basisSize = 0, extraSize = 0, n, numConstraints = 0;
    std::vector<uint32_t> constraints, output;
    SparseVector<float, uint32_t, ZeroTester> delta, delta2, deltaDual, deltaMultipliers;
    BitSet dirtyMultipliers;

    LinearProgramming(uint32_t n)
        : n(n) { }

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
    }

    void addConstraint(uint32_t* constraint) {
        size_t offset = constraints.size();
        constraints.resize(offset + n);
        numConstraints++;
        std::memcpy(&constraints[offset], constraint, n * sizeof(uint32_t));
        addConstraint_internal(constraint, n);
    }

    void addConstraint_internal(uint32_t* constraint, uint32_t m) {
        ZeroTester tester;
        extraSize++;
        typename Matrix::Column& col = extra.columns.emplace_back();
        col.reserve(m);
        for (uint32_t k = 0; k < m; k++) {
            col.pushBack({ 1.0f, constraint[k] });
        }
        col.sort();
        multipliers.resize(extraSize);
        multipliers[extraSize - 1] = SparseOps::dot(dual, col) - 0.5f * m * (m - 1);
        dirtyMultipliers.setBitTo(extraSize - 1, tester.is_positive(-multipliers[extraSize - 1]));
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
                    // LOGD("done");
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

            uint32_t constraint[lastFailure + 1];
            for (uint32_t j = 0; j <= lastFailure; j++) {
                constraint[j] = entries[j].first;
            }
            addConstraint_internal(constraint, lastFailure + 1);

            allGood = false;
            // break;
        }
        return allGood;
    }
};
