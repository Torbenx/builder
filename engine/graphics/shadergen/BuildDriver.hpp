#pragma once

#include "ASTBuilder.hpp"

namespace shadergen {

struct FixedStringView {
    const char* buffer = nullptr;
    int_t length = 0;
    constexpr operator std::string_view() const { return { buffer, (size_t)length }; }
};
template<int_t N>
struct FixedString {
    char buffer[N] = {};
    constexpr FixedString(const char (&str)[N + 1]) {
        std::copy_n(str, N, buffer);
    }
    constexpr int_t length() const { return N; }
    constexpr operator FixedStringView() const { return { buffer, length() }; }
};
template<int_t M>
FixedString(const char (&str)[M]) -> FixedString<M - 1>;

namespace code {
    template<FixedString str>
    constexpr FixedStringView operator"" _v() {
        return str;
    }
}

// Storage
template<int_t N, int_t S>
struct BasicASTStorage {
    ASTNodeStorage nodeStorage[N];
    char messageStorage[S] = {};
    int_t nodeCount = 0;
    int_t messageSize = 0;

    constexpr BasicASTStorage() = default;
    constexpr BasicASTStorage(const BasicASTStorage&) = delete;
    constexpr operator ASTStorage() { return { { nodeStorage, &nodeCount }, { messageStorage, &messageSize } }; }
};

template<typename T>
struct DriverVariable : get_wrapper<T>::template impl<VarRefExpr> {
    using Base = typename get_wrapper<T>::template impl<VarRefExpr>;
    VarDecl driverDecl;
    template<typename Callback>
    constexpr DriverVariable(std::string_view name, Callback&& callback, const VarDeclContext* declContext = nullptr)
        : Base { { &driverDecl } }, driverDecl(name, typeFromCXX<T>, declContext) {
        callback(&driverDecl);
    }
    DriverVariable(const DriverVariable&) = delete;

    using Base::operator=;
    constexpr AssignmentStmt operator=(const DriverVariable& right) const {
        return { this, right.wrappedExpr };
    }
};

// Driver
struct BlockDriver : CheckedASTTransform<BlockDriver> {
    using Base = CheckedASTTransform<BlockDriver>;

    BlockStmt* currentBlock = nullptr;

    struct BlockScope {
        BlockDriver* thiz;
        BlockStmt* prevBlock;
        constexpr BlockScope(BlockDriver* thiz)
            : thiz(thiz), prevBlock(thiz->currentBlock) {
            thiz->currentBlock = thiz->newBlock(thiz->currentDeclContext);
            thiz->currentDeclContext = &thiz->currentBlock->declContext;
        }
        constexpr Stmt* pop() {
            thiz->checkCompleteBlock(thiz->currentBlock);
            Stmt* stmt = thiz->wrapStmtWithError(thiz->currentBlock);
            thiz->currentBlock = prevBlock;
            thiz->currentDeclContext = thiz->currentDeclContext->parent;
            prevBlock = nullptr;
            return stmt;
        }
        constexpr ~BlockScope() {
            if (prevBlock)
                thiz->emitStmt(pop());
        }
    };

    constexpr BlockDriver(ASTStorage storage)
        : Base(storage) { }

    constexpr void emitStmt(Stmt* stmt) {
        storage.pushBackOnList(*currentBlock->body, stmt);
    }
    constexpr BlockStmt* newBlock(VarDeclContext* parent = nullptr) {
        auto* body = storage.template allocateStmt<CompoundStmt>({});
        auto* block = storage.template allocateStmt<BlockStmt>({ body });
        block->declContext.parent = parent;
        return block;
    }

    template<typename T>
    constexpr DriverVariable<T> declareVariable(FixedStringView name, VarDeclContext* declContext) {
        return DriverVariable<T>(
            name, [&](const VarDecl* driverDecl) constexpr {
                createVarDecl(driverDecl, declContext);
            });
    }
    template<typename T>
    constexpr DriverVariable<T> variable(FixedStringView name) {
        return declareVariable<T>(name, currentDeclContext);
    }

    template<typename Block>
    constexpr void block(Block&& block) {
        BlockScope scope { this };
        block();
    }
    template<typename WhenTrue, typename WhenFalse = std::nullptr_t>
    constexpr void branch(const Expr& conditionIn, WhenTrue&& whenTrue, WhenFalse&& whenFalse = {}) {
        Expr* condition = castStmt<Expr>(dispatchTransform(&conditionIn));
        checkBranchCondtion(condition);
        auto* ifStmt = storage.allocateStmt<IfStmt>({ condition });
        {
            BlockScope scope { this };
            whenTrue();
            ifStmt->whenTrue = scope.pop();
        }
        if constexpr (!std::is_same_v<WhenFalse, std::nullptr_t>) {
            BlockScope scope { this };
            whenFalse();
            ifStmt->whenFalse = scope.pop();
        }
        emitStmt(wrapStmtWithError(ifStmt));
    }

    constexpr void operator,(const Stmt& in) {
        Stmt* stmt = dispatchTransform(&in);
        emitStmt(stmt);
    }
};

struct StageDriver : BlockDriver {
    StageDecl* m_stageDecl = nullptr;

    constexpr StageDriver(ASTStorage storage)
        : BlockDriver(storage), m_stageDecl(createEmptyStageDecl()) {
        createEntryPoint();
    }
    constexpr StageDriver(ASTStorage storage, StageDecl* stageDecl)
        : BlockDriver(storage), m_stageDecl(stageDecl) {
        if (stageDecl)
            createEntryPoint();
    }
    constexpr void initialize(StageDecl* stageDecl) {
        this->m_stageDecl = stageDecl;
        createEntryPoint();
    }
    constexpr void createEntryPoint() {
        currentBlock = newBlock(&stageDecl()->outputs);
        stageDecl()->entryPoint = currentBlock;
        currentDeclContext = &currentBlock->declContext;
    }
    constexpr StageDecl* stageDecl() const { return m_stageDecl; }

    template<typename T>
    constexpr auto uniform(FixedStringView name) {
        return declareVariable<T>(name, &stageDecl()->uniforms);
    }
    template<typename T>
    constexpr auto input(FixedStringView name) {
        return declareVariable<T>(name, &stageDecl()->inputs);
    }
    template<typename T>
    constexpr auto output(FixedStringView name) {
        return declareVariable<T>(name, &stageDecl()->outputs);
    }
    template<typename T>
    constexpr auto builtin(FixedStringView name) {
        return declareVariable<T>(name, &stageDecl()->builtins);
    }
};

struct VertexStageDriver : StageDriver {

    constexpr VertexStageDriver(ASTStorage storage)
        : StageDriver(storage) { }
};

struct FragmentStageDriver : StageDriver {

    constexpr FragmentStageDriver(ASTStorage storage)
        : StageDriver(storage) { }
};

template<typename T>
struct VaryingReturnType {
    constexpr VaryingReturnType(FixedStringView name, StageDriver& inStage, StageDriver& outStage)
        : in { inStage.output<T>(name) }, out { outStage.input<T>(name) } { }
    DriverVariable<T> in;
    DriverVariable<T> out;
};

struct GraphicsDriver {

    using Storage = BasicASTStorage<1000, 10000>;
    Storage storage;
    VertexStageDriver vert { storage };
    FragmentStageDriver frag { storage };

    constexpr GraphicsDriver() = default;
    template<typename Callback>
    constexpr GraphicsDriver(Callback&& callback) {
        callback(*this);
        PrevDeclStripper stripper;
        stripper.stripStage(vert.stageDecl());
        stripper.stripStage(frag.stageDecl());
    }

    template<typename T>
    constexpr VaryingReturnType<T> varying(FixedStringView name) {
        return { name, vert, frag };
    }
};
}
