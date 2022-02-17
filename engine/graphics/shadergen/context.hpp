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
// Types
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
struct TypeHandle {
    bool builtin = false;
    int_t index = -1;
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
struct TypeContext {
    template<typename T>
    constexpr TypeHandle findType(Type<T> type = {}) {
        if constexpr (type.isBuiltin()) {
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
        return {};
    }
    constexpr const TypeInfo& type(TypeHandle handle) const {
        VERIFY(ShaderGen, handle.builtin);
        return builtinTypeInfos[handle.index];
    }
};

// Statement consumer
struct StmtConsumerBase {
    size_t localSubBlockCount = 0;
    blockidx_t currentBlockIndex = 1;
    int_t nextVarIndex = 0;

    constexpr void beginBlock() {
        currentBlockIndex <<= BLOCK_SHIFT;
        currentBlockIndex |= localSubBlockCount;
        localSubBlockCount = 0;
    }
    constexpr void endBlock() {
        localSubBlockCount = (currentBlockIndex & SUB_BLOCK_MASK) + 1;
        currentBlockIndex >>= BLOCK_SHIFT;
    }
};

template<typename... Consumers>
struct StmtConsumer : StmtConsumerBase {
    std::tuple<Consumers...> consumers;

    template<typename Callback>
    constexpr void forEachConsumer(Callback&& callback) {
        (callback(std::get<Consumers>(consumers)), ...);
    }

    template<typename Stmt>
    constexpr void operator,(Stmt wrappedStmt) {
        auto stmt = unwrap(wrappedStmt);
        forEachConsumer([&](auto& consumer) { consumer.consumeStmt(*this, stmt); });
    }
    template<typename T, fixed_string name>
    constexpr auto declareVariable() {
        VarHandle handle = { nextVarIndex++, currentBlockIndex };
        forEachConsumer([&](auto& consumer) { consumer.template declareVariable<T, name>(*this, handle); });
        return wrap(VarExpr<T, name> { handle });
    }
    template<fixed_string name, typename T>
    constexpr auto declareVariable(Type<T>) {
        return declareVariable<T, name>();
    }

    constexpr void beginBlock() {
        StmtConsumerBase::beginBlock();
        forEachConsumer([&](auto& consumer) { consumer.beginBlock(*this); });
    }
    constexpr void endBlock() {
        forEachConsumer([&](auto& consumer) { consumer.endBlock(*this); });
        StmtConsumerBase::endBlock();
    }
};

struct StmtConsumerStub {
    template<typename T, fixed_string name>
    void declareVariable(StmtConsumerBase&, VarHandle) { }
    template<typename Stmt>
    void consumeStmt(StmtConsumerBase&, Stmt stmt) { }
    void beginBlock(StmtConsumerBase&) { }
    void endBlock(StmtConsumerBase&) { }
};

}

// Formatting
template<typename FormatContext>
static auto format_type(auto type, FormatContext& ctx) {
    if (type.isBuiltin()) {
        if (type.isScalar()) {
            return fmt::format_to(ctx.out(), "[{}]", toString(type.scalarType()));
        } else if (type.isVector()) {
            return fmt::format_to(ctx.out(), "[{} x{}]", toString(type.scalarType()), type.vectorSize());
        } else {
            return fmt::format_to(ctx.out(), "[unknown]");
        }
    } else {
        return fmt::format_to(ctx.out(), "[user type]");
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
