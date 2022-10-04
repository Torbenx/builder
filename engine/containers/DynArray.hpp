#pragma once

#include "ArrayView.hpp"

namespace stx {

template<typename T>
class DynArray {
private:
    T* m_data = nullptr;
    int_t m_size = 0;

    constexpr void allocate(int_t size) {
        if (size != 0) {
            m_data = std::allocator<T>().allocate(size);
            m_size = size;
        } else {
            clearFields();
        }
    }
    constexpr void allocateAndInitialize(int_t size) {
        allocate(size);
        for (int_t i = 0; i < size; i++)
            new (data() + i) T {};
    }
    constexpr void destroyAndDeallocate() {
        if (data()) {
            for (int_t i = 0; i < size(); i++)
                std::destroy_n(data(), size());
            std::allocator<T>().deallocate(data(), size());
        }
    }
    void clearFields() {
        m_data = nullptr;
        m_size = 0;
    }

public:
    using value_type = T;
    using iterator = T*;
    using size_type = int_t;

    constexpr DynArray() = default;
    constexpr DynArray(int_t size) {
        allocateAndInitialize(size);
    }
    constexpr DynArray(const DynArray& other) {
        allocate(other.size());
        std::uninitialized_copy_n(other.data(), other.size(), data());
    }
    constexpr DynArray(DynArray&& other) {
        m_data = other.data();
        m_size = other.size();
        other.clearFields();
    }
    constexpr ~DynArray() {
        destroyAndDeallocate();
    }

    constexpr DynArray& operator=(const DynArray& other) {
        if (size() == other.size()) {
            std::copy_n(other.data(), other.size(), data());
        } else {
            destroyAndDeallocate();
            allocate(other.size());
            std::uninitialized_copy_n(other.data(), other.size(), data());
        }
        return *this;
    }
    constexpr DynArray& operator=(DynArray&& other) {
        destroyAndDeallocate();
        m_data = other.data();
        m_size = other.size();
        other.clearFields();
        return *this;
    }

    constexpr T* data() { return m_data; }
    constexpr const T* data() const { return m_data; }
    constexpr T* begin() { return data(); }
    constexpr const T* begin() const { return data(); }
    constexpr T* end() { return data() + size(); }
    constexpr const T* end() const { return data() + size(); }
    constexpr int_t size() const { return m_size; }
    constexpr bool empty() const { return m_size > 0; }
    constexpr T& operator[](int_t i) { return data()[i]; }
    constexpr const T& operator[](int_t i) const { return data()[i]; }
    constexpr T& front() { return data()[0]; }
    constexpr const T& front() const { return data()[0]; }
    constexpr T& back() { return data()[size() - 1]; }
    constexpr const T& back() const { return data()[size() - 1]; }
    constexpr operator ArrayView<T>() { return { data(), size() }; }
    constexpr operator ArrayView<const T>() const { return { data(), size() }; }

    constexpr void clear() {
        destroyAndDeallocate();
        clearFields();
    }
};

}