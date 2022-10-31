#pragma once

#include "containers/DirectedAcyclicGraph.hpp"
#include "graphics/shadergen/compute.hpp"
#include "renderer.hpp"
#include <unordered_map>

template<typename T>
struct GraphicsPtr { };

namespace shadergen::effect {

using namespace render3d;

struct EffectDescriptor;
template<typename E>
extern EffectDescriptor descriptorStore;

struct EffectHandle {
    const EffectDescriptor* descriptor = nullptr;
    const void* effect = nullptr;
    constexpr EffectHandle() { }
    template<typename E>
    constexpr EffectHandle(const E* effect)
        : descriptor(&descriptorStore<E>), effect(effect) { }
    constexpr EffectHandle(const EffectDescriptor* desc, const void* effect)
        : descriptor(desc), effect(effect) { }
};

template<typename T, typename E>
inline int_t offsetOfMemberPtr(T E::*ptr) {
    const E* e = nullptr;
    const T* t = &(e->*ptr);
    return (const std::byte*)t - (const std::byte*)e;
}
namespace {
    template<auto memberPtr>
    struct StaticOffsetHolder {
        static inline const int_t offset = offsetOfMemberPtr(memberPtr);
    };
};

template<typename T>
struct CXXTypeFromGraphicsPtr;
template<typename T, typename E>
struct CXXTypeFromGraphicsPtr<GraphicsPtr<T> E::*> {
    using type = std::remove_extent_t<T>;
};

// Driver
struct EffectStageDriver : GraphicsStageDriver {
    struct EffectVariableOffset {
        const VarDecl* decl = nullptr;
        const int_t* offsetPtr = nullptr;
    };
    OversizedArray<EffectVariableOffset, 300> offsets;

    int_t subpassCounter = 0;
    struct SubpassIndexModifier : VarDeclModifier {
        int_t subpassIndex = -1;
        constexpr SubpassIndexModifier() = default;
        constexpr SubpassIndexModifier(int_t subpassIndex)
            : VarDeclModifier("subpass_indexed", true), subpassIndex(subpassIndex) { }
    };
    OversizedArray<SubpassIndexModifier, 100> subpassIndexMods;
    static constexpr VarDeclModifier subpassIndicator { "subpass" };

    constexpr EffectStageDriver(ASTStorage storage)
        : GraphicsStageDriver(storage) { }

    constexpr const int_t* findOffsetFromVarDecl(const VarDecl* decl) const {
        for (auto offsetEntry : offsets) {
            if (offsetEntry.decl == decl)
                return offsetEntry.offsetPtr;
        }
        // VERIFY_NOT_REACHED(ShaderGen);
        return nullptr;
    }
    template<auto memberPtr, typename CXXType, int_t N>
    constexpr auto declareEffectVariable(std::string_view name, stx::Array<const VarDeclModifier*, N> mods) {
        return makeDriverVariable<CXXType, N>(
            name, mods,
            [&](const VarDecl* driverDecl) constexpr {
                createVarDecl(driverDecl, &stageDecl()->globals);
                offsets.pushBack({ driverDecl, &StaticOffsetHolder<memberPtr>::offset });
            });
    }
    template<auto memberPtr>
    constexpr auto declareGraphicsPtrEffectVariable(std::string_view name, const VarDeclModifier* modifier) {
        return declareEffectVariable<memberPtr, typename CXXTypeFromGraphicsPtr<decltype(memberPtr)>::type, 1>(name, { modifier });
    }
    template<typename T>
    constexpr auto uniform(std::string_view name) {
        return declareGlobal<T, 1>(name, { &uniformIndicator });
    }
    template<typename T>
    constexpr auto input(std::string_view name) {
        return declareGlobal<T, 1>(name, { &inputIndicator });
    }
    template<typename T>
    constexpr auto output(std::string_view name) {
        return declareGlobal<T, 1>(name, { &outputIndicator });
    }
    template<auto memberPtr>
    constexpr auto uniform(std::string_view name) {
        return declareGraphicsPtrEffectVariable<memberPtr>(name, &uniformIndicator);
    }
    template<auto memberPtr>
    constexpr auto input(std::string_view name) {
        return declareGraphicsPtrEffectVariable<memberPtr>(name, &inputIndicator);
    }
    template<auto memberPtr>
    constexpr auto output(std::string_view name) {
        return declareGraphicsPtrEffectVariable<memberPtr>(name, &outputIndicator);
    }
    template<auto memberPtr>
    constexpr auto subpassInput(std::string_view name) {
        const SubpassIndexModifier& indexMod = subpassIndexMods.pushBack({ subpassCounter++ });
        return declareEffectVariable<memberPtr, Vec4, 2>(name, { &subpassIndicator, &indexMod });
    }
};

struct EffectDriver {
    EffectStageDriver vert;
    EffectStageDriver frag;
    constexpr EffectDriver(ASTStorage storage)
        : vert(storage), frag(storage) { }

    template<typename T>
    constexpr VaryingReturnType<T> varying(std::string_view name) {
        return { name, vert, frag };
    }
};

// cross effect variables
enum class CumulativeUpdateOp {
    NONE,
    ADD,
    MULTIPLY,
    AND,
    OR,
    MIN,
    MAX,
};
static constexpr VarDeclModifier crossEffectIndicator { "cross_effect" };
template<typename T>
struct CrossEffectVariable : DriverVariable<get_wrapper<T>, 1> {
    using Base = DriverVariable<get_wrapper<T>, 1>;
    CumulativeUpdateOp cumOp;
    constexpr CrossEffectVariable(std::string_view name, CumulativeUpdateOp cumOp = CumulativeUpdateOp::NONE)
        : Base(name, typeFromCXX<T>, { &crossEffectIndicator }, [](const VarDecl*) constexpr {}, &globalPseudoVarDeclContext), cumOp(cumOp) { }
    constexpr void update(const get_wrapper<T>& value) {
        using namespace shadergen::code;
        switch (cumOp) {
        case CumulativeUpdateOp::ADD:
            return *this = *this + value;
        case CumulativeUpdateOp::MULTIPLY:
            return *this = *this * value;
        case CumulativeUpdateOp::AND:
            return *this = *this && value;
        case CumulativeUpdateOp::OR:
            return *this = *this || value;
        case CumulativeUpdateOp::MIN:
            return *this = min(*this, value);
        case CumulativeUpdateOp::MAX:
            return *this = max(*this, value);
        default:
            VERIFY_NOT_REACHED(ShaderGen);
            return *this = value;
        }
    }
    using Base::operator=;
    constexpr AssignmentStmt operator=(const CrossEffectVariable<T>& right) const {
        return { this, right.wrappedExpr };
    }
};

}
