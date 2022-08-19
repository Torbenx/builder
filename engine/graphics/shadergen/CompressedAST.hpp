#pragma once

#include "ASTVisitor.hpp"

namespace shadergen::compressed {

// resolve node types
enum class NodeKind {
    Unknown,
    Stmt,
    ListEntry,
    VarDecl,
    StageDecl,
    ErrorNote,
};
struct NodeType {
    NodeKind nodeKind = NodeKind::Unknown;
    StmtKind stmtKind = StmtKind::Stmt;
    constexpr NodeType() = default;
    constexpr NodeType(NodeKind nodeKind)
        : nodeKind(nodeKind), stmtKind(StmtKind::Stmt) { }
    constexpr NodeType(StmtKind stmtKind)
        : nodeKind(NodeKind::Stmt), stmtKind(stmtKind) { }
};

struct ResolvedNodes {
    NodeType nodes[10000];
    int_t nodeCount = 0;
    int_t totalMessageLength = 0;
};

struct NodeResolver : ASTVisitor<NodeResolver> {
    using Base = ASTVisitor<NodeResolver>;
    ResolvedNodes nodes;
    constexpr void allocate(NodeType type) {
        nodes.nodes[nodes.nodeCount++] = type;
    }
    constexpr void allocateMessage(std::string_view msg) {
        nodes.totalMessageLength += msg.length();
    }
    constexpr void visitStmt(const Stmt* stmt) {
        allocate(stmt->stmtKind);
    }
    constexpr void visitCompoundStmt(const CompoundStmt* stmt) {
        visitStmt(stmt);
        stmt->forEachEntry([&](const Stmt* stmt) {
            dispatchVisit(stmt);
            allocate(NodeKind::ListEntry);
        });
    }
    constexpr void visitDeclContext(const VarDeclContext* declContext) {
        declContext->decls.forEachEntry([&](const VarDecl* decl) {
            allocate(NodeKind::VarDecl);
            for (int_t i = 0; i < decl->modifiers.entryCount; i++)
                allocate(NodeKind::ListEntry);
            allocate(NodeKind::ListEntry);
        });
    }
    constexpr void visitCallExpr(const CallExpr* call) {
        visitExpr(call);
        call->arguments.forEachEntry([&](const Expr* arg) {
            dispatchVisit(arg);
            allocate(NodeKind::ListEntry);
        });
    }
    constexpr void visitStageDecl(const StageDecl* stage) {
        allocate(NodeKind::StageDecl);
        Base::visitStageDecl(stage);
    }
    constexpr void visitErrorNode(const ErrorNode* e) {
        allocateMessage(e->message);
        Base::visitErrorNode(e);
    }
    constexpr void visitNote(const ErrorNote* n) {
        allocate(NodeKind::ErrorNote);
        allocate(NodeKind::ListEntry);
        allocateMessage(n->message);
    }
    static constexpr ResolvedNodes resolve(const StageDecl* stage) {
        NodeResolver resolver;
        resolver.visitStageDecl(stage);
        return resolver.nodes;
    }
};

// compressed storage
struct CompressedASTStorage {
    ASTNode** storage = nullptr;
    constexpr ASTNode* acquireEnd() {
        return *(storage++);
    }
    template<typename T>
    constexpr T* allocateStmt(const T& in) {
        return std::construct_at(static_cast<T*>(acquireEnd()), in);
    }
    constexpr VarDecl* allocateVarDecl(const VarDecl& in) {
        return std::construct_at(static_cast<VarDecl*>(acquireEnd()), in);
    }
    template<typename T>
    constexpr void pushBackOnList(ASTList<T>& list, T* in) {
        auto* entry = std::construct_at(static_cast<ASTListEntry*>(acquireEnd()), ASTListEntry { in });
        list.pushBack(entry);
    }
    constexpr StageDecl* allocateStageDecl(const StageDecl& in) {
        return std::construct_at(static_cast<StageDecl*>(acquireEnd()), in);
    }
    constexpr ErrorNote* allocateNote(const ErrorNote& in) {
        return std::construct_at(static_cast<ErrorNote*>(acquireEnd()), in);
    }
};
struct Compressor : ASTTransform<Compressor, CompressedASTStorage> {
    char* messageBuffer = nullptr;
    constexpr Compressor(CompressedASTStorage alloc, char* messageBuffer)
        : ASTTransform<Compressor, CompressedASTStorage>(alloc), messageBuffer(messageBuffer) { }

    constexpr std::string_view transformMessage(std::string_view in) {
        char* msgBegin = messageBuffer;
        std::copy(in.begin(), in.end(), msgBegin);
        messageBuffer += in.length();
        return { msgBegin, in.length() };
    }
};

template<typename T, int_t i>
struct storage_t {
    union U {
        T node;
        constexpr U() { }
    } u;
    constexpr ASTNode* node() { return &u.node; }
};

template<int_t messageBufferSize, typename... Types>
struct CompressedStage : Types... {
    std::array<char, messageBufferSize> messageBuffer = {};
    StageDecl* stageDecl = nullptr;
    static constexpr int_t nodeCount = sizeof...(Types);
    static constexpr std::array<int_t, sizeof...(Types)> nodeSizes = { sizeof(Types)... };
    constexpr CompressedStage(const StageDecl* in) {
        ASTNode* nodes[] = { Types::node()... };
        CompressedASTStorage alloc { nodes };
        Compressor comp { alloc, messageBuffer.data() };
        stageDecl = comp.createStageDecl(*in);
        PrevDeclStripper stripper1;
        stripper1.visitStageDecl(stageDecl);
        TemporaryModifierStripper stripper2;
        stripper2.visitStageDecl(stageDecl);
    }
};

// node type lookup
template<NodeType type>
struct NodeTypeLookup;
template<>
struct NodeTypeLookup<NodeKind::ListEntry> {
    using type = ASTListEntry;
};
template<>
struct NodeTypeLookup<NodeKind::VarDecl> {
    using type = VarDecl;
};
template<>
struct NodeTypeLookup<NodeKind::StageDecl> {
    using type = StageDecl;
};
template<>
struct NodeTypeLookup<NodeKind::ErrorNote> {
    using type = ErrorNote;
};
#define STMT_KIND(S)                     \
    template<>                           \
    struct NodeTypeLookup<StmtKind::S> { \
        using type = S;                  \
    };
ENUMERATE_STMT_KINDS(STMT_KIND)
#undef STMT_KIND

// compress stage
// clang doesn't support pointers to subojects as direct template parameters???
struct StageDeclPtr {
    const StageDecl* decl = nullptr;
    constexpr StageDeclPtr() = default;
    constexpr StageDeclPtr(const StageDecl* decl)
        : decl(decl) { }
};
template<StageDeclPtr stageDecl>
struct CompressedStageHolder {
    template<ResolvedNodes nodes, int_t... I>
    static constexpr auto generateCompressedStage_helper(stx::int_sequence<I...>) {
        return CompressedStage<nodes.totalMessageLength, storage_t<typename NodeTypeLookup<nodes.nodes[I]>::type, I>...>(stageDecl.decl);
    }
    static constexpr auto generateCompressedStage() {
        constexpr auto nodes = NodeResolver::resolve(stageDecl.decl);
        return generateCompressedStage_helper<nodes>(stx::make_int_sequence<nodes.nodeCount>());
    }
    static constexpr auto compressedStage = generateCompressedStage();
};

}

namespace shadergen {

template<compressed::StageDeclPtr stageDecl>
constexpr const StageDecl* compressStage() {
    return compressed::CompressedStageHolder<stageDecl>::compressedStage.stageDecl;
}

}
