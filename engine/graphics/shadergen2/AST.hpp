#pragma once

#include "ASTVisitor.hpp"

namespace shadergen {

// Storage
union StmtStorage {
#define AST_NODE(N) N N##_;
    ENUMERATE_AST_NODES(AST_NODE)
#undef AST_NODE

    constexpr Stmt& set(const Stmt& s) {
        switch (s.stmtKind) {
#define STMT_KIND(S)  \
    case StmtKind::S: \
        return *std::construct_at(&(S##_), (const S&)s);

            ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND
        default:
            VERIFY_NOT_REACHED(ShaderGen);
        }
    }
    constexpr CompoundStmtEntry& setCompoundStmtEntry(const CompoundStmtEntry& e) {
        return *std::construct_at(&CompoundStmtEntry_, e);
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
    constexpr Stmt& pushBack(const Stmt& stmt) {
        return acquireBack().set(stmt);
    }
};

struct AST {
    StmtVector<100> storage;
    BlockStmt* currentBlock = nullptr;

    template<typename T>
    constexpr T& allocate(const T& stmt) {
        return (T&)storage.pushBack(stmt);
    }

    constexpr AST()
        : currentBlock(&allocate<BlockStmt>({})) { }
    constexpr AST(const AST&) = delete;

    constexpr VarRefExpr declareVariable(std::string_view name, const Type* type) {
        auto& decl = allocate<VarDeclStmt>({ name, type });
        currentBlock->pushBack(*this, decl);
        return { decl };
    }

    struct StmtBuilder : ASTVisitor<StmtBuilder, Stmt&> {
        AST& ast;
        constexpr Stmt& visitStmt(const Stmt& stmt) {
            VERIFY_NOT_REACHED(ShaderGen);
            return ast.allocate<Stmt>({});
        }
        constexpr Stmt& visitVarRefExpr(const VarRefExpr& in) {
            auto& e = ast.allocate<VarRefExpr>({ in.decl });
            e.type = in.decl.type;
            return e;
        }
        constexpr Stmt& visitMemberAccessExpr(const MemberAccessExpr& in) {
            auto& base = castStmt<Expr>(dispatchVisit(in.base));
            const auto* baseType = asVectorType(base.type);
            auto& e = ast.allocate<MemberAccessExpr>({ base, in.memberIndex });
            e.type = baseType->scalarType;
            return e;
        }
        constexpr Stmt& visitUnaryOperatorExpr(const UnaryOperatorExpr& in) {
            auto& subExpr = castStmt<Expr>(dispatchVisit(in.subExpr));
            const auto* type = asVectorType(subExpr.type);
            if (isLogicalOp(in.op))
                VERIFY(ShaderGen, type->isLogical);
            else if (isArithmeticOp(in.op))
                VERIFY(ShaderGen, type->isArithmetic);
            auto& e = ast.allocate<UnaryOperatorExpr>({ in.op, subExpr });
            e.type = type;
            return e;
        }
        constexpr Stmt& visitElementwiseOperatorExpr(const ElementwiseOperatorExpr& in) {
            auto& left = castStmt<Expr>(dispatchVisit(in.left));
            auto& right = castStmt<Expr>(dispatchVisit(in.right));
            const auto* lType = asVectorType(left.type);
            const auto* rType = asVectorType(right.type);
            VERIFY(ShaderGen, lType->scalarType == rType->scalarType);
            VERIFY(ShaderGen, lType->vectorSize == rType->vectorSize);
            auto& e = ast.allocate<ElementwiseOperatorExpr>({ in.op, left, right });
            if (isComparisonOp(in.op)) {
                e.type = BoolType.vectorType(lType->vectorSize);
                if (lType->isLogical)
                    VERIFY(ShaderGen, in.op == BinaryOperator::CMP_EQ || in.op == BinaryOperator::CMP_NE);
            } else {
                e.type = lType;
                if (isLogicalOp(in.op))
                    VERIFY(ShaderGen, lType->isLogical);
                else if (isArithmeticOp(in.op))
                    VERIFY(ShaderGen, lType->isArithmetic);
            }
            return e;
        }
        constexpr Stmt& visitVectorByScalarOperatorExpr(const VectorByScalarOperatorExpr& in) {
            auto& vector = castStmt<Expr>(dispatchVisit(in.vector));
            auto& scalar = castStmt<Expr>(dispatchVisit(in.scalar));
            const auto* vType = asVectorType(vector.type);
            const auto* sType = asScalarType(scalar.type);
            VERIFY(ShaderGen, isArithmeticOp(in.op));
            VERIFY(ShaderGen, vType->scalarType == sType);
            VERIFY(ShaderGen, vType->isArithmetic);
            auto& e = ast.allocate<VectorByScalarOperatorExpr>({ in.op, vector, scalar, in.isVectorLeft });
            e.type = vType;
            return e;
        }
        constexpr Stmt& visitLiteralExpr(const LiteralExpr& in) {
            auto& e = ast.allocate<LiteralExpr>({ in.literal });
            e.type = in.type;
            return e;
        }
        constexpr Stmt& visitAssignmentStmt(const AssignmentStmt& in) {
            auto& left = castStmt<Expr>(dispatchVisit(in.left));
            auto& right = castStmt<Expr>(dispatchVisit(in.right));
            VERIFY(ShaderGen, left.type == right.type);
            return ast.allocate<AssignmentStmt>({ left, right });
        }
    };

    constexpr Stmt& emitStmt(const Stmt& in)  {
        StmtBuilder builder { {}, *this };
        Stmt& stmt = builder.dispatchVisit(in);
        currentBlock->pushBack(*this, stmt);
        return stmt;
    }
};

// Compound statement implementation
constexpr CompoundStmtEntry& CompoundStmt::pushBack(AST& ast, Stmt& stmt) {
    CompoundStmtEntry& newEntry = ast.storage.acquireBack().setCompoundStmtEntry({ stmt });

    if (last)
        last->next = &newEntry;
    else
        first = &newEntry;

    last = &newEntry;
    childCount += 1;
    return newEntry;
}

struct WrappedAST : AST {
    template<typename Callback>
    constexpr explicit WrappedAST(Callback&& callback) { callback(*this); }
    constexpr wrapped<VarRefExpr, void> declareScalar(std::string_view name, const Type* type) {
        return { AST::declareVariable(name, type) };
    }
    constexpr wrapped<VarRefExpr, vec_t> declareVector(std::string_view name, const VectorType* type) {
        return { AST::declareVariable(name, type) };
    }
};
}
