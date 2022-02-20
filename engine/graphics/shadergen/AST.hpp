#pragma once

#include "ASTVisitor.hpp"

namespace shadergen {

// Storage
union StmtStorage {
#define AST_NODE(N) N N##_;
    ENUMERATE_AST_NODES(AST_NODE)
#undef AST_NODE

    constexpr Stmt* set(const Stmt& s) {
        switch (s.stmtKind) {
#define STMT_KIND(S)  \
    case StmtKind::S: \
        return std::construct_at(&(S##_), (const S&)s);

            ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND
        default:
            VERIFY_NOT_REACHED(ShaderGen);
        }
    }
    constexpr ASTListEntry* setListEntry(const ASTListEntry& e) {
        return std::construct_at(&ASTListEntry_, e);
    }
    constexpr VarDecl* setVarDecl(const VarDecl& decl) {
        return std::construct_at(&VarDecl_, decl);
    }

    constexpr StmtStorage() { }
};

template<int_t N>
struct StmtVector {
    StmtStorage storage[N];
    int_t size = 0;
    constexpr StmtStorage& acquireBack() {
        return storage[size++];
    }
    constexpr Stmt* pushBack(const Stmt& stmt) {
        return acquireBack().set(stmt);
    }
};

struct AST {
    StmtVector<100> storage;
    BlockStmt* currentBlock = nullptr;

    template<typename T>
    constexpr T* allocateStmt(const T& stmt) {
        return (T*)storage.pushBack(stmt);
    }
    constexpr VarDecl* allocateVarDecl(const VarDecl& decl) {
        return storage.acquireBack().setVarDecl(decl);
    }
    template<typename T>
    constexpr void pushBackOnList(ASTList<T>& list, T* e) {
        ASTListEntry* entry = storage.acquireBack().setListEntry({ e });
        list.pushBack(entry);
    }

    constexpr BlockStmt* newBlock() {
        auto* body = allocateStmt<CompoundStmt>({});
        auto* block = allocateStmt<BlockStmt>({ body });
        return block;
    }

    constexpr AST()
        : currentBlock(newBlock()) { }
    constexpr AST(const AST&) = delete;

    constexpr VarRefExpr declareVariable(std::string_view name, const Type* type) {
        auto* decl = allocateVarDecl({ name, type, &currentBlock->declContext });
        pushBackOnList(currentBlock->declContext.decls, decl);
        auto* declStmt = allocateStmt<VarDeclStmt>({ decl });
        emitStmt(declStmt);
        return { decl };
    }

    template<typename Impl>
    struct AccessMaskVisitor : ASTVisitor<Impl, void, FixedBitset> {
        constexpr Impl* impl() { return static_cast<Impl*>(this); }

        constexpr FixedBitset fullMask(const Type* type) {
            if (type->isVector)
                return FixedBitset::bitsBelow(asVectorType(type)->vectorSize);
            else
                return FixedBitset::bitsBelow(type->fieldCount);
        }
        constexpr void visitExpr(const Expr*, FixedBitset) { }
        constexpr void visitMemberAccessExpr(const MemberAccessExpr* e, FixedBitset mask) {
            impl()->dispatchVisit(e->base, e->base->type->members[e->memberIndex].fieldMask); // FIXME: this is wrong!
            impl()->visitExpr(e, mask);
        }
        constexpr void visitVectorComponentExpr(const VectorComponentExpr* e, FixedBitset mask) {
            FixedBitset baseMask = 0;
            for (int_t i = 0; i < e->componentCount; i++) {
                if (mask.test(i))
                    baseMask.set(e->components[i]);
            }
            impl()->dispatchVisit(e->base, baseMask);
            impl()->visitExpr(e, mask);
        }
        constexpr void visitVectorByScalarOperatorExpr(const VectorByScalarOperatorExpr* e, FixedBitset mask) {
            impl()->dispatchVisit(e->vector, mask);
            impl()->dispatchVisit(e->scalar, fullMask(e->scalar->type));
            impl()->visitExpr(e, mask);
        }
        constexpr void visit(const Expr* e) {
            impl()->dispatchVisit(e, fullMask(e->type));
        }
    };
    struct CheckInitialized : AccessMaskVisitor<CheckInitialized> {
        constexpr void visitVarRefExpr(const VarRefExpr* e, FixedBitset mask) {
            VERIFY(ShaderGen, !(mask & ~e->decl->initMask));
            e->decl->usedMask |= mask;
        }
    };
    struct MarkInitialized : AccessMaskVisitor<MarkInitialized> {
        constexpr void visitStmt(const Stmt*, FixedBitset) {
            VERIFY_NOT_REACHED(ShaderGen);
        }
        constexpr void visitVarRefExpr(const VarRefExpr* e, FixedBitset mask) {
            e->decl->initMask |= mask;
        }
    };

    struct StmtBuilder : ASTVisitor<StmtBuilder, Stmt*> {
        AST& ast;
        constexpr Stmt* visitStmt(const Stmt*) {
            VERIFY_NOT_REACHED(ShaderGen);
            return ast.allocateStmt<Stmt>({});
        }
        constexpr Stmt* visitVarRefExpr(const VarRefExpr* in) {
            BlockStmt* block = ast.currentBlock;
            while (block && (&block->declContext != in->decl->declContext))
                block = block->parent;
            VERIFY(ShaderGen, block);

            auto* e = ast.allocateStmt<VarRefExpr>({ in->decl });
            e->type = in->decl->type;
            e->valueKind = ValueKind::LVALUE;
            return e;
        }
        constexpr Stmt* visitMemberAccessExpr(const MemberAccessExpr* in) {
            auto* base = castStmt<Expr>(dispatchVisit(in->base));
            VERIFY(ShaderGen, in->memberIndex < base->type->members.size());
            const MemberInfo& member = base->type->members[in->memberIndex];
            auto* e = ast.allocateStmt<MemberAccessExpr>({ base, in->memberIndex });
            e->type = member.type;
            e->valueKind = base->valueKind;
            return e;
        }
        constexpr Stmt* visitVectorComponentExpr(const VectorComponentExpr* in) {
            auto* base = castStmt<Expr>(dispatchVisit(in->base));
            const auto* baseType = asVectorType(base->type);
            VERIFY(ShaderGen, !baseType->isScalar);
            size_t mask = 0;
            bool hasDuplicates = false;
            for (int_t i = 0; i < in->componentCount; i++) {
                VERIFY(ShaderGen, in->components[i] < baseType->vectorSize);
                size_t bit = (size_t)1 << in->components[i];
                if (mask & bit)
                    hasDuplicates = true;
                mask |= bit;
            }
            auto* e = ast.allocateStmt<VectorComponentExpr>({ base, in->componentCount, in->components });
            e->type = baseType->scalarType->vectorType(in->componentCount);
            e->valueKind = hasDuplicates ? ValueKind::RVALUE : base->valueKind;
            return e;
        }
        constexpr Stmt* visitUnaryOperatorExpr(const UnaryOperatorExpr* in) {
            auto* subExpr = castStmt<Expr>(dispatchVisit(in->subExpr));
            const auto* type = asVectorType(subExpr->type);
            if (isLogicalOp(in->op))
                VERIFY(ShaderGen, type->isLogical);
            else if (isArithmeticOp(in->op))
                VERIFY(ShaderGen, type->isArithmetic);
            auto* e = ast.allocateStmt<UnaryOperatorExpr>({ in->op, subExpr });
            e->type = type;
            e->valueKind = ValueKind::RVALUE;
            return e;
        }
        constexpr Stmt* visitElementwiseOperatorExpr(const ElementwiseOperatorExpr* in) {
            auto* left = castStmt<Expr>(dispatchVisit(in->left));
            auto* right = castStmt<Expr>(dispatchVisit(in->right));
            const auto* lType = asVectorType(left->type);
            const auto* rType = asVectorType(right->type);
            VERIFY(ShaderGen, lType->scalarType == rType->scalarType);
            VERIFY(ShaderGen, lType->vectorSize == rType->vectorSize);
            auto* e = ast.allocateStmt<ElementwiseOperatorExpr>({ in->op, left, right });
            if (isComparisonOp(in->op)) {
                e->type = BoolType.vectorType(lType->vectorSize);
                if (lType->isLogical)
                    VERIFY(ShaderGen, in->op == BinaryOperator::CMP_EQ || in->op == BinaryOperator::CMP_NE);
            } else {
                e->type = lType;
                if (isLogicalOp(in->op))
                    VERIFY(ShaderGen, lType->isLogical);
                else if (isArithmeticOp(in->op))
                    VERIFY(ShaderGen, lType->isArithmetic);
            }
            e->valueKind = ValueKind::RVALUE;
            return e;
        }
        constexpr Stmt* visitVectorByScalarOperatorExpr(const VectorByScalarOperatorExpr* in) {
            auto* vector = castStmt<Expr>(dispatchVisit(in->vector));
            auto* scalar = castStmt<Expr>(dispatchVisit(in->scalar));
            const auto* vType = asVectorType(vector->type);
            const auto* sType = asScalarType(scalar->type);
            VERIFY(ShaderGen, isArithmeticOp(in->op));
            VERIFY(ShaderGen, vType->scalarType == sType);
            VERIFY(ShaderGen, vType->isArithmetic);
            auto* e = ast.allocateStmt<VectorByScalarOperatorExpr>({ in->op, vector, scalar, in->isVectorLeft });
            e->type = vType;
            return e;
        }
        constexpr Stmt* visitLiteralExpr(const LiteralExpr* in) {
            auto* e = ast.allocateStmt<LiteralExpr>({ in->literal });
            e->type = in->type;
            e->valueKind = ValueKind::RVALUE;
            return e;
        }
        constexpr Stmt* visitAssignmentStmt(const AssignmentStmt* in) {
            auto* left = castStmt<Expr>(dispatchVisit(in->left));
            auto* right = castStmt<Expr>(dispatchVisit(in->right));
            VERIFY(ShaderGen, left->type == right->type);
            VERIFY(ShaderGen, left->valueKind == ValueKind::LVALUE);
            CheckInitialized check;
            check.visit(right);
            MarkInitialized mark;
            mark.visit(left);
            return ast.allocateStmt<AssignmentStmt>({ left, right });
        }
    };

    constexpr void buildStmt(const Stmt* in) {
        StmtBuilder builder { {}, *this };
        Stmt* stmt = builder.dispatchVisit(in);
        emitStmt(stmt);
    }

    constexpr void emitStmt(Stmt* stmt) {
        pushBackOnList(*castStmt<CompoundStmt>(currentBlock->body), stmt);
    }

    constexpr void beginBlock() {
        auto* newBlock = this->newBlock();
        newBlock->parent = currentBlock;
        emitStmt(newBlock);
        currentBlock = newBlock;
    }
    constexpr void endBlock() {
        currentBlock->declContext.decls.forEachEntry([](const VarDecl* decl) {
            VERIFY(ShaderGen, decl->usedMask != 0);
        });
        currentBlock = currentBlock->parent;
    }
};

// AST helper
struct WrappedAST : AST {
    template<typename Callback>
    constexpr explicit WrappedAST(Callback&& callback) { callback(*this); }
    constexpr wrapped<VarRefExpr, void> declareScalar(std::string_view name, const Type* type) {
        return { AST::declareVariable(name, type) };
    }
    constexpr wrapped<VarRefExpr, vec_t> declareVector(std::string_view name, const VectorType* type) {
        return { AST::declareVariable(name, type) };
    }
    constexpr void operator,(const Stmt& in) {
        buildStmt(&in);
    }
};
}
