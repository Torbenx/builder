#pragma once

#include "ASTVisitor.hpp"

namespace shadergen {

struct ASTDumper : ASTVisitor<ASTDumper> {
    using Base = ASTVisitor<ASTDumper>;
    int_t nextIndent = 0;
    int_t indent = 0;
    std::string nodeType;
    std::vector<std::string> infos;

    template<typename... Args>
    void info(std::string type, Args&&... args) {
        if (nodeType.empty())
            nodeType = type;
        (infos.emplace_back(std::forward<Args>(args)), ...);
    }

    void print() {
        std::string out(indent * 2, ' ');
        out += nodeType;
        for (auto info : infos) {
            out += ' ';
            out += info;
        }
        log("{}", out);
        nodeType = "";
        infos.clear();
    }

    void visitStmt(const Stmt* e) {
        info("Stmt");
        Base::visitStmt(e);
    }
    void visitExpr(const Expr* e) {
        info("Expr", fmt::format("{}[{}]", toShortSting(e->valueKind), *e->type));
        Base::visitExpr(e);
    }
    void visitVarRefExpr(const VarRefExpr* e) {
        info("VarRefExpr", fmt::format("\"{}\"", e->decl->name));
        Base::visitVarRefExpr(e);
    }
    void visitElementwiseOperatorExpr(const ElementwiseOperatorExpr* e) {
        info("ElementwiseOperatorExpr", fmt::format("'{}'", toString(e->op)));
        Base::visitElementwiseOperatorExpr(e);
    }
    void visitVectorByScalarOperatorExpr(const VectorByScalarOperatorExpr* e) {
        info("VectorByScalarOperatorExpr", fmt::format("'{}'", toString(e->op)));
        Base::visitVectorByScalarOperatorExpr(e);
    }
    void visitUnaryOperatorExpr(const UnaryOperatorExpr* e) {
        info("UnaryOperatorExpr", fmt::format("'{}'", toString(e->op)));
        Base::visitUnaryOperatorExpr(e);
    }
    void visitAssignmentStmt(const AssignmentStmt* e) {
        info("AssignmentStmt");
        Base::visitAssignmentStmt(e);
    }
    void visitLiteralExpr(const LiteralExpr* e) {
        std::stringstream literal;
        literal << "{";
        const auto* vType = asVectorType(e->type);
        if (vType->scalarType == &BoolType) {
            for (int_t i = 0; i < vType->vectorSize; i++)
                literal << fmt::format("{:,^{}} {}", "", (int)(i > 0), e->literal.bools[i]);
        } else if (vType->scalarType == &IntType) {
            for (int_t i = 0; i < vType->vectorSize; i++)
                literal << fmt::format("{:,^{}} {}", "", (int)(i > 0), e->literal.ints[i]);
        } else if (vType->scalarType == &FloatType) {
            for (int_t i = 0; i < vType->vectorSize; i++)
                literal << fmt::format("{:,^{}} {:.2f}", "", (int)(i > 0), e->literal.floats[i]);
        }
        literal << " }";
        info("LiteralExpr", literal.str());
        Base::visitLiteralExpr(e);
    }
    void visitCompoundStmt(const CompoundStmt* e) {
        info("CompoundStmt", fmt::format("({} children)", e->entryCount));
        Base::visitCompoundStmt(e);
    }
    void visitBlockStmt(const BlockStmt* e) {
        info("BlockStmt", fmt::format("({} decls)", e->declContext.decls.entryCount));
        Base::visitBlockStmt(e);
    }
    void visitMemberAccessExpr(const MemberAccessExpr* e) {
        info("MemberAccessExpr", fmt::format("'.{}'", e->member().name));
        Base::visitMemberAccessExpr(e);
    }
    void visitVectorComponentExpr(const VectorComponentExpr* e) {
        if (e->componentCount == 0) {
            info("VectorComponentExpr", "'?'");
        } else {
            static constexpr char characters[4] = { 'x', 'y', 'z', 'w' };
            auto getChar = [](int8_t comp) { return comp < 4 ? characters[comp] : '?'; };
            std::stringstream ss;
            ss << "'.";
            for (int_t i = 0; i < e->componentCount; i++)
                ss << getChar(e->components[i]);
            ss << '\'';
            info("VectorComponentExpr", ss.str());
        }
        Base::visitVectorComponentExpr(e);
    }
    void visitCallExpr(const CallExpr* e) {
        info("CallExpr", fmt::format("\"{}\"", e->function->name), fmt::format("({} arguments)", e->arguments.entryCount));
        Base::visitCallExpr(e);
    }
    void visitCompositionExpr(const CompositionExpr* e) {
        info("CompositionExpr");
        Base::visitCompositionExpr(e);
    }
    void visitIfStmt(const IfStmt* e) {
        info("IfStmt");
        Base::visitIfStmt(e);
    }
    void visitUpdateStmt(const UpdateStmt* e) {
        info("UpdateStmt", fmt::format("'{}='", toString(e->op)));
        Base::visitUpdateStmt(e);
    }

    void visitErrorNode(const ErrorNode* e) {
        info("ErrorNode", fmt::format("!!! {} !!!", e->message));
    }
    void visitNotes(const ASTList<ErrorNote>& notes) {
        notes.forEachEntry([&](const ErrorNote* note) {
            beginChild();
            info("Note:", note->message);
            endChild();
        });
    }
    void visitErrorStmt(const ErrorStmt* e) {
        info("ErrorStmt");
        visitErrorNode(e);
        visitStmt(e);
        visitNotes(e->notes);
        dispatchVisit(e->subStmt);
    }
    void visitErrorExpr(const ErrorExpr* e) {
        info("ErrorExpr");
        visitErrorNode(e);
        visitExpr(e);
        visitNotes(e->notes);
        dispatchVisit(e->subExpr);
    }

    void beginChild() {
        if (!nodeType.empty()) {
            print();
        }
        indent = nextIndent;
        nextIndent += 1;
    }
    void endChild() {
        nextIndent -= 1;
    }
    void dispatchVisit(const Stmt* e) {
        beginChild();
        Base::dispatchVisit(e);
        endChild();
    }

    void dump(const Stmt* e) {
        this->dispatchVisit(e);
        print();
    }
};

}
