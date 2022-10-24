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
    ASTNodeStorage nodeStorage[N];
    char messageStorage[S] = {};
    int_t nodeCount = 0;
    int_t messageSize = 0;

    constexpr BasicASTStorage() = default;
    constexpr BasicASTStorage(const BasicASTStorage&) = delete;
    constexpr operator ASTStorage() { return { { nodeStorage, &nodeCount }, { messageStorage, &messageSize } }; }
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

namespace code {
    enum MemQual : uint32_t {
        CanAlias = 1,
        NoRead = 2,
        NoWrite = 4,
    };
}
inline constexpr VarDeclModifier memoryCanAlias { "can-alias" };
inline constexpr VarDeclModifier memoryNoRead { "no-read" };
inline constexpr VarDeclModifier memoryNoWrite { "no-write" };
inline constexpr VarDeclModifier uniformImageIndicator { "uniform-image" };
inline constexpr VarDeclModifier arrayBufferIndicator { "array-buffer" };

struct ComputeDriver : StageDriver {
    constexpr ComputeDriver(ASTStorage storage)
        : StageDriver(storage) { }

    template<typename T>
    constexpr auto uniformBlock(std::string_view name) {
        return declareGlobal<T, 1>(name, { &uniformIndicator });
    }
    template<typename T>
    constexpr auto uniformImage(std::string_view name, code::MemQual qual) {
        const VarDeclModifier* mods[4] = { &uniformImageIndicator };
        int_t offset = 1;
        if (qual & code::MemQual::CanAlias)
            mods[offset++] = &memoryCanAlias;
        if (qual & code::MemQual::NoRead)
            mods[offset++] = &memoryNoRead;
        if (qual & code::MemQual::NoWrite)
            mods[offset++] = &memoryNoWrite;
        return declareGlobal<T, 4>(name, { mods, offset });
    }
    template<typename T>
    constexpr auto arrayBuffer(std::string_view name, code::MemQual qual) {
        const VarDeclModifier* mods[4] = { &arrayBufferIndicator };
        int_t offset = 1;
        if (qual & code::MemQual::CanAlias)
            mods[offset++] = &memoryCanAlias;
        if (qual & code::MemQual::NoRead)
            mods[offset++] = &memoryNoRead;
        if (qual & code::MemQual::NoWrite)
            mods[offset++] = &memoryNoWrite;
        return declareGlobal<T[], 4>(name, { mods, offset });
    }
};

struct GraphicsStageDriver : StageDriver {
    constexpr GraphicsStageDriver(ASTStorage storage)
        : StageDriver(storage) { }

    template<typename T>
    constexpr auto uniform(std::string_view name) {
        return declareGlobal<T, 1>(name, { &uniformIndicator });
    }
    template<typename T>
    constexpr auto input(std::string_view name) {
        return declareGlobal<T, 1>(name, { &inputIndicator });
    }
    template<typename T>
    constexpr auto output(std::string_view name) {
        return declareGlobal<T, 1>(name, { &outputIndicator });
    }
    template<typename T>
    constexpr auto builtin(std::string_view name) {
        return declareGlobal<T, 1>(name);
    }
};

template<typename T>
struct VaryingReturnType {
    DriverVariable<T, 1> in;
    DriverVariable<T, 1> out;
    constexpr VaryingReturnType(std::string_view name, GraphicsStageDriver& inStage, GraphicsStageDriver& outStage)
        : in { inStage.output<T>(name) }, out { outStage.input<T>(name) } { }
};

struct GraphicsDriver {
    using Storage = BasicASTStorage<1000, 10000>;
    Storage storage;
    StageDriver vert { storage };
    StageDriver frag { storage };

    constexpr GraphicsDriver() = default;
    template<typename Callback>
    constexpr GraphicsDriver(Callback&& callback) {
        callback(*this);
        PrevDeclStripper stripper;
        stripper.visitStageDecl(vert.stageDecl());
        stripper.visitStageDecl(frag.stageDecl());
    }

    template<typename T>
    constexpr VaryingReturnType<T> varying(std::string_view name) {
        return { name, vert, frag };
    }
};
}
