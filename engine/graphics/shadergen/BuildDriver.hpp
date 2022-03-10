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
        CheckedASTTransform<Storage> ast { storage };
        ast.currentDeclContext = declContext;
        ast.dispatchTransformVarDecl(&driverDecl);
    }
    DriverVariable(const DriverVariable&) = delete;

    using Base::operator=;
};

// Driver
struct BlockDriver {
    using Storage = BasicASTStorage<1000, 1000>;

    CheckedASTTransform<Storage> builder;
    BlockStmt* currentBlock = nullptr;

    struct BlockScope {
        BlockDriver* thiz;
        BlockStmt* prevBlock;
        constexpr BlockScope(BlockDriver* thiz)
            : thiz(thiz), prevBlock(thiz->currentBlock) {
            thiz->currentBlock = thiz->newBlock();
            thiz->currentBlock->declContext.parent = thiz->builder.currentDeclContext;
            thiz->builder.currentDeclContext = &thiz->currentBlock->declContext;
        }
        constexpr Stmt* pop() {
            thiz->builder.checkCompleteBlock(thiz->currentBlock);
            Stmt* stmt = thiz->builder.wrapStmtWithError(thiz->currentBlock);
            thiz->currentBlock = prevBlock;
            prevBlock = nullptr;
            return stmt;
        }
        constexpr ~BlockScope() {
            if (prevBlock)
                thiz->emitStmt(pop());
        }
    };

    constexpr BlockDriver(Storage* storage, VarDeclContext* parentContext)
        : builder(storage) {
        currentBlock = newBlock();
        currentBlock->declContext.parent = parentContext;
        builder.currentDeclContext = &currentBlock->declContext;
    }

    constexpr void emitStmt(Stmt* stmt) {
        builder.storage->pushBackOnList(*currentBlock->body, stmt);
    }
    constexpr BlockStmt* newBlock() {
        auto* body = builder.storage->template allocateStmt<CompoundStmt>({});
        auto* block = builder.storage->template allocateStmt<BlockStmt>({ body });
        return block;
    }

    template<typename T>
    constexpr DriverVariable<T> declareVariable(std::string_view name, VarDeclContext* declContext) {
        return { builder.storage, name, declContext };
    }
    template<typename T>
    constexpr DriverVariable<T> variable(std::string_view name) {
        return declareVariable<T>(name, builder.currentDeclContext);
    }

    template<typename Block>
    constexpr void block(Block&& block) {
        BlockScope scope { this };
        block();
    }
    template<typename WhenTrue, typename WhenFalse = std::nullptr_t>
    constexpr void branch(const Expr& conditionIn, WhenTrue&& whenTrue, WhenFalse&& whenFalse = {}) {
        Expr* condition = castStmt<Expr>(builder.dispatchTransform(&conditionIn));
        builder.checkBranchCondtion(condition);
        auto* ifStmt = builder.storage->allocateStmt<IfStmt>({ condition });
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
        emitStmt(builder.wrapStmtWithError(ifStmt));
    }

    constexpr void operator,(const Stmt& in) {
        Stmt* stmt = builder.dispatchTransform(&in);
        emitStmt(stmt);
    }
};

struct StageDriver : BlockDriver {
    VarDeclContext uniforms;

    BlockStmt* mainBlock;

    constexpr StageDriver(Storage* storage, VarDeclContext* parentContext)
        : BlockDriver(storage, &uniforms), uniforms { parentContext }, mainBlock(currentBlock) { }

    template<typename T>
    constexpr auto uniform(std::string_view name) {
        return declareVariable<T>(name, &uniforms);
    }
};

struct VertexStageDriver : StageDriver {
    VarDeclContext attributes;
    VarDeclContext builtins;

    DriverVariable<Vec4> position;

    constexpr VertexStageDriver(Storage* storage, VarDeclContext* varyings)
        : StageDriver(storage, &builtins)
        , attributes { varyings }
        , builtins { &attributes }
        , position { declareVariable<Vec4>("gl_Position", &builtins) } { }

    template<typename T>
    constexpr auto attribute(std::string_view name) {
        return declareVariable<T>(name, &attributes);
    }
};

struct FragmentStageDriver : StageDriver {
    VarDeclContext colorOutputs;
    VarDeclContext builtins;

    constexpr FragmentStageDriver(Storage* storage, VarDeclContext* varyings)
        : StageDriver(storage, &builtins)
        , colorOutputs { varyings }
        , builtins { &colorOutputs } { }

    template<typename T>
    constexpr auto colorOutput(std::string_view name) {
        return declareVariable<T>(name, &colorOutputs);
    }
};

struct GraphicsDriver {
    using Storage = StageDriver::Storage;
    Storage storage;

    VarDeclContext varyings;

    VertexStageDriver vert { &storage, &varyings };
    FragmentStageDriver frag { &storage, &varyings };

    constexpr GraphicsDriver() = default;
    template<typename Callback>
    constexpr GraphicsDriver(Callback&& callback) {
        callback(*this);
        PrevDeclStripper stripper;
        stripper.dispatchVisit(vert.mainBlock);
        stripper.dispatchVisit(frag.mainBlock);
        stripper.stripDeclContext(&varyings);
        stripper.stripDeclContext(&vert.attributes);
        stripper.stripDeclContext(&vert.builtins);
        stripper.stripDeclContext(&vert.uniforms);
        stripper.stripDeclContext(&frag.colorOutputs);
        stripper.stripDeclContext(&frag.builtins);
        stripper.stripDeclContext(&frag.uniforms);
    }

    template<typename T>
    constexpr auto varying(std::string_view name) {
        return vert.declareVariable<T>(name, &varyings);
    }
};

}
