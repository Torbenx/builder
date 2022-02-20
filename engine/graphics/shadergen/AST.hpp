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

struct AST : ASTTransform<AST> {
    using Base = ASTTransform<AST>;
    StmtVector<100> storage;
    // BlockStmt* currentBlock = nullptr;

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

    constexpr AST() {
        currentBlock = newBlock();
    }
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

    constexpr void transformVarRefExpr(VarRefExpr* e) {
        BlockStmt* block = currentBlock;
        while (block && (&block->declContext != e->decl->declContext))
            block = block->parent;
        VERIFY(ShaderGen, block);

        e->type = e->decl->type;
        e->valueKind = ValueKind::LVALUE;
    }
    constexpr void transformMemberAccessExpr(MemberAccessExpr* e) {
        Base::transformMemberAccessExpr(e);
        VERIFY(ShaderGen, e->memberIndex < e->base->type->members.size());
        const MemberInfo& member = e->base->type->members[e->memberIndex];
        e->type = member.type;
        e->valueKind = e->base->valueKind;
    }
    constexpr void transformVectorComponentExpr(VectorComponentExpr* e) {
        Base::transformVectorComponentExpr(e);
        const auto* baseType = asVectorType(e->base->type);
        VERIFY(ShaderGen, !baseType->isScalar);
        size_t mask = 0;
        bool hasDuplicates = false;
        for (int_t i = 0; i < e->componentCount; i++) {
            VERIFY(ShaderGen, e->components[i] < baseType->vectorSize);
            size_t bit = (size_t)1 << e->components[i];
            if (mask & bit)
                hasDuplicates = true;
            mask |= bit;
        }
        e->type = baseType->scalarType->vectorType(e->componentCount);
        e->valueKind = hasDuplicates ? ValueKind::RVALUE : e->base->valueKind;
    }
    constexpr void transformUnaryOperatorExpr(UnaryOperatorExpr* e) {
        Base::transformUnaryOperatorExpr(e);
        const auto* type = asVectorType(e->subExpr->type);
        if (isLogicalOp(e->op))
            VERIFY(ShaderGen, type->isLogical);
        else if (isArithmeticOp(e->op))
            VERIFY(ShaderGen, type->isArithmetic);
        e->type = type;
        e->valueKind = ValueKind::RVALUE;
    }
    constexpr void transformElementwiseOperatorExpr(ElementwiseOperatorExpr* e) {
        Base::transformElementwiseOperatorExpr(e);
        const auto* lType = asVectorType(e->left->type);
        const auto* rType = asVectorType(e->right->type);
        VERIFY(ShaderGen, lType->scalarType == rType->scalarType);
        VERIFY(ShaderGen, lType->vectorSize == rType->vectorSize);
        if (isComparisonOp(e->op)) {
            e->type = BoolType.vectorType(lType->vectorSize);
            if (lType->isLogical)
                VERIFY(ShaderGen, e->op == BinaryOperator::CMP_EQ || e->op == BinaryOperator::CMP_NE);
        } else {
            e->type = lType;
            if (isLogicalOp(e->op))
                VERIFY(ShaderGen, lType->isLogical);
            else if (isArithmeticOp(e->op))
                VERIFY(ShaderGen, lType->isArithmetic);
        }
        e->valueKind = ValueKind::RVALUE;
    }
    constexpr void transformVectorByScalarOperatorExpr(VectorByScalarOperatorExpr* e) {
        Base::transformVectorByScalarOperatorExpr(e);
        const auto* vType = asVectorType(e->vector->type);
        const auto* sType = asScalarType(e->scalar->type);
        VERIFY(ShaderGen, isArithmeticOp(e->op));
        VERIFY(ShaderGen, vType->scalarType == sType);
        VERIFY(ShaderGen, vType->isArithmetic);
        e->type = vType;
        e->valueKind = ValueKind::RVALUE;
    }
    constexpr void transformLiteralExpr(LiteralExpr* e) {
        Base::transformLiteralExpr(e);
        e->type = e->type;
        e->valueKind = ValueKind::RVALUE;
    }
    constexpr void transformAssignmentStmt(AssignmentStmt* e) {
        Base::transformAssignmentStmt(e);
        VERIFY(ShaderGen, e->left->type == e->right->type);
        VERIFY(ShaderGen, e->left->valueKind == ValueKind::LVALUE);
        CheckInitialized check;
        check.visit(e->right);
        MarkInitialized mark;
        mark.visit(e->left);
    }

    constexpr void buildStmt(const Stmt* in) {
        Stmt* stmt = dispatchTransform(in);
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
