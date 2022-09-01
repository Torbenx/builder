#pragma once

#include "Matrix.hpp"

namespace sparse {

struct Inverse {
public:
    Matrix inverse, inverseTranspose;
    void expandPreInverted(Vector invColumn, Vector invRow, float invF) {
        VERIFY(Sparse, std::abs(invF) < 1.0f / EPSILON);
        uint32_t newIndex = inverse.size();
        inverse.forEachColumn(invRow, [&](Vector& oldColumn, Entry entry) {
            Vector newColumn = add(oldColumn, invColumn, entry.value * invF, 1);
            newColumn.appendEntry({ newIndex, -entry.value * invF });
            oldColumn = std::move(newColumn);
        });

        inverseTranspose.forEachColumn(invColumn, [&](Vector& oldRow, Entry entry) {
            Vector newRow = add(oldRow, invRow, entry.value * invF, 1);
            newRow.appendEntry({ newIndex, -entry.value * invF });
            oldRow = std::move(newRow);
        });

        scalarMultiplyInplace(invColumn, -invF);
        invColumn.appendEntry({ newIndex, invF });
        inverse.appendColumn(std::move(invColumn));

        scalarMultiplyInplace(invRow, -invF);
        invRow.appendEntry({ newIndex, invF });
        inverseTranspose.appendColumn(std::move(invRow));
    }

    Vector preInvertColumn(VectorView column) {
        return inverse.multiply(column, 6);
    }
    Vector preInvertRow(VectorView row) {
        return inverseTranspose.multiply(row, 6);
    }
    float preInvertCornerWithColumn(VectorView invColumn, VectorView row, float corner) {
        return 1.0f / (corner - dot(row, invColumn));
    }
    float preInvertCornerWithRow(VectorView column, VectorView invRow, float corner) {
        return 1.0f / (corner - dot(invRow, column));
    }
    void expand(VectorView column, VectorView row, float corner) {
        Vector invColumn = preInvertColumn(column);
        Vector invRow = preInvertRow(row);
        float invF = preInvertCornerWithColumn(invColumn, row, corner);
        expandPreInverted(std::move(invColumn), std::move(invRow), invF);
    }

    void replaceColumnPreInverted(uint32_t columnIndex, Vector invColumn) {
        replaceColumnPreInvertedImpl(inverse, inverseTranspose, columnIndex, std::move(invColumn));
    }
    void replaceColumn(uint32_t columnIndex, VectorView column) {
        Vector invColumn = preInvertColumn(column);
        replaceColumnPreInverted(columnIndex, std::move(invColumn));
    }
    void replaceRowPreInverted(uint32_t rowIndex, Vector invRow) {
        replaceColumnPreInvertedImpl(inverseTranspose, inverse, rowIndex, std::move(invRow));
    }
    void replaceRow(uint32_t rowIndex, VectorView row) {
        Vector invRow = preInvertRow(row);
        replaceRowPreInverted(rowIndex, std::move(invRow));
    }

private:
    static void replaceColumnPreInvertedImpl(Matrix& inv, Matrix& invTrans, uint32_t columnIndex, Vector invColumn) {
        float invF;
        {
            int_t offset = invColumn.lowerBound(columnIndex);
            Entry& entry = invColumn[offset];
            VERIFY(Sparse, entry.index == columnIndex);
            invF = 1.0f / entry.value;
            entry.value -= 1.0f;
        }
        Vector& row = invTrans[columnIndex];

        inv.forEachColumn(row, [&](Vector& oldColumn, Entry entry) {
            oldColumn = add(oldColumn, invColumn, -entry.value * invF);
        });

        invTrans.forEachColumn(invColumn, [&](Vector& oldRow, Entry entry) {
            if (entry.index == columnIndex)
                return;
            oldRow = add(oldRow, row, -entry.value * invF);
        });
        scalarMultiplyInplace(row, invF);
    }
};

}