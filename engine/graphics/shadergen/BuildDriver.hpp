#pragma once

#include "ASTBuilder.hpp"
#include "containers/FixedString.hpp"

namespace shadergen {

namespace code {
    template<stx::FixedString str>
    constexpr const stx::FixedString<str.size()>& operator"" _v() {
        return str;
    }
}
template<stx::FixedString str>
constexpr std::string_view varName() { return str; }

// Storage
template<int_t N, int_t S>
struct BasicASTStorage {
    OversizedArray<ASTNodeStorage, N> nodeStorage;
    OversizedArray<char, S> messageStorage;

    constexpr BasicASTStorage() = default;
    constexpr BasicASTStorage(const BasicASTStorage&) = delete;
    constexpr operator ASTStorage() { return { nodeStorage, messageStorage }; }
};

template<typename wrapper, int_t N = 0>
struct DriverVariable : wrapper::template impl<VarRefExpr> {
    using Base = typename wrapper::template impl<VarRefExpr>;
    VarDecl driverDecl;
    stx::Array<const VarDeclModifier*, N> modifierStorage = {};
    template<typename Callback>
    constexpr DriverVariable(std::string_view name, const Type* type, ArrayView<const VarDeclModifier* const> mods, Callback&& callback, const VarDeclContext* declContext = nullptr)
        : Base { { &driverDecl } }, driverDecl(name, type, declContext) {
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
template<typename T, int_t N = 0, typename Callback>
constexpr auto makeDriverVariable(std::string_view name, ArrayView<const VarDeclModifier* const> mods, Callback&& callback, const VarDeclContext* declContext = nullptr) {
    return DriverVariable<get_wrapper<T>, N>(name, typeFromCXX<T>, mods, std::forward<Callback>(callback), declContext);
}
template<typename T, int_t N = 0>
constexpr auto declareVariable(ASTStorage storage, std::string_view name, VarDeclContext* declContext, ArrayView<const VarDeclModifier* const> mods = {}) {
    return makeDriverVariable<T, N>(
        name, mods, [&](const VarDecl* driverDecl) constexpr {
            createVarDecl(storage, driverDecl, declContext);
        });
}

// Driver
struct BlockDriver : CheckedASTTransform<BlockDriver> {
    using Base = CheckedASTTransform<BlockDriver>;

    BlockStmt* currentBlock = nullptr;

    struct BlockScope : DeclContextScope {
        BlockStmt* prevBlock;
        constexpr BlockScope(BlockDriver* thiz)
            : BlockScope(thiz, thiz->newBlock()) { }
        constexpr BlockScope(BlockDriver* thiz, BlockStmt* newBlock)
            : DeclContextScope(thiz, &newBlock->declContext), prevBlock(thiz->currentBlock) {
            thiz->currentBlock = newBlock;
        }
        constexpr Stmt* pop() {
            thiz->checkCompleteBlock(thiz->currentBlock);
            Stmt* stmt = thiz->wrapStmtWithError(thiz->currentBlock);
            thiz->currentBlock = prevBlock;
            DeclContextScope::pop();
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
    constexpr auto variable(std::string_view name) {
        return declareVariable<T>(storage, name, currentDeclContext);
    }
    template<typename wrapper_impl>
    constexpr auto variableWithInit(std::string_view name, const wrapper_impl& initIn) {
        Expr* init = castStmt<Expr>(dispatchTransform((const Expr*)&initIn));
        return DriverVariable<typename wrapper_impl::wrapper_base>(
            name, init->type, {}, [&](const VarDecl* driverDecl) constexpr {
                VarDecl* decl = createVarDecl(driverDecl, currentDeclContext);

                VarRefExpr* ref = storage.allocateStmt<VarRefExpr>({ decl });
                ref->type = init->type;
                ref->valueKind = ValueKind::LVALUE;

                AssignmentStmt* assignment = storage.allocateStmt<AssignmentStmt>({ ref, init });
                emitStmt(assignment);
            });
    }

    template<typename Block>
    constexpr void block(Block&& block) {
        BlockScope scope { this };
        block();
    }
    template<typename WhenTrue, typename WhenFalse = std::nullptr_t>
    constexpr void branch(const Expr& conditionIn, WhenTrue&& whenTrue, WhenFalse&& whenFalse = {}) {
        Expr* condition = castStmt<Expr>(dispatchTransform(&conditionIn));
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
        checkBranchCondtion(condition);
        emitStmt(wrapStmtWithError(ifStmt));
    }
    template<typename Body>
    constexpr void range(const Expr& beginIn, const Expr& endIn, Body&& body) {
        Expr* begin = castStmt<Expr>(dispatchTransform(&beginIn));
        Expr* end = castStmt<Expr>(dispatchTransform(&endIn));
        auto* rangeStmt = storage.allocateStmt<RangeStmt>({ begin, end });
        {
            DeclContextScope scope { this, &rangeStmt->iteratorDeclContext };
            auto iterator = variable<int>(varName<"range_iterator">());
            rangeStmt->iteratorDecl = lookupPrevDecl(iterator.decl);
            {
                BlockScope scope { this };
                body(iterator);
                rangeStmt->body = scope.pop();
            }
        }
        checkRangeArgument(begin);
        checkRangeArgument(end);
        emitStmt(wrapStmtWithError(rangeStmt));
    }

    constexpr void operator,(const Stmt& in) {
        Stmt* stmt = dispatchTransform(&in);
        emitStmt(stmt);
    }
};

struct VariableInitHelper {
    BlockDriver& driver;
    std::string_view name;
    constexpr VariableInitHelper(BlockDriver& driver, std::string_view name)
        : driver(driver), name(name) { }

    template<typename wrapper_impl>
    constexpr auto operator=(const wrapper_impl& init) const {
        return driver.variableWithInit(name, init);
    }
};
#define AUTO_VARIABLE(driver, name) auto name = ::shadergen::VariableInitHelper(driver, ::shadergen::varName<#name>())

struct StageDeclDriver : BlockDriver {
    StageDecl* m_stageDecl = nullptr;

    constexpr StageDeclDriver(ASTStorage storage, StageKind stageKind, VarDeclContext* parentDeclContext = nullptr)
        : BlockDriver(storage), m_stageDecl(storage.allocateStageDecl({ stageKind })) {
        stageDecl()->globals.parent = parentDeclContext;
        currentBlock = newBlock(&stageDecl()->globals);
        stageDecl()->entryPoint = currentBlock;
        currentDeclContext = &currentBlock->declContext;
    }
    constexpr StageDecl* stageDecl() const { return m_stageDecl; }
    template<typename T, int_t N = 0>
    constexpr auto declareGlobal(std::string_view name, ArrayView<const VarDeclModifier* const> mods = {}) {
        return declareVariable<T, N>(storage, name, &stageDecl()->globals, mods);
    }
};

struct ShaderDeclDriver {
    ASTStorage storage;
    ShaderDecl* m_shaderDecl = nullptr;

    constexpr ShaderDeclDriver(ASTStorage storage)
        : storage(storage), m_shaderDecl(storage.allocateShaderDecl({})) { }

    constexpr ShaderDecl* shaderDecl() const { return m_shaderDecl; }

    template<typename T, int_t N = 0>
    constexpr auto declareUniform(std::string_view name, ArrayView<const VarDeclModifier* const> mods) {
        return declareVariable<T, N>(storage, name, &shaderDecl()->uniforms, mods);
    }

    constexpr void addStage(StageDecl* stage) {
        storage.pushBackOnList(shaderDecl()->stages, stage);
    }
};

}
