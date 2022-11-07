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
    ShaderDecl,
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
    constexpr void visitShaderDecl(const ShaderDecl* shader) {
        allocate(NodeKind::ShaderDecl);
        visitDeclContext(&shader->uniforms);
        shader->stages.forEachEntry([&](const StageDecl* stage) {
            visitStageDecl(stage);
            allocate(NodeKind::ListEntry);
        });
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
    template<typename Node>
    static constexpr ResolvedNodes resolve(const Node* node) {
        NodeResolver resolver;
        resolver.dispatchVisit(node);
        return resolver.nodes;
    }
};

// compressed storage
struct CompressedASTStorage {
    ASTNode*** storage = nullptr;
    constexpr ASTNode* acquireEnd() {
        return *((*storage)++);
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
    constexpr ShaderDecl* allocateShaderDecl(const ShaderDecl& in) {
        return std::construct_at(static_cast<ShaderDecl*>(acquireEnd()), in);
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

template<typename Node, int_t messageBufferSize, typename... Types>
struct CompressedAST : Types... {
    stx::Array<char, messageBufferSize> messageBuffer = {};
    Node* output = nullptr;
    static constexpr int_t nodeCount = sizeof...(Types);
    static constexpr stx::Array<int_t, sizeof...(Types)> nodeSizes = { sizeof(Types)... };
    constexpr CompressedAST(const Node* input) {
        ASTNode* nodes[] = { Types::node()... };
        ASTNode** nodesPos = nodes;
        CompressedASTStorage alloc { &nodesPos };
        Compressor comp { alloc, messageBuffer.data() };
        output = comp.dispatchTransform(input);
        PrevDeclStripper stripper1;
        stripper1.dispatchVisit(output);
        TemporaryModifierStripper stripper2;
        stripper2.dispatchVisit(output);
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
struct NodeTypeLookup<NodeKind::ShaderDecl> {
    using type = ShaderDecl;
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

// compress AST
// clang doesn't support pointers to subojects as direct template parameters???
template<typename T>
struct NodePtr {
    using Node = T;
    const Node* node = nullptr;
    constexpr NodePtr() = default;
    constexpr NodePtr(const Node* node)
        : node(node) { }
};
template<typename Node, NodePtr<Node> node>
struct CompressedASTHolder {
    template<ResolvedNodes nodes, int_t... I>
    static constexpr auto generateCompressedAST_helper2(stx::int_sequence<I...>) {
        return CompressedAST<Node, nodes.totalMessageLength, storage_t<typename NodeTypeLookup<nodes.nodes[I]>::type, I>...>(node.node);
    }
    template<ResolvedNodes nodes>
    static constexpr auto generateCompressedAST_helper1() {
        return generateCompressedAST_helper2<nodes>(stx::make_int_sequence<nodes.nodeCount>());
    }
    static constexpr auto generateCompressedAST() {
        return generateCompressedAST_helper1<NodeResolver::resolve(node.node)>();
    }
    static constexpr auto compressedAST = generateCompressedAST();
};

}

namespace shadergen {

template<compressed::NodePtr node>
constexpr const typename decltype(node)::Node* compress() {
    return compressed::CompressedASTHolder<typename decltype(node)::Node, node>::compressedAST.output;
}

}
