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
struct BuildDriver {
    using Storage = BasicASTStorage<1000, 1000>;
    ASTBuilder<Storage> builder { &storage };
    Storage storage;

    constexpr void emitStmt(Stmt* stmt) {
        storage.pushBackOnList(*builder.currentBlock->body, stmt);
    }

    constexpr VarRefExpr declareVariableImpl(std::string_view name, const Type* type) {
        auto* decl = storage.allocateVarDecl({ name, type, &builder.currentBlock->declContext });
        storage.pushBackOnList(builder.currentBlock->declContext.decls, decl);
        auto* declStmt = storage.allocateStmt<VarDeclStmt>({ decl });
        emitStmt(declStmt);
        return { decl };
    }
    template<typename T>
    constexpr typename get_wrapper<T>::template impl<VarRefExpr> declareVariable(std::string_view name) {
        return { declareVariableImpl(name, typeFromCXX<T>) };
    }
    constexpr wrapped_scalar::impl<VarRefExpr> declareScalar(std::string_view name, const Type* type) {
        return { declareVariableImpl(name, type) };
    }
    constexpr wrapped_vector::impl<VarRefExpr> declareVector(std::string_view name, const VectorType* type) {
        return { declareVariableImpl(name, type) };
    }

    constexpr BlockStmt* newBlock() {
        auto* body = storage.allocateStmt<CompoundStmt>({});
        auto* block = storage.allocateStmt<BlockStmt>({ body });
        return block;
    }
    constexpr void pushBlock() {
        auto* newBlock = this->newBlock();
        newBlock->parent = builder.currentBlock;
        builder.currentBlock = newBlock;
    }
    constexpr Stmt* popBlock() {
        Stmt* block = builder.checkCompleteBlock(builder.currentBlock);
        builder.currentBlock = builder.currentBlock->parent;
        return block;
    }
    constexpr void beginBlock() {
        pushBlock();
    }
    constexpr void endBlock() {
        emitStmt(popBlock());
    }
    template<typename WhenTrue, typename WhenFalse = std::nullptr_t>
    constexpr void branch(const Expr& conditionIn, WhenTrue&& whenTrue, WhenFalse&& whenFalse = {}) {
        Expr* condition = castStmt<Expr>(builder.dispatchTransform(&conditionIn));
        condition = builder.checkBranchCondtion(condition);
        auto* ifStmt = storage.allocateStmt<IfStmt>({ condition });
        pushBlock();
        whenTrue();
        ifStmt->whenTrue = popBlock();
        if constexpr (!std::is_same_v<WhenFalse, std::nullptr_t>) {
            pushBlock();
            whenFalse();
            ifStmt->whenFalse = popBlock();
        }
        emitStmt(ifStmt);
    }

    constexpr void operator,(const Stmt& in) {
        Stmt* stmt = builder.dispatchTransform(&in);
        emitStmt(stmt);
    }

    template<typename Callback>
    constexpr BuildDriver(Callback&& callback) {
        builder.currentBlock = newBlock();
        callback(*this);
    }
};
}
