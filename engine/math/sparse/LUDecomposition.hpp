#pragma once

#include "Vector.hpp"

namespace sparse {

class Permutation {
public:
    uint32_t operator[](uint32_t in) const {
        return map[in];
    }

    void setIdentity(int_t size) {
        map.resize(size);
        inv.resize(size);
        for (int_t i = 0; i < size; i++) {
            map[i] = i;
            inv[i] = i;
        }
    }

    uint32_t inverse(uint32_t in) const {
        return inv[in];
    }

    void swap(uint32_t in1, uint32_t in2) {
        // map[inv[in1]] => in1;
        // map[inv[in2]] => in2;

        std::swap(inv[in1], inv[in2]);

        // map[inv[in1]] => in2;
        // map[inv[in2]] => in1;

        map[inv[in1]] = in1;
        map[inv[in2]] = in2;
    }

    int_t size() const {
        return map.size();
    }

    void expandBy(int_t increase) {
        int_t oldSize = size();
        map.resize(oldSize + increase);
        inv.resize(oldSize + increase);
        for (int_t i = oldSize; i < oldSize + increase; i++) {
            map[i] = i;
            inv[i] = i;
        }
    }

private:
    std::vector<uint32_t> map, inv;
};

struct LUDecomposition {
    Matrix l, u;
    Permutation p;
    std::vector<uint32_t> rowIndices;
    int_t m_rank = 0;

    int_t size() const { return u.columns.size(); }
    int_t rank() const { return m_rank; }

    void inverseTranspose(const float* in, float* out) const {
        for (int_t i = 0; i < rank(); i++)
            out[rowIndices[i]] = in[p.inverse(i)];
        for (int_t i = 0; i < rank(); i++) {
            uint32_t rowIndex = rowIndices[i];
            float dot = 0.0f;
            for (int_t j = 0; j < u[i].size() - 1; j++) {
                Entry e = u[i][j];
                dot += e.value * out[e.index];
            }
            out[rowIndex] = (out[rowIndex] - dot) / u[i].back().value;
        }
        for (int_t i = rank() - 1; i >= 0; i--)
            out[rowIndices[i]] -= dot(l[i], out);
    }

    void decompose(int_t maxIndex) {
        l.clear();
        p.setIdentity(size());
        int_t outputColumn = 0;
        std::vector<uint32_t> columnOffsets(size());
        for (int_t rowIndex = 0; rowIndex < maxIndex; rowIndex++) {

            int_t pivotColumn = size();
            for (int_t i = outputColumn; i < size(); i++) {
                if (u[i][columnOffsets[i]].index == rowIndex) {
                    pivotColumn = i;
                    break;
                }
            }
            if (pivotColumn == size())
                continue;
            rowIndices.push_back(rowIndex);

            if (pivotColumn != outputColumn) {
                VERIFY(Sparse, pivotColumn > outputColumn);
                p.swap(pivotColumn, outputColumn);
                std::swap(u[pivotColumn], u[outputColumn]);
                std::swap(columnOffsets[pivotColumn], columnOffsets[outputColumn]);
            }

            uint32_t diagOffset = columnOffsets[outputColumn];
            Entry diagEntry = u[outputColumn][diagOffset];
            VERIFY(Sparse, diagEntry.index == rowIndex);
            float invDiag = 1.0f / diagEntry.value;
            VectorView pivot = u[outputColumn].subView(diagOffset + 1);

            // l[column,j] = u[column,j] / u[column,row] ; j > row
            VERIFY(Sparse, (int_t)l.size() == outputColumn);
            l.appendColumn(scalarMultiplyOutOfPlace(pivot, invDiag));

            // u[i,j] -= u[i,row] * u[column,j] / u[column,row] ; i > column, j > row
            for (int_t i = outputColumn + 1; i < size(); i++) {
                Entry entry = u[i][columnOffsets[i]];
                if (entry.index != rowIndex) {
                    VERIFY(Sparse, entry.index > rowIndex);
                    continue;
                }
                if (pivot.size() > 0)
                    u[i] = add(u[i], pivot, -entry.value * invDiag);
                columnOffsets[i] += 1;
            }

            u[outputColumn].resizeTo(diagOffset + 1);
            outputColumn += 1;
        }
        m_rank = outputColumn;
        VERIFY(Sparse, l.size() == rank());
        VERIFY(Sparse, (int_t)rowIndices.size() == rank());
    }
};

}