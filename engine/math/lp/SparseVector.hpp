#pragma once

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
        int_t offset = 0;
        uint32_t index;
        do {
            Entry e = (*this)[offset++];
            index = e.index;
            log<Tag::Sparse>("{{ {}, {} }}", e.index, e.value);
        } while (index != MAX_INDEX);
    }

    Vector copy(int_t extraSize = 0) const;
};

class Vector : public VectorView {
public:
    Vector() { }

    Vector(int_t size) {
        constexpr int_t space_size = 256 * (1 << 20);
        static char* space = new char[space_size];
        static int_t offset;
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

    ~Vector() {
        release();
    }
};

inline Vector VectorView::copy(int_t extraSize) const {
    Vector r = Vector(size() + extraSize);
    std::copy(entries, entries + size() + 1, r.entries);
    r.setSize(size());
    return r;
}

Vector allocate(int_t size) {
    return Vector(size);
}

Vector allocateEmpty(int_t extraSize = 0) {
    Vector r = allocate(extraSize);
    r[0] = END_ENTRY;
    return r;
}

void add(Vector& out, VectorView v1, VectorView v2, float factor = 1.0f) {
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
    out.setSize(offset3);
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

void scalarMultiplyBy(Vector& v, float f) {
    int_t offset = 0;
    uint32_t index;
    do {
        Entry& e = v[offset++];
        index = e.index;
        e.value *= f;
    } while (index != MAX_INDEX);
}

struct Matrix {
    std::vector<Vector> m_columns;

    template<typename Callback>
    void columns(VectorView row, Callback&& callback) {
        for (int_t i = 0; i < row.size(); i++) {
            Entry entry = row[i];
            callback(m_columns[entry.index], entry);
        }
    }

    Vector multiply(VectorView v, int_t extraSpace = 0) {
        Vector ret = allocateEmpty(extraSpace);
        columns(v, [&](VectorView column, Entry entry) {
            Vector tmp = allocate(ret.size() + column.size() + extraSpace);
            add(tmp, ret, column, entry.value);
            ret = std::move(tmp);
        });
        return ret;
    }

    void appendColumn(uint32_t index, Vector column) {
        m_columns.emplace_back(std::move(column));
    }

    int_t size() const { return m_columns.size(); }
    Vector& operator[](int_t i) {
        return m_columns[i];
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
    Matrix inverse, inverseTranspose;
    void expandPreInverted(Vector invColumn, Vector invRow, float invF) {
        uint32_t newIndex = inverse.size();
        inverse.columns(invRow, [&](Vector& oldColumn, Entry entry) {
            Vector newColumn = allocate(invColumn.size() + oldColumn.size() + 1);
            add(newColumn, oldColumn, invColumn, entry.value * invF);
            newColumn.appendEntry({ newIndex, -entry.value * invF });
            oldColumn = std::move(newColumn);
        });

        inverseTranspose.columns(invColumn, [&](Vector& oldRow, Entry entry) {
            Vector newRow = allocate(invRow.size() + oldRow.size() + 1);
            add(newRow, oldRow, invRow, entry.value * invF);
            newRow.appendEntry({ newIndex, -entry.value * invF });
            oldRow = std::move(newRow);
        });

        scalarMultiplyBy(invColumn, -invF);
        invColumn.appendEntry({ newIndex, invF });
        inverse.appendColumn(newIndex, std::move(invColumn));

        scalarMultiplyBy(invRow, -invF);
        invRow.appendEntry({ newIndex, invF });
        inverseTranspose.appendColumn(newIndex, std::move(invRow));
    }

    Vector preInvertColumn(VectorView column) {
        Vector invColumn = inverse.multiply(column, 1);
        invColumn.appendEntry({ inverse.size(), -1.0f });
        return invColumn;
    }
    Vector preInvertRow(VectorView row) {
        Vector invRow = inverseTranspose.multiply(row, 1);
        invRow.appendEntry({ inverse.size(), -1.0f });
        return invRow;
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
        float invF = preInvertCornerWithColumn(invColumn, row);
        expandPreInverted(std::move(invColumn), std::move(invRow), invF);
    }
};

}
