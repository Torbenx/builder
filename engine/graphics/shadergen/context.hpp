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
    template<int_t N>
    constexpr DynamicName(fixed_string<N> str) {
        static_assert(str.length() <= MAX_NAME_LENGTH);
        std::copy(str.data(), str.data() + N, storage);
    }
    constexpr DynamicName() { }
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
    size_t alignment = 0;
    int_t sizeInBytes = -1;
    DynamicName name;
    // StaticVector<FieldDecl, 20> fields;
    // TypeIdentifier typeId;
};
struct TypeHandle {
    int_t index = -1;
};
struct TypeContext {
    template<typename T>
    constexpr TypeHandle findType(Type<T>) {
        return {};
    }
    constexpr TypeInfo type(TypeHandle handle) const {
        return {};
    }
};

struct VarDecl {
    DynamicName name;
    bool isInitialized = false;
};
struct VarHandle {
    int_t index = -1;
};

struct VarDeclContext {
    StaticVector<VarDecl, 100> variables;
    template<typename T, fixed_string name>
    constexpr VarHandle findVarDecl(VarExpr<T, name>) {
        for (int_t i = 0; i < variables.size(); i++) {
            if (variables[i].name.view() == name.view()) {
                return { i };
            }
        }
        variables.pushBack(VarDecl { name });
        return { variables.size() - 1 };
    }

    constexpr VarDecl varDecl(VarHandle handle) const {
        return variables[handle.index];
    }
};

}
