#pragma once

#include <bit>
#include <limits>

template<typename T>
struct BitTraits {
    static constexpr int logDigits = 0;
};

template<>
struct BitTraits<uint8_t> { static constexpr int logDigits = 3; };
template<>
struct BitTraits<uint16_t> { static constexpr int logDigits = 4; };
template<>
struct BitTraits<uint32_t> { static constexpr int logDigits = 5; };
template<>
struct BitTraits<uint64_t> { static constexpr int logDigits = 6; };

template<typename T>
class BitSetBase {
public:
    BitSetBase() { }
    BitSetBase(BitSetBase&& other) { *this = other; }
    BitSetBase(const BitSetBase& other) { *this = other; }

    BitSetBase& operator=(BitSetBase&& other) {
        m_data = other.m_data;
        m_size = other.m_size;
        other.m_data = nullptr;
        other.m_size = 0;
        return *this;
    }

    BitSetBase& operator=(const BitSetBase& other) {
        m_size = other.m_size;
        m_data = new T[m_size];
        std::memcpy(m_data, other.m_data, m_size * sizeof(T));
        return *this;
    }

    bool getUnchecked(size_t i) const {
        T mask = (T)1 << (i & (std::numeric_limits<T>::digits - 1));
        i >>= BitTraits<T>::logDigits;
        return (m_data[i] & mask) != 0;
    }

    void setBitUnchecked(size_t i) {
        T mask = (T)1 << (i & (std::numeric_limits<T>::digits - 1));
        i >>= BitTraits<T>::logDigits;
        m_data[i] |= mask;
    }

    void clearBitUnchecked(size_t i) {
        T mask = (T)1 << (i & (std::numeric_limits<T>::digits - 1));
        i >>= BitTraits<T>::logDigits;
        m_data[i] &= ~mask;
    }

    bool get(size_t i) const {
        T mask = (T)1 << (i & (std::numeric_limits<T>::digits - 1));
        i >>= BitTraits<T>::logDigits;
        return i < m_size && (m_data[i] & mask) != 0;
    }

    void setBit(size_t i) {
        T mask = (T)1 << (i & (std::numeric_limits<T>::digits - 1));
        i >>= BitTraits<T>::logDigits;
        if (i >= m_size)
            growAndClear(i + 1);
        m_data[i] |= mask;
    }

    void clearBit(size_t i) {
        T mask = (T)1 << (i & (std::numeric_limits<T>::digits - 1));
        i >>= BitTraits<T>::logDigits;
        if (i < m_size)
            m_data[i] &= ~mask;
    }

    BitSetBase& operator|=(const BitSetBase<T>& other) {
        size_t size = other.m_size;
        if (size > m_size) {
            size = m_size;
            grow(other.m_size);
            std::memcpy(m_data + size, other.m_data, (m_size - size) * sizeof(T));
        }
        for (size_t i = 0; i < size; i++) {
            m_data[i] |= other.m_data[i];
        }
        return *this;
    }

    BitSetBase& operator&=(const BitSetBase<T>& other) {
        size_t size = m_size;
        if (other.m_size < size) {
            std::memset(m_data + other.m_size, 0, (m_size - other.m_size) * sizeof(T));
            size = other.m_size;
        }
        for (size_t i = 0; i < size; i++) {
            m_data[i] &= other.m_data[i];
        }
        return *this;
    }

    size_t popcount() const {
        size_t count = 0;
        for (size_t i = 0; i < m_size; i++) {
            count += std::popcount(m_data[i]);
        }
        return count;
    }

    void clearAfter(size_t i) {
        i++;
        T mask = (T)1 << (i & (std::numeric_limits<T>::digits - 1));
        i >>= BitTraits<T>::logDigits;
        if (i >= m_size)
            return;
        m_data[i] &= mask - 1;
        std::memset(m_data + i + 1, 0, (m_size - i - 1) * sizeof(T));
    }

    void clear() {
        std::memset(m_data, 0, m_size * sizeof(T));
    }

    static bool shareAny(const BitSetBase<T>& a, const BitSetBase<T>& b) {
        size_t size = std::min(a.m_size, b.m_size);
        T ret = 0;
        for (size_t i = 0; i < size; i++) {
            ret |= a.m_data[i] & b.m_data[i];
        }
        return ret != 0;
    }

    ~BitSetBase() {
        delete[] m_data;
    }

private:
    void grow(size_t size) {
        T* newData = new T[size];
        std::memcpy(newData, m_data, m_size * sizeof(T));
        delete[] m_data;
        m_data = newData;
        m_size = size;
    }

    void growAndClear(size_t size) {
        size_t oldSize = m_size;
        grow(size);
        std::memset(m_data + oldSize, 0, (size - oldSize) * sizeof(T));
    }

    T* m_data = nullptr;
    size_t m_size = 0;
};

template<typename T>
void swap(BitSetBase<T>& a, BitSetBase<T>& b) {
    std::swap(a.m_data, b.m_data);
    std::swap(a.m_size, b.m_size);
}

typedef BitSetBase<uint64_t> BitSet;
