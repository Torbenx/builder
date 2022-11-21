#pragma once

#include "Array.hpp"
#include "ArrayView.hpp"

namespace stx {

template<typename E, typename T>
struct EnumTable : stx::Array<T, stx::to_underlying(E::COUNT)> {
private:
    using Base = stx::Array<T, stx::to_underlying(E::COUNT)>;
    using E_t = std::underlying_type_t<E>;

public:
    struct Entry {
        E key;
        T value;
        constexpr Entry(E key, T value)
            : key(key), value(value) { }
    };
    template<int_t N>
    constexpr EnumTable(T defaultValue, const Entry (&entries)[N]) {

        std::fill_n(this->data(), this->size(), defaultValue);
        stx::Array<bool, stx::to_underlying(E::COUNT)> checkArray = {};
        for (const Entry& entry : entries) {
            auto val = stx::to_underlying(entry.key);
            VERIFY(Containers, !checkArray[val]);
            checkArray[val] = true;
            this->data()[val] = entry.value;
        }
    }

    using Base::operator[];
    constexpr T operator[](E e) const { return (*this)[stx::to_underlying(e)]; }
};

namespace detail {
    template<typename E, template<E> typename Template, E value>
    concept HasSpecialisation = requires { typename Template<value>; };
}

template<typename E, typename T, template<E> typename Template>
struct TemplateEnumTable : stx::Array<T, stx::to_underlying(E::COUNT)> {
private:
    using Base = stx::Array<T, stx::to_underlying(E::COUNT)>;
    template<int_t i>
    static constexpr T valueOrDefault(T defaultValue) {
        if constexpr (detail::HasSpecialisation<E, Template, static_cast<E>(i)>) {
            return Template<static_cast<E>(i)>::value;
        } else {
            return defaultValue;
        }
    }
    template<int_t... I>
    constexpr TemplateEnumTable(T defaultValue, stx::int_sequence<I...>)
        : Base { { valueOrDefault<I>(defaultValue)... } } { }

public:
    constexpr TemplateEnumTable(T defaultValue)
        : TemplateEnumTable(defaultValue, stx::make_int_sequence<stx::to_underlying(E::COUNT)>()) { }

    using Base::operator[];
    constexpr T operator[](E e) const { return (*this)[stx::to_underlying(e)]; }
};

}