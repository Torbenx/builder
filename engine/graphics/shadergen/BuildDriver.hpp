#pragma once

#include "ASTBuilder.hpp"

namespace shadergen {

// Storage
union ASTNodeStorage {
#define AST_NODE(N) N N##_;
    ENUMERATE_AST_NODES(AST_NODE)
#undef AST_NODE

    constexpr Stmt* setStmt(const Stmt& s) {
#define STMT_KIND(S)  \
    case StmtKind::S: \
        return std::construct_at(&(S##_), (const S&)s);

        switch (s.stmtKind) {
            ENUMERATE_STMT_KINDS(STMT_KIND)
        default:
            VERIFY_NOT_REACHED(ShaderGen);
        }
#undef STMT_KIND
    }
    constexpr ASTListEntry* setListEntry(const ASTListEntry& e) {
        return std::construct_at(&ASTListEntry_, e);
    }
    constexpr VarDecl* setVarDecl(const VarDecl& decl) {
        return std::construct_at(&VarDecl_, decl);
    }
    constexpr ErrorNote* setNote(const ErrorNote& note) {
        return std::construct_at(&ErrorNote_, note);
    }
    constexpr StageDecl* setStageDecl(const StageDecl& stage) {
        return std::construct_at(&StageDecl_, stage);
    }

    constexpr ASTNodeStorage() { }
};

template<int_t N, int_t S>
struct BasicASTStorage {
    ASTNodeStorage storage[N];
    char messageBuffer[S] = {};
    int_t size = 0;
    char* messageBufferPosition = messageBuffer;
    constexpr ASTNodeStorage& acquireBack() {
        return storage[size++];
    }
    template<typename T>
    constexpr T* allocateStmt(const T& stmt) {
        return (T*)acquireBack().setStmt(stmt);
    }
    constexpr VarDecl* allocateVarDecl(const VarDecl& decl) {
        return acquireBack().setVarDecl(decl);
    }
    template<typename T>
    constexpr void pushBackOnList(ASTList<T>& list, T* e) {
        ASTListEntry* entry = acquireBack().setListEntry({ e });
        list.pushBack(entry);
    }
    constexpr ErrorNote* allocateNote(const ErrorNote& note) {
        return acquireBack().setNote(note);
    }
    constexpr StageDecl* allocateStageDecl(const StageDecl& stage) {
        return acquireBack().setStageDecl(stage);
    }

    constexpr BasicASTStorage() = default;
    constexpr BasicASTStorage(const BasicASTStorage&) = delete;
};

template<typename T>
struct DriverVariable : get_wrapper<T>::template impl<VarRefExpr> {
    using Base = typename get_wrapper<T>::template impl<VarRefExpr>;
    VarDecl driverDecl;
    template<typename Storage>
    constexpr DriverVariable(Storage* storage, std::string_view name, VarDeclContext* declContext)
        : Base { { &driverDecl } }, driverDecl(name, typeFromCXX<T>, nullptr) {
        BasicASTTransform<Storage> ast { storage };
        ast.createVarDecl(&driverDecl, declContext);
    }
    DriverVariable(const DriverVariable&) = delete;

    using Base::operator=;
    constexpr AssignmentStmt operator=(const DriverVariable& right) {
        return { this, right.wrappedExpr };
    }
};

// Driver
struct BlockDriver : CheckedASTTransform<BlockDriver, BasicASTStorage<1000, 1000>> {
    using Storage = BasicASTStorage<1000, 1000>;
    using Base = CheckedASTTransform<BlockDriver, BasicASTStorage<1000, 1000>>;

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

    constexpr BlockDriver(Storage* storage)
        : Base(storage) { }

    constexpr void emitStmt(Stmt* stmt) {
        storage->pushBackOnList(*currentBlock->body, stmt);
    }
    constexpr BlockStmt* newBlock(VarDeclContext* parent = nullptr) {
        auto* body = storage->template allocateStmt<CompoundStmt>({});
        auto* block = storage->template allocateStmt<BlockStmt>({ body });
        block->declContext.parent = parent;
        return block;
    }

    template<typename T>
    constexpr DriverVariable<T> declareVariable(std::string_view name, VarDeclContext* declContext) {
        return { storage, name, declContext };
    }
    template<typename T>
    constexpr DriverVariable<T> variable(std::string_view name) {
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
        auto* ifStmt = storage->allocateStmt<IfStmt>({ condition });
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
    StageDecl* stageDecl = nullptr;

    constexpr StageDriver(Storage* storage)
        : BlockDriver(storage) {
        stageDecl = createEmptyStageDecl();
        currentBlock = newBlock(&stageDecl->outputs);
        stageDecl->entryPoint = currentBlock;
        currentDeclContext = &currentBlock->declContext;
    }

    template<typename T>
    constexpr auto uniform(std::string_view name) {
        return declareVariable<T>(name, &stageDecl->uniforms);
    }
    template<typename T>
    constexpr auto input(std::string_view name) {
        return declareVariable<T>(name, &stageDecl->inputs);
    }
    template<typename T>
    constexpr auto output(std::string_view name) {
        return declareVariable<T>(name, &stageDecl->outputs);
    }
};

struct VertexStageDriver : StageDriver {
    DriverVariable<Vec4> position;

    constexpr VertexStageDriver(Storage* storage)
        : StageDriver(storage)
        , position { declareVariable<Vec4>("gl_Position", &stageDecl->builtins) } { }
};

struct FragmentStageDriver : StageDriver {

    constexpr FragmentStageDriver(Storage* storage)
        : StageDriver(storage) { }
};

struct GraphicsDriver {
    using Storage = StageDriver::Storage;
    Storage storage;

    VertexStageDriver vert { &storage };
    FragmentStageDriver frag { &storage };

    constexpr GraphicsDriver() = default;
    template<typename Callback>
    constexpr GraphicsDriver(Callback&& callback) {
        callback(*this);
        PrevDeclStripper stripper;
        stripper.stripStage(vert.stageDecl);
        stripper.stripStage(frag.stageDecl);
    }

    template<typename T>
    struct VaryingReturnType {
        DriverVariable<T> in;
        DriverVariable<T> out;
    };
    template<typename T>
    constexpr VaryingReturnType<T> varying(std::string_view name) {
        return { vert.output<T>(name), frag.input<T>(name) };
    }
};

}
