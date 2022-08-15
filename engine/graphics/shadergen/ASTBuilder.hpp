#pragma once

#include "ASTVisitor.hpp"

namespace shadergen {

struct MessageBuilder {
    std::string_view* message = nullptr;
    PsudoVector<char> storage;

    constexpr MessageBuilder() = default;
    constexpr MessageBuilder(std::string_view& message, PsudoVector<char> storage)
        : message(&message), storage(storage) { }

    constexpr void setEmpty() {
        message = nullptr;
        storage = {};
    }
    constexpr void grow(int_t size) {
        *storage.endPtr += size;
        *message = { message->data(), message->size() + size };
    }
    constexpr MessageBuilder& operator<<(std::string_view str) {
        if (!message)
            return *this;
        std::copy_n(str.data(), str.size(), storage.end());
        grow(str.size());
        return *this;
    }
    constexpr MessageBuilder& operator<<(int_t in) {
        if (!message)
            return *this;
        while (in != 0) {
            int_t mod = in % 10;
            int_t div = in / 10;
            *storage.end() = '0' + mod;
            grow(1);
            in = div;
        }
        return *this;
    }
    constexpr MessageBuilder& operator<<(const Type* type) {
        *this << "'";
        formatType(type, *this);
        *this << "'";
        return *this;
    }
    constexpr MessageBuilder& operator<<(const VarDecl* decl) {
        return *this << "\"" << decl->name << "\"";
    }
    constexpr MessageBuilder& operator<<(const VarRefExpr* e) {
        return *this << e->decl;
    }
};
struct ErrorBuilder : MessageBuilder {
    ErrorNode* error = nullptr;
    constexpr ErrorBuilder() = default;
    constexpr ErrorBuilder(ErrorNode& error, PsudoVector<char> storage)
        : MessageBuilder(error.message, storage), error(&error) { }
    constexpr void setEmpty() {
        MessageBuilder::setEmpty();
        error = nullptr;
    }
    constexpr ErrorBuilder& setCausingSubExpr(const Expr* expr) {
        if (!error)
            return *this;
        error->causingSubExpr = expr;
        return *this;
    }
};

struct AccessMaskUtils {
    static constexpr FixedBitset fullMask(const Type* type) {
        if (type->isVector)
            return FixedBitset::bitsBelow(asVectorType(type)->vectorSize);
        else if (type->isUser)
            return FixedBitset::bitsBelow(asUserType(type)->leafFieldCount);
        else {
            return {};
        }
    }
};

template<typename Impl>
struct AccessMaskVisitor : ASTVisitor<Impl, void, FixedBitset> {
    constexpr Impl* impl() { return static_cast<Impl*>(this); }

    constexpr void dispatchVisitWithFullMask(const Expr* e) {
        impl()->dispatchVisit(e, AccessMaskUtils::fullMask(e->type));
    }
    constexpr void visitExpr(const Expr*, FixedBitset) { }
    constexpr void visitMemberAccessExpr(const MemberAccessExpr* e, FixedBitset mask) {
        impl()->dispatchVisit(e->base, mask << e->member().leafFieldOffset);
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
        impl()->dispatchVisitWithFullMask(e->scalar);
        impl()->visitExpr(e, mask);
    }
    constexpr void visitCallExpr(const CallExpr* e, FixedBitset mask) {
        e->arguments.forEachEntry([&](const Expr* arg) {
            impl()->dispatchVisitWithFullMask(arg); // be more accurate?
        });
        impl()->visitExpr(e, mask);
    }
    constexpr void visitCompositionExpr(const CompositionExpr* e, FixedBitset mask) {
        for (int_t i = 0; i < e->componentCount; i++)
            impl()->dispatchVisitWithFullMask(e->components[i]); // be more accurate?
        impl()->visitExpr(e, mask);
    }
    constexpr void visit(const Expr* e) {
        impl()->dispatchVisitWithFullMask(e);
    }
};

template<typename Impl>
struct CheckedASTTransform : ASTTransform<Impl> {
    using Base = ASTTransform<Impl>;

    constexpr CheckedASTTransform() = default;
    constexpr CheckedASTTransform(ASTStorage storage)
        : Base(storage) { }

    struct CheckInitialized : AccessMaskVisitor<CheckInitialized> {
        CheckedASTTransform& ast;
        constexpr CheckInitialized(CheckedASTTransform& ast)
            : ast(ast) { }
        constexpr void visitVarRefExpr(const VarRefExpr* e, FixedBitset mask) {
            ast.check(!(mask & ~e->decl->initMask)) << "variable " << e << " uninitialized when used";
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

    ErrorNode currentError;
    constexpr ErrorBuilder buildError(ErrorNode& error) {
        error.message = { this->storage.messageStorage.end(), 0 };
        return { error, this->storage.messageStorage };
    }
    constexpr ErrorBuilder error() {
        if (currentError.hasError())
            return {};
        return buildError(currentError);
    }
    constexpr MessageBuilder addNote(ErrorBuilder err) {
        if (!err.error)
            return {};
        auto* note = this->storage.allocateNote({});
        this->storage.pushBackOnList(err.error->notes, note);
        note->message = { this->storage.messageStorage.end(), 0 };
        return { note->message, this->storage.messageStorage };
    }
    constexpr ErrorBuilder check(bool cond) {
        if (!cond)
            return error();
        return {};
    }
    constexpr void checkSameType(const Type* left, const Type* right, std::string_view message) {
        check(left == right) << message << ", got " << left << " and " << right << "";
    }
    constexpr const VectorType* checkVectorExpr(const Expr* e) {
        if (e->type->isVector)
            return asVectorType(e->type);

        error() << "expected vector type, got " << e->type << "";
        return nullptr;
    }
    constexpr const ScalarType* checkScalarExpr(const Expr* e) {
        if (e->type->isScalar)
            return asScalarType(e->type);

        auto err = error();
        err << "expected scalar type, got " << e->type << "";
        err.setCausingSubExpr(e);
        return nullptr;
    }

    constexpr void transformVarRefExpr(VarRefExpr* e) {
        Base::transformVarRefExpr(e);
        e->type = e->decl->type;
        e->valueKind = ValueKind::LVALUE;
    }
    constexpr void transformMemberAccessExpr(MemberAccessExpr* e) {
        Base::transformMemberAccessExpr(e);
        const auto* type = asUserType(e->base->type);
        check(e->memberIndex < type->members.size()) << "member index out of range, " << e->memberIndex << " out of " << type->members.size();
        const MemberInfo& member = type->members[e->memberIndex];
        e->type = member.type;
        e->valueKind = e->base->valueKind;
    }
    constexpr void transformVectorComponentExpr(VectorComponentExpr* e) {
        Base::transformVectorComponentExpr(e);
        const auto* baseType = checkVectorExpr(e->base);
        if (!baseType)
            return;
        check(!baseType->isScalar) << "taking vector components of a scalar";
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
        const auto* type = checkVectorExpr(e->subExpr);
        if (!type)
            return;
        if (isLogicalOp(e->op))
            check(type->isLogical) << "logical operation requires boolean, got " << type << "";
        else if (isArithmeticOp(e->op))
            check(type->isArithmetic) << "arithmetic operation requires arithmetic type, got " << type << "";
        e->type = type;
        e->valueKind = ValueKind::RVALUE;
    }
    constexpr void transformElementwiseOperatorExpr(ElementwiseOperatorExpr* e) {
        Base::transformElementwiseOperatorExpr(e);
        const auto* lType = checkVectorExpr(e->left);
        const auto* rType = checkVectorExpr(e->right);
        if (!lType || !rType)
            return;
        checkSameType(lType, rType, "operands must have the same type");
        if (isComparisonOp(e->op)) {
            e->type = BoolType.vectorType(lType->vectorSize);
            if (lType->isLogical)
                check(e->op == BinaryOperator::CMP_EQ || e->op == BinaryOperator::CMP_NE) << "only '==' and '!=' allowed on booleans, got '" << toString(e->op) << "'";
        } else {
            e->type = lType;
            if (isLogicalOp(e->op))
                check(lType->isLogical) << "logical operation requires boolean, got " << lType << "";
            else if (isArithmeticOp(e->op))
                check(lType->isArithmetic) << "arithmetic operation requires arithmetic type, got " << lType << "";
        }
        e->valueKind = ValueKind::RVALUE;
    }
    constexpr void transformVectorByScalarOperatorExpr(VectorByScalarOperatorExpr* e) {
        Base::transformVectorByScalarOperatorExpr(e);
        const auto* vType = checkVectorExpr(e->vector);
        const auto* sType = checkScalarExpr(e->scalar);
        if (!vType || !sType)
            return;
        check(isArithmeticOp(e->op)) << "only arithmetic operations are allowed for vector by scalar operations, got '" << toString(e->op) << "'";
        checkSameType(vType->scalarType, sType, "operands must have the same base type");
        check(vType->isArithmetic) << "only arithmetic types allowed in vector by scalar operations, got " << vType << "";
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
        checkSameType(e->left->type, e->right->type, "operands must have the same type");
        check(e->left->valueKind == ValueKind::LVALUE) << "left hand side of assignment must be an lvalue";
        // CheckInitialized check(*this);
        // check.visit(e->right);
        // MarkInitialized mark;
        // mark.visit(e->left);
    }
    struct MatchFunctionResult {
        bool match;
        int_t dynamicSize;
    };
    constexpr MatchFunctionResult matchFunctionInstance(ASTList<const Expr>& arguments, const FunctionInstance* func, ErrorBuilder err = {}) {
        auto note = [&]() -> MessageBuilder {
            auto n = addNote(err);
            err = {};
            n << "instance not viable: ";
            return n;
        };
        if (arguments.entryCount != func->parameters.size()) {
            note() << "expected " << func->parameters.size() << " parameters but got " << arguments.entryCount;
            return { false, 0 };
        }
        int_t dynamicSize = 0;
        int_t dynamicSizeOrigin = 0;
        int_t i = 0;
        bool isMatch = true;
        arguments.forEachEntry([&](const Expr* arg) {
            const FunctionParameter& parm = func->parameters[i++];
            if (!parm.isDynamic) {
                if (arg->type != parm.u.fixed.type) {
                    note() << "parameter " << i << " expects " << parm.u.fixed.type << " but got " << arg->type << "";
                    isMatch = false;
                }
                return;
            }
            if (!arg->type->isVector) {
                note() << "parameter " << i << " expects vector type but got " << arg->type << "";
                isMatch = false;
                return;
            }
            const auto* vType = asVectorType(arg->type);
            if (vType->scalarType != parm.u.dynamic.scalarType) {
                note() << "parameter " << i << " expects vector of base type " << parm.u.dynamic.scalarType << " but got base type " << vType->scalarType << "";
                isMatch = false;
                return;
            }
            if (dynamicSize == 0) {
                dynamicSize = vType->vectorSize;
                dynamicSizeOrigin = i;
                return;
            }
            if (dynamicSize == vType->vectorSize)
                return;
            note() << "deduced vector size inconsistent, argument " << dynamicSizeOrigin << " decuced " << dynamicSize << " but argument " << i << " deduced " << vType->vectorSize;
            isMatch = false;
        });
        if (dynamicSize == 1 && !func->dynamicScalarAllowed)
            isMatch = false;
        return { isMatch, dynamicSize };
    }
    constexpr void transformCallExpr(CallExpr* e) {
        Base::transformCallExpr(e);
        e->valueKind = ValueKind::RVALUE;
        const FunctionInstance* func = nullptr;
        int_t dynamicSize = 0;
        if (!e->function->isGroup) {
            func = asFunctionInstance(e->function);
            auto [match, dynSize] = matchFunctionInstance(e->arguments, func);
            if (!match) {
                auto err = error();
                err << "invalid arguments to function call";
                matchFunctionInstance(e->arguments, func, err);
            }
            dynamicSize = dynSize;
        } else {
            const auto* group = asFunctionGroup(e->function);
            for (int_t i = 0; i < group->instances.size(); i++) {
                const FunctionInstance* inst = group->instances[i];
                auto [match, dynSize] = matchFunctionInstance(e->arguments, inst);
                if (!match)
                    continue;
                check(func == nullptr) << "function call is ambiguous";
                func = inst;
                dynamicSize = dynSize;
            }
            if (func == nullptr) {
                auto err = error();
                err << "no matching overload found";
                for (int_t i = 0; i < group->instances.size(); i++) {
                    const FunctionInstance* inst = group->instances[i];
                    matchFunctionInstance(e->arguments, inst, err);
                }
            }
        }
        e->function = func;
        if (func->returnType.isDynamic) {
            if (dynamicSize > 0)
                e->type = func->returnType.u.dynamic.scalarType->vectorType(dynamicSize);
            else
                e->type = &ErrorType;
        } else {
            e->type = func->returnType.u.fixed.type;
        }
    }
    constexpr void transformCompositionExpr(CompositionExpr* e) {
        Base::transformCompositionExpr(e);
        if (e->componentCount <= 0) {
            error() << "invalid component count " << e->componentCount;
            return;
        }
        int_t resultSize = 0;
        const ScalarType* resultType = nullptr;
        for (int_t i = 0; i < e->componentCount; i++) {
            const Expr* comp = e->components[i];
            const auto* type = checkVectorExpr(comp);
            if (!type)
                continue;
            if (resultType == nullptr)
                resultType = type->scalarType;
            else
                checkSameType(type->scalarType, resultType, "all componets must have the same type");
            resultSize += type->vectorSize;
        }
        if (!resultType)
            return;
        check(resultSize <= 4) << "resulting vector of size " << resultSize << " to large";
        resultSize = std::min<int_t>(resultSize, 4);
        e->type = resultType->vectorType(resultSize);
        e->valueKind = ValueKind::RVALUE;
    }
    constexpr void transformUpdateStmt(UpdateStmt* e) {
        Base::transformUpdateStmt(e);
        check(e->left->valueKind == ValueKind::LVALUE) << "left hand side of update must be an lvalue";
        const auto* lType = checkVectorExpr(e->left);
        const auto* rType = checkVectorExpr(e->right);
        if (!lType || !rType)
            return;
        checkSameType(lType->scalarType, rType->scalarType, "operands must have the same base type");
        check(rType->isScalar || rType->vectorSize == lType->vectorSize) << "the right operand must either be a scalar or a vector of same size as the left operand";
    }
    constexpr void checkCompleteBlock(BlockStmt* block) {
        block->declContext.decls.forEachEntry([&](const VarDecl* decl) {
            check(decl->usedMask != 0) << "variable " << decl << " not used";
        });
    }
    constexpr void transformBlockStmt(BlockStmt* block) {
        Base::transformBlockStmt(block);
        checkCompleteBlock(block);
    }
    constexpr void checkBranchCondtion(const Expr* condition) {
        check(condition->type == &BoolType) << "branch condition must be a scalar boolean, got " << condition->type;
        // CheckInitialized check(*this);
        // check.visit(condition);
    }
    constexpr void transformIfStmt(IfStmt* e) {
        Base::transformIfStmt(e);
        checkBranchCondtion(e->condition);
    }
    constexpr void transformParenExpr(ParenExpr* e) {
        Base::transformParenExpr(e);
        e->type = e->subExpr->type;
        e->valueKind = e->subExpr->valueKind;
    }

    constexpr Stmt* createErrorStmt(const ErrorStmt& in) {
        return this->impl()->dispatchTransform(in.subStmt);
    }
    constexpr Stmt* createErrorExpr(const ErrorExpr& in) {
        return this->impl()->dispatchTransform(in.subExpr);
    }
    constexpr Expr* wrapExprWithError(Expr* expr) {
        if (currentError.hasError()) {
            auto* err = this->storage.template allocateStmt<ErrorExpr>({ currentError });
            err->subExpr = expr;
            if (expr->type) {
                err->type = expr->type;
                err->valueKind = expr->valueKind;
            } else {
                err->type = &ErrorType;
                err->valueKind = ValueKind::RVALUE;
            }
            expr = err;
            currentError.clear();
        }
        return expr;
    }
    constexpr Stmt* wrapStmtWithError(Stmt* stmt) {
        if (isExpr(stmt))
            return wrapExprWithError(castStmt<Expr>(stmt));
        if (currentError.hasError()) {
            auto* err = this->storage.template allocateStmt<ErrorStmt>({ currentError });
            err->subStmt = stmt;
            stmt = err;
            currentError.clear();
        }
        return stmt;
    }
    constexpr Stmt* dispatchTransform(const Stmt* in) {
        Stmt* stmt = Base::dispatchTransform(in);
        return wrapStmtWithError(stmt);
    }
};

}
