#pragma once

#include "ASTVisitor.hpp"
#include "containers/BitSet.hpp"

namespace shadergen {

class ASTDumper : private ASTVisitor<ASTDumper> {
private:
    using Base = ASTVisitor<ASTDumper>;
    friend class ASTVisitor<ASTDumper>;
    int_t nextIndent = 0;
    struct Line {
        int_t indent;
        BitSet indentMarks;
        std::string nodeType;
        std::vector<std::string> infos;
        bool isLastOfParent = false;
    };
    Line currentLine;
    std::vector<Line> lines;

    template<typename... Args>
    void info(std::string type, Args&&... args) {
        if (currentLine.nodeType.empty())
            currentLine.nodeType = type;
        (currentLine.infos.emplace_back(std::forward<Args>(args)), ...);
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
    void visitMatrixMultiplicationExpr(const MatrixMultiplicationExpr* e) {
        info("MatrixMultiplicationExpr");
        Base::visitMatrixMultiplicationExpr(e);
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
        } else if (vType->scalarType == &UIntType) {
            for (int_t i = 0; i < vType->vectorSize; i++)
                literal << fmt::format("{:,^{}} {}", "", (int)(i > 0), e->literal.uints[i]);
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
        info("BlockStmt");
        Base::visitBlockStmt(e);
    }
    void visitDeclContext(const VarDeclContext* c) {
        beginNode();
        info("VarDeclContext", fmt::format("({} decls)", c->decls.entryCount));
        c->decls.forEachEntry([&](const VarDecl* decl) {
            beginNode();
            info("VarDecl");
            decl->modifiers.forEachEntry([&](const VarDeclModifier* mod) {
                info("VarDeclModifier", fmt::format("'{}'", mod->name));
            });
            info("VarDecl", fmt::format("\"{}\"", decl->name), fmt::format("[{}]", *decl->type));
            endNode();
        });
        endNode();
    }
    void visitMemberAccessExpr(const MemberAccessExpr* e) {
        info("MemberAccessExpr", fmt::format("'.{}'", e->member().name));
        Base::visitMemberAccessExpr(e);
    }
    void visitArraySubscriptExpr(const ArraySubscriptExpr* e) {
        info("ArraySubscriptExpr");
        Base::visitArraySubscriptExpr(e);
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
    void visitRangeStmt(const RangeStmt* e) {
        info("RangeStmt");
        Base::visitRangeStmt(e);
    }
    void visitUpdateStmt(const UpdateStmt* e) {
        info("UpdateStmt", fmt::format("'{}='", toString(e->op)));
        Base::visitUpdateStmt(e);
    }
    void visitCastExpr(const CastExpr* e) {
        info("CastExpr", fmt::format("'{}'", *e->toType));
        Base::visitCastExpr(e);
    }
    void visitParenExpr(const ParenExpr* e) {
        info("ParenExpr");
        visitExpr(e);
        dispatchVisit(e->subExpr);
    }
    void visitErrorNode(const ErrorNode* e) {
        info("ErrorNode", fmt::format("!!! {} !!!", e->message));
    }
    void visitNotes(const ASTList<ErrorNote>& notes) {
        notes.forEachEntry([&](const ErrorNote* note) {
            beginNode();
            info("Note:", note->message);
            endNode();
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
    void visitStageDecl(const StageDecl* e) {
        beginNode();
        info("StageDecl", fmt::format("'{}'", toString(e->kind)));
        Base::visitStageDecl(e);
        endNode();
    }
    void visitShaderDecl(const ShaderDecl* e) {
        beginNode();
        info("ShaderDecl");
        Base::visitShaderDecl(e);
        endNode();
    }

    void nextLine() {
        if (currentLine.nodeType.empty())
            return;
        const auto& lastLine = lines.emplace_back(std::move(currentLine));
        currentLine.indentMarks = lastLine.indentMarks;
    }
    void print() {
        for (const auto& line : lines) {
            std::string out;
            for (int_t i = 0; i < line.indent - 1; i++)
                out += line.indentMarks.get(i) ? "  " : "| ";
            if (line.indent > 0)
                out += line.isLastOfParent ? "'-" : "|-";
            out += line.nodeType;
            for (auto info : line.infos) {
                out += ' ';
                out += info;
            }
            log("{}", out);
        }
        lines.clear();
    }
    void beginNode() {
        nextLine();
        currentLine.indent = nextIndent;
        nextIndent += 1;
    }
    void endNode() {
        nextIndent -= 1;
        nextLine();
        for (auto it = lines.rbegin(); it != lines.rend(); it++) {
            if (it->indent == nextIndent + 1) {
                it->isLastOfParent = true;
                break;
            }
            it->indentMarks.setBit(nextIndent);
        }
    }
    void dispatchVisit(const Stmt* e) {
        beginNode();
        Base::dispatchVisit(e);
        endNode();
    }

public:
    void dump(const Stmt* e) {
        dispatchVisit(e);
        print();
    }
    void dump(const StageDecl* stage) {
        visitStageDecl(stage);
        print();
    }
    void dump(const ShaderDecl* shader) {
        visitShaderDecl(shader);
        print();
    }
};

}
