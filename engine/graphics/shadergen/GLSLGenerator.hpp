#pragma once

#include "ASTVisitor.hpp"

namespace shadergen {

struct GLSLStmtTransform : CheckedASTTransform<GLSLStmtTransform> {
    using Base = CheckedASTTransform<GLSLStmtTransform>;
    constexpr GLSLStmtTransform() = default;
    constexpr GLSLStmtTransform(ASTStorage storage)
        : Base(storage) { }

    constexpr Expr* wrapWithParenExpr(const Expr* e) {
        auto* paren = storage.allocateStmt<ParenExpr>({ e });
        paren->type = e->type;
        paren->valueKind = e->valueKind;
        return paren;
    }
    constexpr void transformElementwiseOperatorExpr(ElementwiseOperatorExpr* e) {
        Base::transformElementwiseOperatorExpr(e);
        int_t myPrecedence = operatorPrecedence(e->op);
        if (myPrecedence <= resolveOperatorPrecedence(e->left))
            e->left = wrapWithParenExpr(e->left);
        if (myPrecedence < resolveOperatorPrecedence(e->right))
            e->right = wrapWithParenExpr(e->right);
    }
    constexpr void transformVectorByScalarOperatorExpr(VectorByScalarOperatorExpr* e) {
        Base::transformVectorByScalarOperatorExpr(e);
        int_t myPrecedence = operatorPrecedence(e->op);
        int_t vectorPrecedence = resolveOperatorPrecedence(e->vector);
        if (myPrecedence < vectorPrecedence || (!e->isVectorLeft && myPrecedence == vectorPrecedence))
            e->vector = wrapWithParenExpr(e->vector);
        int_t scalarPrecedence = resolveOperatorPrecedence(e->scalar);
        if (myPrecedence < scalarPrecedence || (e->isVectorLeft && myPrecedence == scalarPrecedence))
            e->scalar = wrapWithParenExpr(e->scalar);
    }
    constexpr void transformMatrixMultiplicationExpr(MatrixMultiplicationExpr* e) {
        Base::transformMatrixMultiplicationExpr(e);
        int_t myPrecedence = operatorPrecedence(BinaryOperator::MULTIPLY);
        if (myPrecedence <= resolveOperatorPrecedence(e->left))
            e->left = wrapWithParenExpr(e->left);
        if (myPrecedence < resolveOperatorPrecedence(e->right))
            e->right = wrapWithParenExpr(e->right);
    }
    constexpr void transformUnaryOperatorExpr(UnaryOperatorExpr* e) {
        Base::transformUnaryOperatorExpr(e);
        if (1 < resolveOperatorPrecedence(e->subExpr))
            e->subExpr = wrapWithParenExpr(e->subExpr);
    }
    constexpr void transformMemberAccessExpr(MemberAccessExpr* e) {
        Base::transformMemberAccessExpr(e);
        if (1 < resolveOperatorPrecedence(e->base))
            e->base = wrapWithParenExpr(e->base);
    }
    constexpr void transformVectorComponentExpr(VectorComponentExpr* e) {
        Base::transformVectorComponentExpr(e);
        if (1 < resolveOperatorPrecedence(e->base))
            e->base = wrapWithParenExpr(e->base);
    }

    constexpr const Function* binaryToFuntion(BinaryOperator op) {
        using namespace functions;
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
    constexpr Stmt* createElementwiseOperatorExpr(const ElementwiseOperatorExpr& e) {
        if ((isComparisonOp(e.op) || isLogicalOp(e.op)) && !e.type->isScalar)
            return this->createCallExpr(UnresolvedCall<2> { binaryToFuntion(e.op), { e.left, e.right } });
        return Base::createElementwiseOperatorExpr(e);
    }
    constexpr const Function* unaryToFunction(UnaryOperator e) {
        using namespace functions;
        VERIFY(ShaderGen, e == UnaryOperator::NOT);
        return &notFunc;
    }
    constexpr Stmt* createUnaryOperatorExpr(const UnaryOperatorExpr& e) {
        if (isLogicalOp(e.op) && !e.type->isScalar)
            return this->createCallExpr(UnresolvedCall<1> { unaryToFunction(e.op), { e.subExpr } });
        return Base::createUnaryOperatorExpr(e);
    }
};

template<typename Impl>
struct GLSLGeneratorBase {
    struct Indentation { };
    using OutRet = Impl;

    PseudoVector<char> buffer;
    int_t indent = 0;
    constexpr GLSLGeneratorBase() = default;
    constexpr GLSLGeneratorBase(PseudoVector<char> buffer)
        : buffer(buffer) { }

    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    constexpr OutRet& out() { return *static_cast<Impl*>(this); }
    constexpr OutRet& operator<<(std::string_view str) {
        std::copy_n(str.data(), str.size(), buffer.acquireEnd(str.size()));
        return out();
    }
    constexpr OutRet& operator<<(const char* str) {
        return *this << std::string_view(str);
    }
    constexpr OutRet& operator<<(const Type* type) {
        emitTypeName(type);
        return out();
    }
    constexpr OutRet& operator<<(const VarDecl* decl) {
        emitVarDecl(decl);
        return out();
    }
    constexpr OutRet& operator<<(Indentation) {
        for (int_t i = 0; i < indent * 4; i++)
            *this << " ";
        return out();
    }
    constexpr OutRet& operator<<(bool b) {
        return *this << (b ? "true" : "false");
    }

    constexpr void emitScalarTypeName(const ScalarType* type) {
        std::string_view typeName = "invalid_scalar_type";
        if (type->scalarType == &BoolType)
            typeName = "bool";
        if (type->scalarType == &IntType)
            typeName = "int";
        if (type->scalarType == &FloatType)
            typeName = "float";
        out() << typeName;
    }
    constexpr void emitVectorTypeName(const VectorType* type) {
        if (type->isScalar) {
            emitScalarTypeName(asScalarType(type));
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
    constexpr void emitMatrixTypeName(const MatrixType* type) {
        if (type->isVector) {
            emitVectorTypeName(asVectorType(type));
            return;
        }
        std::string_view typeName = "invalid_matrix_type";
        if (type->scalarType == &BoolType)
            typeName = "bmat";
        if (type->scalarType == &IntType)
            typeName = "imat";
        if (type->scalarType == &FloatType)
            typeName = "mat";
        char sizeBuf[3];
        sizeBuf[0] = '0' + type->vectorSize;
        sizeBuf[1] = 'x';
        sizeBuf[2] = '0' + type->covectorSize;
        out() << typeName << std::string_view(sizeBuf, type->vectorSize == type->covectorSize ? 1 : 3);
    }
    constexpr void emitUserTypeName(const UserType* type) {
        out() << type->name;
    }
    constexpr void emitTypeName(const Type* type) {
        if (type->isMatrix)
            emitMatrixTypeName(asMatrixType(type));
        else if (type->isUser)
            emitUserTypeName(asUserType(type));
        else
            out() << "invalid_type";
    }
    constexpr void emitVarDecl(const VarDecl* decl) {
        out() << decl->type << " ";
        impl()->emitVarName(decl);
    }
    constexpr void emitVarName(const VarDecl* decl) {
        out() << decl->name;
    }
};

template<typename Impl>
struct GLSLStmtGenerator : GLSLGeneratorBase<Impl>, ASTVisitor<Impl> {
    using Base = ASTVisitor<Impl>;
    using GLSLBase = GLSLGeneratorBase<Impl>;
    using GLSLBase::GLSLBase;
    using GLSLBase::operator<<;
    using Indentation = typename GLSLBase::Indentation;
    using OutRet = typename GLSLBase::OutRet;

    constexpr OutRet& operator<<(const Stmt* stmt) {
        this->dispatchVisit(stmt);
        return out();
    }
    constexpr OutRet& out() { return GLSLBase::out(); }

    constexpr void visitVarRefExpr(const VarRefExpr* e) {
        GLSLBase::impl()->emitVarName(e->decl);
    }
    constexpr void visitElementwiseOperatorExpr(const ElementwiseOperatorExpr* e) {
        out() << e->left << " " << toString(e->op) << " " << e->right;
    }
    constexpr void visitVectorByScalarOperatorExpr(const VectorByScalarOperatorExpr* e) {
        out() << (e->isVectorLeft ? e->vector : e->scalar) << " " << toString(e->op) << " " << (e->isVectorLeft ? e->scalar : e->vector);
    }
    constexpr void visitMatrixMultiplicationExpr(const MatrixMultiplicationExpr* e) {
        out() << e->left << " * " << e->right;
    }
    constexpr void visitUnaryOperatorExpr(const UnaryOperatorExpr* e) {
        out() << toString(e->op) << e->subExpr;
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
                out() << ", ";
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
        this->indent += 1;
        e->declContext.decls.forEachEntry([&](const VarDecl* decl) {
            out() << Indentation() << decl << ";\n";
        });
        out() << e->body;
        this->indent -= 1;
        out() << Indentation() << "}";
    }
    constexpr void visitParenExpr(const ParenExpr* e) {
        out() << "(" << e->subExpr << ")";
    }
    constexpr void visitStageDecl(const StageDecl* stage) {
        stage->uniforms.decls.forEachEntry([&](const VarDecl* decl) {
            out() << "uniform " << decl << ";\n";
        });
        stage->inputs.decls.forEachEntry([&](const VarDecl* decl) {
            out() << "in " << decl << ";\n";
        });
        stage->outputs.decls.forEachEntry([&](const VarDecl* decl) {
            out() << "out " << decl << ";\n";
        });
        out() << "void main() " << stage->entryPoint << "\n";
    }
};

struct GLSLTypeGenerator : GLSLGeneratorBase<GLSLTypeGenerator> {
    using GLSLGeneratorBase::GLSLGeneratorBase;
    using GLSLGeneratorBase::operator<<;

    constexpr void visitType(const UserType* type) {
        out() << "struct " << type << " {\n";
        indent += 1;
        for (auto member : type->members)
            out() << Indentation() << member.type << " " << member.name << ";\n";
        indent -= 1;
        out() << "};\n";
    }
};

struct GLSLStageGenerator : GLSLStmtGenerator<GLSLStageGenerator> {
    using Base = GLSLStmtGenerator<GLSLStageGenerator>;
    using Base::Base;

    constexpr void visitStage(const StageDriver& stage) {
        visitStageDecl(stage.stageDecl());
    }
};

}
