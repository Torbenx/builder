#pragma once

#include "ASTVisitor.hpp"

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

struct MessageBuilder {
    std::string_view* message = nullptr;
    char** storage = nullptr;

    constexpr MessageBuilder() = default;
    constexpr MessageBuilder(std::string_view& message, char*& storage)
        : message(&message), storage(&storage) { }

    constexpr void setEmpty() {
        message = nullptr;
        storage = nullptr;
    }
    constexpr void grow(int_t size) {
        *storage += size;
        *message = { message->data(), message->size() + size };
    }
    constexpr MessageBuilder& operator<<(std::string_view str) {
        if (!message)
            return *this;
        std::copy_n(str.data(), str.size(), *storage);
        grow(str.size());
        return *this;
    }
    constexpr MessageBuilder& operator<<(int_t in) {
        if (!message)
            return *this;
        while (in != 0) {
            int_t mod = in % 10;
            int_t div = in / 10;
            **storage = '0' + mod;
            grow(1);
            in = div;
        }
        return *this;
    }
    constexpr MessageBuilder& operator<<(const Type* type) {
        if (!message)
            return *this;
        formatType(type, *this);
        return *this;
    }
};
struct ErrorBuilder : MessageBuilder {
    ErrorNode* error = nullptr;
    constexpr ErrorBuilder() = default;
    constexpr ErrorBuilder(ErrorNode& error, char*& storage)
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

// Builder
template<typename Storage>
struct ASTBuilder : ASTTransform<ASTBuilder<Storage>, Storage> {
    using Base = ASTTransform<ASTBuilder, Storage>;

    constexpr ASTBuilder() = default;
    constexpr ASTBuilder(Storage* storage)
        : Base(storage) { }

    template<typename Impl>
    struct AccessMaskVisitor : ASTVisitor<Impl, void, FixedBitset> {
        constexpr Impl* impl() { return static_cast<Impl*>(this); }

        constexpr FixedBitset fullMask(const Type* type) {
            if (type->isVector)
                return FixedBitset::bitsBelow(asVectorType(type)->vectorSize);
            else if (type->isUser)
                return FixedBitset::bitsBelow(asUserType(type)->fieldCount);
            else {
                return {};
            }
        }
        constexpr void visitExpr(const Expr*, FixedBitset) { }
        constexpr void visitMemberAccessExpr(const MemberAccessExpr* e, FixedBitset mask) {
            impl()->dispatchVisit(e->base, asUserType(e->base->type)->members[e->memberIndex].fieldMask); // FIXME: this is wrong!
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

    ErrorNode currentError;
    constexpr ErrorBuilder error() {
        if (currentError.hasError())
            return {};
        currentError.message = { this->storage->messageBufferPosition, 0 };
        return { currentError, this->storage->messageBufferPosition };
    }
    constexpr MessageBuilder addNote(ErrorBuilder err) {
        if (!err.error)
            return {};
        auto* note = this->storage->allocateNote({});
        this->storage->pushBackOnList(err.error->notes, note);
        note->message = { this->storage->messageBufferPosition, 0 };
        return { note->message, this->storage->messageBufferPosition };
    }
    constexpr ErrorBuilder check(bool cond) {
        if (!cond)
            return error();
        return {};
    }
    constexpr void checkSameType(const Type* left, const Type* right, std::string_view message) {
        check(left == right) << message << ", got '" << left << "' and '" << right << "'";
    }
    constexpr const VectorType* checkVectorExpr(const Expr* e) {
        if (e->type->isVector)
            return asVectorType(e->type);

        error() << "expected vector type, got '" << e->type << "'";
        return nullptr;
    }
    constexpr const ScalarType* checkScalarExpr(const Expr* e) {
        if (e->type->isScalar)
            return asScalarType(e->type);

        auto err = error();
        err << "expected scalar type, got '" << e->type << "'";
        err.setCausingSubExpr(e);
        return nullptr;
    }

    constexpr void transformVarRefExpr(VarRefExpr* e) {
        BlockStmt* block = this->currentBlock;
        while (block && (&block->declContext != e->decl->declContext))
            block = block->parent;
        check(block) << "variable \"" << e->decl->name << "\" not found";

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
            check(type->isLogical) << "logical operation requires boolean, got '" << type << "'";
        else if (isArithmeticOp(e->op))
            check(type->isArithmetic) << "arithmetic operation requires arithmetic type, got '" << type << "'";
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
                check(lType->isLogical) << "logical operation requires boolean, got '" << lType << "'";
            else if (isArithmeticOp(e->op))
                check(lType->isArithmetic) << "arithmetic operation requires arithmetic type, got '" << lType << "'";
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
        check(vType->isArithmetic) << "only arithmetic types allowed in vector by scalar operations, got '" << vType << "'";
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
        CheckInitialized check;
        check.visit(e->right);
        MarkInitialized mark;
        mark.visit(e->left);
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
                if (arg->type != parm.fixed.type) {
                    note() << "parameter " << i << " expects '" << parm.fixed.type << "' but got '" << arg->type << "'";
                    isMatch = false;
                }
                return;
            }
            if (!arg->type->isVector) {
                note() << "parameter " << i << " expects vector type but got '" << arg->type << "'";
                isMatch = false;
                return;
            }
            const auto* vType = asVectorType(arg->type);
            if (vType->scalarType != parm.dynamic.scalarType) {
                note() << "parameter " << i << " expects vector of base type '" << parm.dynamic.scalarType << "' but got base type '" << vType->scalarType << "'";
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
                e->type = func->returnType.dynamic.scalarType->vectorType(dynamicSize);
            else
                e->type = &ErrorType;
        } else {
            e->type = func->returnType.fixed.type;
        }
    }

    constexpr Stmt* dispatchTransform(const Stmt* in) {
        Stmt* stmt = Base::dispatchTransform(in);
        if (currentError.hasError()) {
            if (isExpr(stmt)) {
                auto* expr = castStmt<Expr>(stmt);
                auto* err = this->storage->template allocateStmt<ErrorExpr>({ currentError });
                err->subExpr = expr;
                if (expr->type) {
                    err->type = expr->type;
                    err->valueKind = expr->valueKind;
                } else {
                    err->type = &ErrorType;
                    err->valueKind = ValueKind::RVALUE;
                }
                stmt = err;
            } else {
                auto* err = this->storage->template allocateStmt<ErrorStmt>({ currentError });
                err->subStmt = stmt;
                stmt = err;
            }
            currentError.clear();
        }
        return stmt;
    }
};

// Driver
struct BuildDriver {
    using Storage = BasicASTStorage<100, 1000>;
    ASTBuilder<Storage> builder { &storage };
    Storage storage;

    constexpr void emitStmt(Stmt* stmt) {
        storage.pushBackOnList(*castStmt<CompoundStmt>(builder.currentBlock->body), stmt);
    }

    constexpr VarRefExpr declareVariable(std::string_view name, const Type* type) {
        auto* decl = storage.allocateVarDecl({ name, type, &builder.currentBlock->declContext });
        storage.pushBackOnList(builder.currentBlock->declContext.decls, decl);
        auto* declStmt = storage.allocateStmt<VarDeclStmt>({ decl });
        emitStmt(declStmt);
        return { decl };
    }
    constexpr wrapped<VarRefExpr, void> declareScalar(std::string_view name, const Type* type) {
        return { declareVariable(name, type) };
    }
    constexpr wrapped<VarRefExpr, vec_t> declareVector(std::string_view name, const VectorType* type) {
        return { declareVariable(name, type) };
    }

    constexpr BlockStmt* newBlock() {
        auto* body = storage.allocateStmt<CompoundStmt>({});
        auto* block = storage.allocateStmt<BlockStmt>({ body });
        return block;
    }
    constexpr void beginBlock() {
        auto* newBlock = this->newBlock();
        newBlock->parent = builder.currentBlock;
        emitStmt(newBlock);
        builder.currentBlock = newBlock;
    }
    constexpr void endBlock() {
        builder.currentBlock->declContext.decls.forEachEntry([](const VarDecl* decl) {
            VERIFY(ShaderGen, decl->usedMask != 0);
        });
        builder.currentBlock = builder.currentBlock->parent;
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
