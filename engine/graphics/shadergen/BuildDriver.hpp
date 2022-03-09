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

// Driver
struct CoreDriver {
    using Storage = BasicASTStorage<1000, 1000>;
    ASTBuilder<Storage> builder;

    constexpr CoreDriver(Storage* storage)
        : builder(storage) { }

    constexpr void emitStmt(Stmt* stmt) {
        builder.storage->pushBackOnList(*builder.currentBlock->body, stmt);
    }

    constexpr VarDecl* declareVariableIn(VarDeclContext* declContext, std::string_view name, const Type* type) {
        auto* decl = builder.storage->allocateVarDecl({ name, type, declContext });
        builder.storage->pushBackOnList(declContext->decls, decl);
        return decl;
    }
    constexpr VarRefExpr declareVariableImpl(std::string_view name, const Type* type) {
        auto* decl = declareVariableIn(&builder.currentBlock->declContext, name, type);
        auto* declStmt = builder.storage->allocateStmt<VarDeclStmt>({ decl });
        emitStmt(declStmt);
        return { decl };
    }
    template<typename T>
    constexpr typename get_wrapper<T>::template impl<VarRefExpr> variable(std::string_view name) {
        return { declareVariableImpl(name, typeFromCXX<T>) };
    }
    constexpr wrapped_scalar::impl<VarRefExpr> declareScalar(std::string_view name, const Type* type) {
        return { declareVariableImpl(name, type) };
    }
    constexpr wrapped_vector::impl<VarRefExpr> declareVector(std::string_view name, const VectorType* type) {
        return { declareVariableImpl(name, type) };
    }

    template<typename Block>
    constexpr void block(Block&& block) {
        builder.pushBlock();
        block();
        emitStmt(builder.popBlock());
    }
    template<typename WhenTrue, typename WhenFalse = std::nullptr_t>
    constexpr void branch(const Expr& conditionIn, WhenTrue&& whenTrue, WhenFalse&& whenFalse = {}) {
        Expr* condition = castStmt<Expr>(builder.dispatchTransform(&conditionIn));
        condition = builder.checkBranchCondtion(condition);
        auto* ifStmt = builder.storage->allocateStmt<IfStmt>({ condition });
        builder.pushBlock();
        whenTrue();
        ifStmt->whenTrue = builder.popBlock();
        if constexpr (!std::is_same_v<WhenFalse, std::nullptr_t>) {
            builder.pushBlock();
            whenFalse();
            ifStmt->whenFalse = builder.popBlock();
        }
        emitStmt(ifStmt);
    }

    constexpr void operator,(const Stmt& in) {
        Stmt* stmt = builder.dispatchTransform(&in);
        emitStmt(stmt);
    }
};

struct StageDriver : CoreDriver {
    VarDeclContext uniforms;

    BlockStmt* mainBlock;

    constexpr StageDriver(Storage* storage, VarDeclContext* parentContext)
        : CoreDriver(storage), uniforms { parentContext } {
        auto* body = storage->allocateStmt<CompoundStmt>({});
        mainBlock = storage->allocateStmt<BlockStmt>({ body });
        mainBlock->declContext.parent = &uniforms;
        builder.currentBlock = mainBlock;
    }

    template<typename T>
    constexpr typename get_wrapper<T>::template impl<VarRefExpr> uniform(std::string_view name) {
        return { { declareVariableIn(&uniforms, name, typeFromCXX<T>) } };
    }
};

struct VertexStageDriver : StageDriver {
    VarDeclContext attributes;
    VarDeclContext builtins;

    VarDecl* positionDecl;

    constexpr VertexStageDriver(Storage* storage, VarDeclContext* varyings)
        : StageDriver(storage, &builtins), attributes { varyings }, builtins { &attributes } {
        positionDecl = declareVariableIn(&builtins, "gl_Position", FloatType.vectorType(4));
    }

    template<typename T>
    constexpr typename get_wrapper<T>::template impl<VarRefExpr> attribute(std::string_view name) {
        const Type* type = typeFromCXX<T>;
        VERIFY(ShaderGen, type->isVector);
        return { { declareVariableIn(&attributes, name, type) } };
    }

    constexpr wrapped_vector::impl<VarRefExpr> position() { return { { positionDecl } }; }
};

struct FragmentStageDriver : StageDriver {
    VarDeclContext colorOutputs;
    VarDeclContext builtins;

    constexpr FragmentStageDriver(Storage* storage, VarDeclContext* varyings)
        : StageDriver(storage, &builtins), colorOutputs { varyings }, builtins { &colorOutputs } { }

    template<typename T>
    constexpr typename get_wrapper<T>::template impl<VarRefExpr> colorOutput(std::string_view name) {
        const Type* type = typeFromCXX<T>;
        VERIFY(ShaderGen, type->isVector);
        return { { declareVariableIn(&colorOutputs, name, type) } };
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
    }

    template<typename T>
    constexpr typename get_wrapper<T>::template impl<VarRefExpr> varying(std::string_view name) {
        const Type* type = typeFromCXX<T>;
        VERIFY(ShaderGen, type->isVector);
        return { { vert.declareVariableIn(&varyings, name, type) } };
    }
};

}
