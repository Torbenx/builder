#pragma once

#include "Array.hpp"
#include "util/types.hpp"
#include <algorithm>

namespace stx {

template<typename T, int_t N>
class InlineVector;

template<typename T>
class Vector {
protected:
    T* m_data = nullptr;
    uint32_t m_size : 31 = 0;
    bool m_cantMove : 1 = false;
    uint32_t m_capacity : 31 = 0;
    bool m_cantDealloc : 1 = false;

    constexpr void maybeDeallocateStorage() {
        if (m_cantDealloc || !data())
            return;
        std::allocator<T> alloc;
        alloc.deallocate(data(), capacity());
    }
    constexpr void copyFields(const Vector& other) {
        m_data = other.m_data;
        m_size = other.m_size;
        m_cantMove = other.m_cantMove;
        m_capacity = other.m_capacity;
        m_cantDealloc = other.m_cantDealloc;
    }
    constexpr void clearFields() {
        m_data = nullptr;
        m_size = 0;
        m_cantMove = false;
        m_capacity = 0;
        m_cantDealloc = false;
    }
    constexpr void moveFrom(Vector&& other) {
        clear();
        if (other.m_cantMove) {
            reserve(other.size());
            // std::uninitialized_move_n(other.data(), other.size(), data());
            std::copy_n(other.data(), other.size(), data());
            m_size = other.size();
            other.clear();
        } else {
            copyFields(other);
            other.clearFields();
        }
    }
    constexpr void copyFrom(ArrayView<const T> input) {
        clear();
        reserve(input.size());
        // std::uninitialized_copy_n(input.data(), input.size(), data());
        std::copy_n(input.data(), input.size(), data());
        m_size = input.size();
    }

public:
    constexpr Vector() = default;
    constexpr Vector(int_t size, const T& init) {
        resize(size, init);
    }
    explicit constexpr Vector(int_t size) {
        resize(size);
    }
    constexpr Vector(const Vector& other) { copyFrom(other); }
    constexpr Vector(Vector&& other) { moveFrom(std::move(other)); }
    constexpr Vector(std::initializer_list<T> init) { copyFrom(init); }
    constexpr Vector(T* data, int_t size, int_t capacity, bool cantMove, bool cantDealloc)
        : m_data(data), m_size(size), m_cantMove(cantMove), m_capacity(capacity), m_cantDealloc(cantDealloc) { }

    constexpr ~Vector() {
        // std::destroy_n(data(), size());
        maybeDeallocateStorage();
    }

    constexpr Vector& operator=(const Vector& other) {
        copyFrom(other);
        return *this;
    }
    constexpr Vector& operator=(Vector&& other) {
        moveFrom(std::move(other));
        return *this;
    }

    constexpr T& operator[](int_t offset) { return data()[offset]; }
    constexpr const T& operator[](int_t offset) const { return data()[offset]; }
    constexpr T& front() { return (*this)[0]; }
    constexpr const T& front() const { return (*this)[0]; }
    constexpr T& back() { return (*this)[size() - 1]; }
    constexpr const T& back() const { return (*this)[size() - 1]; }
    constexpr T* data() { return m_data; }
    constexpr const T* data() const { return m_data; }
    constexpr T* begin() { return data(); }
    constexpr const T* begin() const { return data(); }
    constexpr T* end() { return data() + size(); }
    constexpr const T* end() const { return data() + size(); }
    constexpr int_t size() const { return m_size; }
    constexpr int_t capacity() const { return m_capacity; }
    constexpr bool empty() const { return size() == 0; }

    constexpr void reserve(int_t newCap) {
        if (newCap <= capacity())
            return;
        std::allocator<T> alloc;
        T* newStorage = alloc.allocate(newCap);
        // std::uninitialized_move_n(data(), size(), newStorage);
        std::copy_n(data(), size(), newStorage);
        std::destroy_n(data(), size());
        maybeDeallocateStorage();
        m_data = newStorage;
        m_capacity = newCap;
        m_cantMove = false;
        m_cantDealloc = false;
    }

    constexpr void clear() { truncate(0); }

    constexpr void resize(int_t newSize) { resize(newSize, {}); }
    constexpr void resize(int_t newSize, const T& init) {
        if (newSize == size())
            return;
        if (newSize < size())
            truncate(newSize);
        else
            append(init, newSize - size());
    }

    constexpr void truncate(int_t newSize) {
        // std::destroy_n(data() + newSize, size() - newSize);
        m_size = newSize;
    }

    constexpr void append(const T& value, int_t count) {
        reserve(size() + count);
        std::uninitialized_fill_n(end(), count, value);
        m_size += count;
    }

    constexpr void pushBack(const T& value) {
        reserve(size() + 1);
        std::construct_at(end(), value);
        m_size += 1;
    }

    constexpr operator ArrayView<T>() { return { data(), size() }; }
    constexpr operator ArrayView<const T>() const { return { data(), size() }; }
};

template<typename T, int_t N>
class InlineVector : public Vector<T> {
    Array<T, N> storage = {};

public:
    constexpr InlineVector()
        : Vector<T>(storage.elements, 0, N, true, true) {
        if (storage.data() == this->data())
            delete (int*)nullptr;
    }
    constexpr InlineVector(const InlineVector& other)
        : InlineVector() {
        this->copyFrom(other);
    }
    constexpr InlineVector(InlineVector&& other)
        : InlineVector() {
        this->moveFrom(std::move(other));
    }
};

namespace detail {
    template<typename T, int_t N>
    struct ArrayWrapper {
        using value_type = T;
        Array<T, N> storage = {};
        int_t size = 0;
        constexpr ArrayWrapper(const InlineVector<T, N>& v) {
            std::copy_n(v.data(), v.size(), storage.data());
            size = v.size();
        }
    };
}

template<detail::ArrayWrapper arg>
struct StaticStorage {
    using T = typename decltype(arg)::value_type;
    static constexpr Array<T, arg.size> storage = []() constexpr {
        Array<T, arg.size> array;
        std::copy_n(arg.storage.data(), arg.size, array.data());
        return array;
    }
    ();
    constexpr operator const Vector<T>() const { return { const_cast<T*>(storage.data()), storage.size(), storage.size(), false, true }; }
};

}
