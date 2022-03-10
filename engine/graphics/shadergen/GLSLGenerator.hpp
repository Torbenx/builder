#pragma once

#include "ASTVisitor.hpp"

namespace shadergen {

template<typename Storage>
struct GLSLStmtTransform : CheckedASTTransform<GLSLStmtTransform<Storage>, Storage> {
    using Base = CheckedASTTransform<GLSLStmtTransform<Storage>, Storage>;
    constexpr GLSLStmtTransform() = default;
    constexpr GLSLStmtTransform(Storage* storage)
        : Base(storage) { }

    constexpr const Function* binaryToFuntion(BinaryOperator op) {
        switch (op) {
        case BinaryOperator::CMP_LE:
            return &lessThanEqualFunc;
        case BinaryOperator::CMP_L:
            return &lessThanFunc;
        case BinaryOperator::CMP_EQ:
            return &equalFunc;
        case BinaryOperator::CMP_GE:
            return &greaterThanEqualFunc;
        case BinaryOperator::CMP_G:
            return &greaterThanFunc;
        case BinaryOperator::CMP_NE:
            return &notEqualFunc;
        case BinaryOperator::AND:
            return &andFunc;
        case BinaryOperator::OR:
            return &orFunc;
        default:
            VERIFY_NOT_REACHED(ShaderGen);
            return nullptr;
        }
    }
    constexpr const Function* unaryToFunction(UnaryOperator e) {
        VERIFY(ShaderGen, e == UnaryOperator::NOT);
        return &notFunc;
    }
    constexpr Stmt* createElementwiseOperatorExpr(const ElementwiseOperatorExpr& e) {
        if ((isComparisonOp(e.op) || isLogicalOp(e.op)) && !e.type->isScalar)
            return this->createCallExpr(UnresolvedCall<2> { binaryToFuntion(e.op), { e.left, e.right } });
        return Base::createElementwiseOperatorExpr(e);
    }
    constexpr Stmt* createUnaryOperatorExpr(const UnaryOperatorExpr& e) {
        if (isLogicalOp(e.op) && !e.type->isScalar)
            return this->createCallExpr(UnresolvedCall<1> { unaryToFunction(e.op), { e.subExpr } });
        return Base::createUnaryOperatorExpr(e);
    }
};

struct GLSLStmtGenerator : ASTVisitor<GLSLStmtGenerator> {
    using Base = ASTVisitor<GLSLStmtGenerator>;
    struct Indentation { };

    char* end = nullptr;
    int_t indent = 0;
    constexpr GLSLStmtGenerator() = default;
    constexpr GLSLStmtGenerator(char* buffer)
        : end(buffer) { }

    constexpr auto& out() { return *this; }
    constexpr auto& operator<<(std::string_view str) {
        std::copy_n(str.data(), str.size(), end);
        end += str.size();
        return *this;
    }
    constexpr auto& operator<<(const char* str) {
        return *this << std::string_view(str);
    }
    constexpr auto& operator<<(const Stmt* stmt) {
        dispatchVisit(stmt);
        return *this;
    }
    constexpr auto& operator<<(const Type* type) {
        visitType(type);
        return *this;
    }
    constexpr auto& operator<<(Indentation) {
        for (int_t i = 0; i < indent * 4; i++)
            *this << " ";
        return *this;
    }
    constexpr auto& operator<<(bool b) {
        return *this << (b ? "true" : "false");
    }

    constexpr void visitScalarType(const ScalarType* type) {
        std::string_view typeName = "invalid_scalar_type";
        if (type->scalarType == &BoolType)
            typeName = "bool";
        if (type->scalarType == &IntType)
            typeName = "int";
        if (type->scalarType == &FloatType)
            typeName = "float";
        out() << typeName;
    }
    constexpr void visitVectorType(const VectorType* type) {
        if (type->isScalar) {
            visitScalarType(asScalarType(type));
            return;
        }
        std::string_view typeName = "invalid_vector_type";
        if (type->scalarType == &BoolType)
            typeName = "bvec";
        if (type->scalarType == &IntType)
            typeName = "ivec";
        if (type->scalarType == &FloatType)
            typeName = "vec";
        char sizeChar = '0' + type->vectorSize;
        out() << typeName << std::string_view(&sizeChar, 1);
    }
    constexpr void visitUserType(const UserType* type) {
        out() << type->name;
    }
    constexpr void visitType(const Type* type) {
        if (type->isVector)
            visitVectorType(asVectorType(type));
        else if (type->isUser)
            visitUserType(asUserType(type));
        else
            out() << "invalid_type";
    }

    constexpr void visitVarRefExpr(const VarRefExpr* e) {
        out() << e->decl->name;
    }
    constexpr void visitVarDecl(const VarDecl* decl) {
        out() << decl->type << " " << decl->name;
    }
    constexpr std::string_view binaryToFuntion(BinaryOperator op) {
        switch (op) {
        case BinaryOperator::CMP_LE:
            return "lessThanEqual";
        case BinaryOperator::CMP_L:
            return "lessThan";
        case BinaryOperator::CMP_EQ:
            return "equal";
        case BinaryOperator::CMP_GE:
            return "greaterThanEqual";
        case BinaryOperator::CMP_G:
            return "greaterThan";
        case BinaryOperator::CMP_NE:
            return "notEqual";
        case BinaryOperator::AND:
            return "and";
        case BinaryOperator::OR:
            return "or";
        default:
            return "invalid_cmp_op";
        }
    }
    constexpr void visitElementwiseOperatorExpr(const ElementwiseOperatorExpr* e) {
        if ((isComparisonOp(e->op) || isLogicalOp(e->op)) && !e->type->isScalar)
            out() << binaryToFuntion(e->op) << "(" << e->left << ", " << e->right << ")";
        else
            out() << "(" << e->left << " " << toString(e->op) << " " << e->right << ")";
    }
    constexpr void visitVectorByScalarOperatorExpr(const VectorByScalarOperatorExpr* e) {
        out() << "(" << (e->isVectorLeft ? e->vector : e->scalar) << " " << toString(e->op) << " " << (e->isVectorLeft ? e->scalar : e->vector) << ")";
    }
    constexpr std::string_view unaryToFunction(UnaryOperator e) {
        VERIFY(ShaderGen, e == UnaryOperator::NOT);
        return "not";
    }
    constexpr void visitUnaryOperatorExpr(const UnaryOperatorExpr* e) {
        if (isLogicalOp(e->op) && !e->type->isScalar)
            out() << unaryToFunction(e->op) << "(" << e->subExpr << ")";
        else
            out() << "(" << toString(e->op) << e->subExpr << ")";
    }
    constexpr void visitAssignmentStmt(const AssignmentStmt* e) {
        out() << e->left << " = " << e->right;
    }
    constexpr void visitUpdateStmt(const UpdateStmt* e) {
        out() << e->left << " " << toString(e->op) << "= " << e->right;
    }
    constexpr void visitLiteralExpr(const LiteralExpr* e) {
        const auto* type = asVectorType(e->type);
        out() << type << "(";
        if (type->scalarType == &FloatType) {
            out() << "0.0";
            for (int_t i = 1; i < type->vectorSize; i++)
                out() << ", "
                      << "0.0";
        }
        if (type->scalarType == &IntType) {
            out() << "0";
            for (int_t i = 1; i < type->vectorSize; i++)
                out() << ", "
                      << "0";
        }
        if (type->scalarType == &BoolType) {
            out() << e->literal.bools[0];
            for (int_t i = 1; i < type->vectorSize; i++)
                out() << ", " << e->literal.bools[i];
        }
        out() << ")";
    }
    constexpr void visitCompoundStmt(const CompoundStmt* e) {
        e->forEachEntry([&](const Stmt* stmt) {
            out() << Indentation() << stmt << ";\n";
        });
    }
    constexpr void visitVectorComponentExpr(const VectorComponentExpr* e) {
        constexpr char comps[] = { 'x', 'y', 'z', 'w' };
        out() << e->base << ".";
        for (int_t i = 0; i < e->componentCount; i++)
            out() << std::string_view(&comps[e->components[i]], 1);
    }
    constexpr void visitCompositionExpr(const CompositionExpr* e) {
        if (e->type->isScalar)
            return;
        out() << e->type << "(" << e->components[0];
        for (int_t i = 1; i < e->componentCount; i++)
            out() << ", " << e->components[i];
        out() << ")";
    }
    constexpr void visitMemberAccessExpr(const MemberAccessExpr* e) {
        out() << e->base << "." << asUserType(e->base->type)->members[e->memberIndex].name;
    }
    constexpr void visitCallExpr(const CallExpr* e) {
        out() << e->function->name << "(";
        bool first = true;
        e->arguments.forEachEntry([&](const Expr* expr) {
            if (!first)
                out() << " ,";
            first = false;
            out() << expr;
        });
        out() << ")";
    }
    constexpr void visitIfStmt(const IfStmt* e) {
        out() << "if (" << e->condition << ") " << e->whenTrue;
        if (e->whenFalse)
            out() << " else " << e->whenFalse;
    }
    constexpr void visitBlockStmt(const BlockStmt* e) {
        out() << "{\n";
        indent += 1;
        out() << e->body;
        indent -= 1;
        out() << Indentation() << "}";
    }
};

struct GLSLStageGenerator : GLSLStmtGenerator {
    constexpr GLSLStageGenerator() = default;
    constexpr GLSLStageGenerator(char* buffer)
        : GLSLStmtGenerator(buffer) { }

    constexpr void visitStage(const StageDriver& stage) {
        stage.stageDecl->uniforms.decls.forEachEntry([&](const VarDecl* decl) {
            out() << "uniform ";
            visitVarDecl(decl);
            out() << ";\n";
        });
        out() << "void main() ";
        dispatchVisit(stage.stageDecl->entryPoint);
    }
};

}
