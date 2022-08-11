#pragma once

#include "NodeSequence.hpp"

namespace render3d {

template<typename S, typename Impl>
struct Visitor {
    IMPORT_NODES(S);
    Impl* impl() { return static_cast<Impl*>(this); }
    void visitSequence(NodeSequence<S> seq) {
        impl()->dispatchVisit(seq);
    }
    void dispatchVisit(NodeSequence<S> seq) {
        switch (seq.frontKind()) {
        case NodeKind::Portal:
            impl()->visitPortalNode(seq.template firstIs<PortalNode>());
            break;
        case NodeKind::Transform:
            impl()->visitTransformNode(seq.template firstIs<TransformNode>());
            break;
        case NodeKind::Bounding:
            impl()->visitBoundingNode(seq.template firstIs<BoundingNode>());
            break;
        case NodeKind::Object:
            impl()->visitObjectNode(seq.template firstIs<ObjectNode>());
            break;
        case NodeKind::Compound:
            impl()->visitCompoundNode(seq.template firstIs<CompoundNode>());
            break;
        case NodeKind::WorldCoordinatesMarker:
            impl()->visitWorldCoordinatesMarkerNode(seq.template firstIs<WorldCoordinatesMarkerNode>());
            break;
        default:
            VERIFY_NOT_REACHED(render3d);
            break;
        }
    }
    void visitPortalNode(typename NodeSequence<S>::template FirstIs<PortalNode> seq) {
        impl()->dispatchVisit(seq.subSequence());
    }
    void visitTransformNode(typename NodeSequence<S>::template FirstIs<TransformNode> seq) {
        impl()->dispatchVisit(seq.subSequence());
    }
    void visitBoundingNode(typename NodeSequence<S>::template FirstIs<BoundingNode> seq) {
        impl()->dispatchVisit(seq.subSequence());
    }
    void visitObjectNode(typename NodeSequence<S>::template FirstIs<ObjectNode>) { }
    void visitCompoundNode(typename NodeSequence<S>::template FirstIs<CompoundNode> seq) {
        for (auto subSeq : seq.first()->subSeqs())
            impl()->visitSequence(subSeq);
    }
    void visitWorldCoordinatesMarkerNode(typename NodeSequence<S>::template FirstIs<WorldCoordinatesMarkerNode> seq) {
        impl()->dispatchVisit(seq.subSequence());
    }
};

template<typename Impl>
struct ParallelVisitor {
    Impl* impl() { return static_cast<Impl*>(this); }
    void visitSequence(NodeSequence<SG> sgSeq, NodeSequence<RT> rtSeq) {
        impl()->dispatchVisit(sgSeq, rtSeq);
    }
    void dispatchVisit(NodeSequence<SG> sgSeq, NodeSequence<RT> rtSeq) {
        switch (sgSeq.frontKind()) {
        case NodeKind::Portal:
            impl()->visitPortalNode(sgSeq.firstIs<SG::PortalNode>(), rtSeq.firstIs<RT::PortalNode>());
            break;
        case NodeKind::Transform:
            impl()->visitTransformNode(sgSeq.firstIs<SG::TransformNode>(), rtSeq.firstIs<RT::TransformNode>());
            break;
        case NodeKind::Bounding:
            impl()->visitBoundingNode(sgSeq.firstIs<SG::BoundingNode>(), rtSeq.firstIs<RT::BoundingNode>());
            break;
        case NodeKind::Object:
            impl()->visitObjectNode(sgSeq.firstIs<SG::ObjectNode>(), rtSeq.firstIs<RT::ObjectNode>());
            break;
        case NodeKind::Compound:
            impl()->visitCompoundNode(sgSeq.firstIs<SG::CompoundNode>(), rtSeq.firstIs<RT::CompoundNode>());
            break;
        case NodeKind::WorldCoordinatesMarker:
            impl()->visitWorldCoordinatesMarkerNode(sgSeq.firstIs<SG::WorldCoordinatesMarkerNode>(), rtSeq.firstIs<RT::WorldCoordinatesMarkerNode>());
            break;
        default:
            VERIFY_NOT_REACHED(render3d);
            break;
        }
    }
    void visitPortalNode(NodeSequence<SG>::FirstIs<SG::PortalNode> sgSeq, NodeSequence<RT>::FirstIs<RT::PortalNode> rtSeq) {
        impl()->dispatchVisit(sgSeq.subSequence(), rtSeq.subSequence());
    }
    void visitTransformNode(NodeSequence<SG>::FirstIs<SG::TransformNode> sgSeq, NodeSequence<RT>::FirstIs<RT::TransformNode> rtSeq) {
        impl()->dispatchVisit(sgSeq.subSequence(), rtSeq.subSequence());
    }
    void visitBoundingNode(NodeSequence<SG>::FirstIs<SG::BoundingNode> sgSeq, NodeSequence<RT>::FirstIs<RT::BoundingNode> rtSeq) {
        impl()->dispatchVisit(sgSeq.subSequence(), rtSeq.subSequence());
    }
    void visitObjectNode(NodeSequence<SG>::FirstIs<SG::ObjectNode>, NodeSequence<RT>::FirstIs<RT::ObjectNode>) { }
    void visitCompoundNode(NodeSequence<SG>::FirstIs<SG::CompoundNode> sgSeq, NodeSequence<RT>::FirstIs<RT::CompoundNode> rtSeq) {
        auto sgSeqs = sgSeq.first()->subSeqs();
        auto rtSeqs = rtSeq.first()->subSeqs();
        VERIFY(render3d, sgSeqs.size() == rtSeqs.size());
        for (int_t i = 0; i < sgSeqs.size(); i++)
            impl()->visitSequence(sgSeqs[i], rtSeqs[i]);
    }
    void visitPortalNode(NodeSequence<SG>::FirstIs<SG::WorldCoordinatesMarkerNode> sgSeq, NodeSequence<RT>::FirstIs<RT::WorldCoordinatesMarkerNode> rtSeq) {
        impl()->dispatchVisit(sgSeq.subSequence(), rtSeq.subSequence());
    }
};

struct RTDumper : Visitor<RT, RTDumper> {
    using Base = Visitor<RT, RTDumper>;
    int_t indent = 0;
    std::string output;
    void info(std::string_view name, bool isLast = false) {
        output += name;
        if (!isLast)
            output += " -> ";
    }
    void visitPortalNode(NodeSequence<RT>::FirstIs<RT::PortalNode> seq) {
        info("Portal");
        Base::visitPortalNode(seq);
    }
    void visitTransformNode(NodeSequence<RT>::FirstIs<RT::TransformNode> seq) {
        info("Transform");
        Base::visitTransformNode(seq);
    }
    void visitBoundingNode(NodeSequence<RT>::FirstIs<RT::BoundingNode> seq) {
        info("Bounding");
        Base::visitBoundingNode(seq);
    }
    void visitObjectNode(NodeSequence<RT>::FirstIs<RT::ObjectNode> seq) {
        info("Object", true);
        Base::visitObjectNode(seq);
    }
    void visitCompoundNode(NodeSequence<RT>::FirstIs<RT::CompoundNode> seq) {
        info("Compound ", true);
        output += std::to_string(seq.first()->subSeqs().size());
        indent += 1;
        Base::visitCompoundNode(seq);
        indent -= 1;
    }
    void visitWorldCoordinatesMarkerNode(NodeSequence<RT>::FirstIs<RT::WorldCoordinatesMarkerNode> seq) {
        info("WorldCoordinatesMarker");
        Base::visitWorldCoordinatesMarkerNode(seq);
    }
    void visitSequence(NodeSequence<RT> seq) {
        log("{}", output);
        output = std::string(indent * 4, ' ');
        Base::visitSequence(seq);
    }
    void dump(NodeSequence<RT> seq) {
        visitSequence(seq);
        log("{}", output);
    }
};

}
