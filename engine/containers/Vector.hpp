#pragma once

namespace stx {

template<typename T>
class Vector {
private:
    T* m_data = nullptr;
    int_t m_size = 0;
    size_t m_capacity : 63 = 0;
    bool m_inline : 1 = false;

    constexpr T* allocateStorage(int_t size) { return new T[size]; }
    constexpr void freeStorage(T* storage, int_t size) { delete[] storage; }
    constexpr void copyFields(const Vector& other) {
        m_data = other.m_data;
        m_size = other.m_size;
        m_capacity = other.m_capacity;
        m_inline = other.m_inline;
    }
    constexpr void clearFields() {
        m_data = nullptr;
        m_size = 0;
        m_capacity = 0;
        m_inline = false;
    }
    constexpr void moveFrom(Vector&& other) {
        copyFields(other);
        other.clearFields();
    }
    template<typename InputIt>
    constexpr void initializeStorage(T* storage, InputIt begin, InputIt end) {
        std::uninitialized_copy(begin, end, storage);
    }

public:
    constexpr Vector(int_t size, const T& init) {
        resize(size, init);
    }
    explicit constexpr Vector(int_t size) {
        resize(size);
    }
    constexpr Vector(const Vector& other) { }
    constexpr Vector(Vector&& other) { moveFrom(std::move(other)); }
    constexpr Vector(std::initializer_list<T> init)
        : Vector(init.begin(), init.end()) { }
    constexpr ~Vector() {

    }
    constexpr Vector& operator=(const Vector& other) {

    }
    constexpr Vector& operator=(Vector&& other) { moveFrom(std::move(other)); }

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
    constexpr int_t m_capacity() const { return m_capacity; }
    constexpr bool empty() const { return size() == 0; }
    constexpr bool hasInlineStorage() const { return m_inline; }

    constexpr void reserve(int_t capacity) {

    }
};

}
