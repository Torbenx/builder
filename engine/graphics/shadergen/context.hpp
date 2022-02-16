#pragma once

#include "TemplateAST.hpp"
#include "util/reflection.hpp"
#include "util/types.hpp"
#include "util/uninitialized.hpp"

// TypeIdentifier
template<typename T>
class IdentifableType;

class TypeIdentifier {
private:
    struct stub { };

    const stub* id;

    constexpr TypeIdentifier(const stub* id)
        : id(id) { }

    template<typename T>
    friend class IdentifableType;

public:
    constexpr bool operator==(const TypeIdentifier& other) const = default;
};

template<typename T>
class IdentifableType {
private:
    static constexpr TypeIdentifier::stub s_identifier;

public:
    static constexpr TypeIdentifier TYPE_ID = { &s_identifier };
};

// StaticVector
template<typename T, int_t cap>
struct StaticVector {
    uninitialized<T> storage[cap];
    int_t m_size = 0;

    constexpr void pushBack(T t) {
        storage[size()].initialize(t);
        m_size++;
    }

    constexpr T& operator[](int_t i) {
        return storage[i].value;
    }
    constexpr const T& operator[](int_t i) const {
        return storage[i].value;
    }

    constexpr int_t size() const { return m_size; }

    constexpr ~StaticVector() {
        for (int_t i = 0; i < size(); i++) {
            storage[i].destroy();
        }
    }
};

namespace shadergen {

// Name and Type validation
static constexpr int_t MAX_NAME_LENGTH = 22;

struct DynamicName {
    char storage[MAX_NAME_LENGTH + 1] = {};
    int8_t m_length = 0;

    constexpr DynamicName() { }
    template<int_t N>
    constexpr DynamicName(fixed_string<N> str)
        : m_length(N) {
        static_assert(str.length() <= MAX_NAME_LENGTH);
        std::copy_n(str.data(), length() + 1, storage);
    }
    constexpr DynamicName(std::string_view view)
        : m_length(view.length()) {
        std::copy_n(view.data(), length() + 1, storage);
    }

    constexpr char* data() {
        return storage;
    }
    constexpr std::string_view view() const { return { data(), (size_t)length() }; }
    constexpr const char* data() const {
        return storage;
    }
    constexpr int_t length() const { return m_length; }
};

struct TypeInfo {
    bool scalar = false;
    bool vector = false;
    ScalarType m_scalarType;
    int_t m_vectorSize = 0;
    constexpr bool isBuiltin() const { return true; }
    constexpr bool isVector() const { return vector; }
    constexpr bool isScalar() const { return scalar; }
    constexpr ScalarType scalarType() const { return m_scalarType; }
    constexpr int_t vectorSize() const { return m_vectorSize; }
    // StaticVector<FieldDecl, 20> fields;
    // TypeIdentifier typeId;
};
static constexpr TypeInfo builtinTypeInfos[] = {
    { true, true, ScalarType::BOOL, 1 },
    { false, true, ScalarType::BOOL, 2 },
    { false, true, ScalarType::BOOL, 3 },
    { false, true, ScalarType::BOOL, 4 },
    { true, true, ScalarType::INT, 1 },
    { false, true, ScalarType::INT, 2 },
    { false, true, ScalarType::INT, 3 },
    { false, true, ScalarType::INT, 4 },
    { true, true, ScalarType::FLOAT, 1 },
    { false, true, ScalarType::FLOAT, 2 },
    { false, true, ScalarType::FLOAT, 3 },
    { false, true, ScalarType::FLOAT, 4 },
};
struct TypeHandle {
    bool builtin = false;
    int_t index = -1;
};
struct TypeContext {
    constexpr TypeHandle findType(const auto& type) {
        VERIFY(ShaderGen, type.isBuiltin());
        VERIFY(ShaderGen, type.vectorSize() <= 4);
        int_t index;
        switch (type.scalarType()) {
        case ScalarType::BOOL:
            index = 0;
            break;
        case ScalarType::INT:
            index = 1;
            break;
        case ScalarType::FLOAT:
            index = 2;
            break;
        default:
            VERIFY_NOT_REACHED(ShaderGen);
        }
        TypeHandle ret = { true, index * 4 + type.vectorSize() - 1 };
        return ret;
    }
    constexpr const TypeInfo& type(TypeHandle handle) const {
        VERIFY(ShaderGen, handle.builtin);
        return builtinTypeInfos[handle.index];
    }
};

struct VarDecl {
    DynamicName m_name;
    bool isInitialized = false;
    constexpr std::string_view name() const { return m_name.view(); }
};
struct VarHandle {
    int_t index = -1;
};

struct VarDeclContext {
    StaticVector<VarDecl, 100> variables;
    constexpr VarHandle findVarDecl(std::string_view name) {
        for (int_t i = 0; i < variables.size(); i++) {
            if (variables[i].name() == name) {
                return { i };
            }
        }
        variables.pushBack(VarDecl { name });
        return { variables.size() - 1 };
    }
    template<int_t N>
    constexpr VarHandle findVarDecl(fixed_string<N> name) {
        return findVarDecl(name.view());
    }

    constexpr const VarDecl& varDecl(VarHandle handle) const {
        return variables[handle.index];
    }
    constexpr VarDecl& varDecl(VarHandle handle) {
        return variables[handle.index];
    }
};

}

template<typename FormatContext>
static auto format_type(auto type, FormatContext& ctx) {
    if (type.isScalar()) {
        return fmt::format_to(ctx.out(), "[{}]", toString(type.scalarType()));
    } else if (type.isVector()) {
        return fmt::format_to(ctx.out(), "[{} x{}]", toString(type.scalarType()), type.vectorSize());
    } else {
        return fmt::format_to(ctx.out(), "[unknown]");
    }
}

template<typename T>
struct fmt::formatter<shadergen::Type<T>> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(shadergen::Type<T> type, FormatContext& ctx) {
        return format_type(type, ctx);
    }
};

template<>
struct fmt::formatter<shadergen::TypeInfo> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(shadergen::TypeInfo type, FormatContext& ctx) {
        return format_type(type, ctx);
    }
};
