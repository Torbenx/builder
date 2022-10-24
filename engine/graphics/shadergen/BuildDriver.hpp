#pragma once

#include "ASTBuilder.hpp"
#include "containers/FixedString.hpp"

namespace shadergen {

namespace code {
    template<stx::FixedString str>
    constexpr std::string_view operator"" _v() {
        return str;
    }
}

// Storage
template<int_t N, int_t S>
struct BasicASTStorage {
    OversizedArray<ASTNodeStorage, N> nodeStorage;
    OversizedArray<char, S> messageStorage;

    constexpr BasicASTStorage() = default;
    constexpr BasicASTStorage(const BasicASTStorage&) = delete;
    constexpr operator ASTStorage() { return { nodeStorage, messageStorage }; }
};

template<typename T, int_t N = 0>
struct DriverVariable : get_wrapper<T>::template impl<VarRefExpr> {
    using Base = typename get_wrapper<T>::template impl<VarRefExpr>;
    VarDecl driverDecl;
    stx::Array<const VarDeclModifier*, N> modifierStorage = {};
    template<typename Callback>
    constexpr DriverVariable(std::string_view name, ArrayView<const VarDeclModifier* const> mods, Callback&& callback, const VarDeclContext* declContext = nullptr)
        : Base { { &driverDecl } }, driverDecl(name, typeFromCXX<T>, declContext) {
        std::copy_n(mods.data(), mods.size(), modifierStorage.data());
        driverDecl.modifiers.setArray(modifierStorage.data(), mods.size());
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

    template<typename T, int_t N = 0>
    constexpr auto declareVariable(std::string_view name, VarDeclContext* declContext, ArrayView<const VarDeclModifier* const> mods = {}) {
        return DriverVariable<T, N>(
            name, mods, [&](const VarDecl* driverDecl) constexpr {
                createVarDecl(driverDecl, declContext);
            });
    }
    template<typename T>
    constexpr auto variable(std::string_view name) {
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
        : BlockDriver(storage), m_stageDecl(storage.allocateStageDecl({})) {
        currentBlock = newBlock(&stageDecl()->globals);
        stageDecl()->entryPoint = currentBlock;
        currentDeclContext = &currentBlock->declContext;
    }
    constexpr StageDecl* stageDecl() const { return m_stageDecl; }
    template<typename T, int_t N = 0>
    constexpr auto declareGlobal(std::string_view name, ArrayView<const VarDeclModifier* const> mods = {}) {
        return declareVariable<T, N>(name, &stageDecl()->globals, mods);
    }
};
}
