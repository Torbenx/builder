#pragma once

#include "util/log.hpp"
#include "util/types.hpp"

namespace Tag {
struct Sparse {
    static constexpr char tag[] = "sparse";
    static constexpr bool verifyEnabled = true;
};
};

namespace sparse {

static constexpr uint32_t MAX_INDEX = std::numeric_limits<uint32_t>::max();
static constexpr float EPSILON = 0.0001f;

struct Entry {
    uint32_t index;
    float value;
};
static constexpr Entry END_ENTRY = { MAX_INDEX, 0.0f };

class Vector;

class VectorView {
protected:
    Entry* entries = nullptr;
    int_t numEntries = 0;

    void clearFields() {
        entries = nullptr;
        numEntries = 0;
    }

public:
    VectorView() { }
    VectorView(const Entry* entries, int_t size)
        : entries(const_cast<Entry*>(entries)), numEntries(size) { }

    const Entry& operator[](int_t i) const {
        return entries[i];
    }

    int_t size() const { return numEntries; }

    void debug() const {
        if (!entries) {
            log<Tag::Sparse>("sparse::Vector {{ null }}");
            return;
        }
        log<Tag::Sparse>("sparse::Vector {{ {} }}", size());
        for (int_t i = 0; i < size(); i++) {
            Entry e = (*this)[i];
            log<Tag::Sparse>("{{ {}, {} }}", e.index, e.value);
        }
        Entry endEntry = (*this)[size()];
        if (endEntry.index != MAX_INDEX || endEntry.value != 0.0f)
            log<Tag::Sparse>("END INVALID: idx={}, val={}", endEntry.index, endEntry.value);
    }

    const Entry* begin() const { return entries; }
    const Entry* end() const { return entries + size(); }

    Vector copy(int_t extraSize = 0) const;

    int_t lowerBound(uint32_t index) const {
        Entry e0 = (*this)[0];
        Entry e1 = (*this)[1];
        Entry e2 = (*this)[2];
        Entry e3 = (*this)[3];
        int_t r = e3.index >= index ? 3 : 4;
        r = e2.index >= index ? 2 : r;
        r = e1.index >= index ? 1 : r;
        r = e0.index >= index ? 0 : r;
        return r;
        /*int_t i;
        for (i = 0;; i++) {
            if ((*this)[i].index >= index)
                break;
        }
        return i;*/
    }
    float findValue(uint32_t index) const {
        Entry e = (*this)[lowerBound(index)];
        return e.index == index ? e.value : 0.0f;
    }

    const Entry* data() const { return entries; }
};

class Vector : public VectorView {
public:
    Vector() { }

    static constexpr int_t space_size = 256 * (1 << 20);
    static inline char* space = new char[space_size];

    Vector(int_t size) {
        static int_t offset = 0;
        entries = (Entry*)(space + offset);
        offset += (size + 1) * sizeof(Entry);
        VERIFY(Sparse, offset < space_size);
        // entries = new Entry[size + 1];
    }

    Vector(Vector&& other)
        : VectorView(other) {
        other.clearFields();
    }
    Vector(const Vector&) = delete;

    Vector& operator=(Vector&& other) {
        release();
        *(VectorView*)this = other;
        other.clearFields();
        return *this;
    }
    Vector& operator=(const Vector&) = delete;

    Entry& operator[](int_t i) {
        return entries[i];
    }

    void appendEntry(Entry entry) {
        // VERIFY(Sparse, size() + 1 <= capacity());
        (*this)[size()] = entry;
        resizeTo(size() + 1);
    }

    void release() {
        // delete[] entries;
        clearFields();
    }

    void shrikeBy(int_t delta) {
        resizeTo(size() - delta);
    }

    void resizeTo(int_t newSize) {
        (*this)[newSize] = END_ENTRY;
        setSize(newSize);
    }

    void setSize(int_t newSize) {
        numEntries = newSize;
    }

    Entry* data() { return entries; }

    void add(Entry newEntry) {
        int_t offset = lowerBound(newEntry.index);
        Entry& entry = (*this)[offset];
        if (entry.index == newEntry.index) {
            entry.value += newEntry.value;
            return;
        }
        std::copy_backward(data() + offset, data() + size() + 1, data() + size() + 2);
        entry = newEntry;
        setSize(size() + 1);
    }

    void removeAt(int_t offset) {
        std::copy(data() + offset + 1, data() + size() + 1, data() + offset);
        setSize(size() - 1);
    }

    void removeIndex(uint32_t index) {
        int_t offset = lowerBound(index);
        if ((*this)[offset].index == index)
            removeAt(offset);
    }

    void sort() {
        std::sort(data(), data() + size(),
            [](const Entry& e1, const Entry& e2) {
                return e1.index < e2.index;
            });
    }

    ~Vector() {
        release();
    }
};

inline Vector VectorView::copy(int_t extraSize) const {
    Vector r = Vector(size() + extraSize);
    std::copy(data(), data() + size() + 1, r.data());
    r.setSize(size());
    return r;
}

Vector allocate(int_t size) {
    return Vector(size);
}

Vector allocateEmpty(int_t extraSize = 0) {
    Vector r = allocate(extraSize);
    r.resizeTo(0);
    return r;
}

int_t addImpl(Entry* out, const Entry* v1, const Entry* v2, float factor) {
    int_t offset1 = 0;
    int_t offset2 = 0;
    int_t offset3 = 0;

    uint32_t i3;
    float r = 0.0f;
    do {
        if (std::abs(r) > EPSILON)
            offset3 += 1;

        Entry e1 = v1[offset1];
        Entry e2 = v2[offset2];

        uint32_t i1 = e1.index;
        uint32_t i2 = e2.index;

        offset1 += i1 <= i2 ? 1 : 0;
        offset2 += i1 >= i2 ? 1 : 0;
        i3 = i1 <= i2 ? i1 : i2;
        uint32_t f1 = i1 <= i2 ? __builtin_bit_cast(uint32_t, e1.value) : 0;
        uint32_t f2 = i1 >= i2 ? __builtin_bit_cast(uint32_t, e2.value) : 0;

        r = __builtin_bit_cast(float, f1) + factor * __builtin_bit_cast(float, f2);

        out[offset3] = { i3, r };
    } while (i3 != MAX_INDEX);
    out[offset3] = END_ENTRY;
    return offset3;
}

Vector add(VectorView v1, VectorView v2, float factor = 1.0f, int_t extraSize = 0) {
    Vector out = allocate(v1.size() + v2.size() + extraSize);
    int_t size = addImpl(out.data(), v1.data(), v2.data(), factor);
    out.setSize(size);
    return out;
}

void addTo(float* dense, VectorView v, float factor = 1.0f) {
    for (int_t i = 0; i < v.size(); i++) {
        Entry e = v[i];
        dense[e.index] += factor * e.value;
    }
}

float dot(VectorView v1, VectorView v2) {
    int_t offset1 = 0;
    int_t offset2 = 0;
    uint32_t min;
    float out = 0.0f;
    do {
        Entry e1 = v1[offset1];
        Entry e2 = v2[offset2];

        uint32_t i1 = e1.index;
        uint32_t i2 = e2.index;

        offset1 += i1 <= i2 ? 1 : 0;
        offset2 += i1 >= i2 ? 1 : 0;
        min = i1 <= i2 ? i1 : i2;

        if (i1 == i2)
            out += e1.value * e2.value;
    } while (min != MAX_INDEX);
    return out;
}

template<typename Kernel>
void elementwiseKernelInPlace(Vector& v, Kernel&& kernel) {
    for (int_t offset = 0; offset < v.size(); offset++) {
        Entry& e = v[offset];
        e.value = kernel(e.value);
    }
}

template<typename Kernel>
Vector elementwiseKernelOutOfPlace(VectorView v, Kernel&& kernel, int_t extraSize = 0) {
    Vector r = allocate(v.size() + extraSize);
    for (int_t offset = 0; offset < v.size(); offset++) {
        Entry in = v[offset];
        Entry& out = r[offset];
        out.index = in.index;
        out.value = kernel(in.value);
    }
    r.resizeTo(v.size());
    return r;
}

void scalarMultiplyInplace(Vector& v, float f) {
    elementwiseKernelInPlace(v, [f](float val) { return val * f; });
}

template<typename Transform, typename Finalize>
Vector entrywiseTransformOutOfPlace(VectorView v, Transform&& transform, int_t extraSize, Finalize&& finalize) {
    Vector r = allocate(v.size() + extraSize);
    int_t rOffset = 0;
    for (int_t vOffset = 0; vOffset < v.size(); vOffset++) {
        Entry in = v[vOffset];
        Entry& out = r[rOffset];
        out = transform(in);
        rOffset += out.index != MAX_INDEX ? 1 : 0;
    }
    r.resizeTo(rOffset);
    finalize(r);
    r.sort();
    return r;
}

template<typename Transform>
Vector entrywiseTransformOutOfPlace(VectorView v, Transform&& transform, int_t extraSize = 0) {
    return entrywiseTransformOutOfPlace(v, std::forward<Transform>(transform), extraSize, [](Vector&) {});
}

struct Matrix {
    std::vector<Vector> columns;

    template<typename Callback>
    void forEachColumn(VectorView row, Callback&& callback) {
        for (int_t i = 0; i < row.size(); i++) {
            Entry entry = row[i];
            callback((*this)[entry.index], entry);
        }
    }

    Vector multiply(VectorView v, int_t extraSize = 0) {
        /*Vector ret = allocateEmpty(extraSize);
        for (Entry e : v)
            ret = add(ret, (*this)[e.index], e.value, extraSize);
        return ret;*/

        if (v.size() == 0)
            return allocateEmpty(extraSize);

        std::array<Entry, 10> scratch1;
        std::array<Entry, 10> scratch2;
        Entry* in = scratch1.data();
        Entry* out = scratch2.data();
        int_t size = 0;
        in[0] = END_ENTRY;
        for (int_t i = 0; i < v.size() - 1; i++) {
            Entry e = v[i];
            size = addImpl(out, in, (*this)[e.index].data(), e.value);
            std::swap(in, out);
        }
        Entry e = v[v.size() - 1];
        return add({ in, size }, (*this)[e.index], e.value, extraSize);
    }

    void appendColumn(Vector column) {
        columns.emplace_back(std::move(column));
    }

    int_t size() const { return columns.size(); }
    Vector& operator[](int_t i) {
        return columns[i];
    }

    void debug() {
        log<Tag::Sparse>("sparse::Matrix {{ {} }}", size());
        for (int_t i = 0; i < size(); i++) {
            log<Tag::Sparse>("column {}:", i);
            (*this)[i].debug();
        }
    }
};

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
