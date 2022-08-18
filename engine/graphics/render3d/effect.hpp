#pragma once

#include "containers/DirectedAcyclicGraph.hpp"
#include "graphics/shadergen/BuildDriver.hpp"
#include "graphics/shadergen/GLSLGenerator.hpp"
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

template<typename T, int_t N>
struct OversizedArray {
    using value_type = T;
    T data[N] = {};
    int_t m_size = 0;

    constexpr T* begin() { return data; }
    constexpr T* end() { return begin() + size(); }
    constexpr const T* begin() const { return data; }
    constexpr const T* end() const { return begin() + size(); }
    constexpr int_t size() const { return m_size; }
    constexpr int_t capacity() const { return N; }
    constexpr T* acquireEnd(int_t size = 1) {
        T* end = this->end();
        m_size += size;
        return end;
    }
    constexpr T& pushBack(T item) {
        return *acquireEnd() = item;
    }
    constexpr operator PseudoVector<T>() { return { data, &m_size }; }
    constexpr operator ArrayView<T>() { return { data, size() }; }
    constexpr operator ArrayView<const T>() const { return { data, size() }; }
};
template<auto oversized>
constexpr auto shrinkToFit() {
    using T = typename decltype(oversized)::value_type;
    OversizedArray<T, oversized.size()> shrunk;
    std::copy(oversized.begin(), oversized.end(), shrunk.acquireEnd(oversized.size()));
    return shrunk;
}
template<auto oversized>
constexpr ArrayView<const typename decltype(oversized)::value_type> staticView() {
    if constexpr (oversized.size() == oversized.capacity())
        return oversized;
    return staticView<shrinkToFit<oversized>()>();
}

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
struct EffectStageDriver : StageDriver {
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
            : VarDeclModifier("subpass_indexed"), subpassIndex(subpassIndex) { }
    };
    OversizedArray<SubpassIndexModifier, 100> subpassIndexMods;
    static constexpr VarDeclModifier subpassIndicator { "subpass" };

    constexpr EffectStageDriver(ASTStorage storage)
        : StageDriver(storage) { }

    constexpr const int_t* findOffsetFromVarDecl(const VarDecl* decl) const {
        for (auto offsetEntry : offsets) {
            if (offsetEntry.decl == decl)
                return offsetEntry.offsetPtr;
        }
        // VERIFY_NOT_REACHED(ShaderGen);
        return nullptr;
    }
    template<auto memberPtr, typename CXXType, int_t N>
    constexpr auto declareEffectVariable(FixedStringView name, std::array<const VarDeclModifier*, N> mods) {
        return DriverVariable<CXXType, N>(
            name, mods,
            [&](const VarDecl* driverDecl) constexpr {
                createVarDecl(driverDecl, &stageDecl()->globals);
                offsets.pushBack({ driverDecl, &StaticOffsetHolder<memberPtr>::offset });
            });
    }
    template<auto memberPtr>
    constexpr auto declareGraphicsPtrEffectVariable(FixedStringView name, const VarDeclModifier* modifier) {
        return declareEffectVariable<memberPtr, typename CXXTypeFromGraphicsPtr<decltype(memberPtr)>::type, 1>(name, { modifier });
    }
    template<typename T>
    constexpr auto uniform(FixedStringView name) {
        return declareGlobal<T, 1>(name, { &uniformIndicator });
    }
    template<typename T>
    constexpr auto input(FixedStringView name) {
        return declareGlobal<T, 1>(name, { &inputIndicator });
    }
    template<typename T>
    constexpr auto output(FixedStringView name) {
        return declareGlobal<T, 1>(name, { &outputIndicator });
    }
    template<auto memberPtr>
    constexpr auto uniform(FixedStringView name) {
        return declareGraphicsPtrEffectVariable<memberPtr>(name, &uniformIndicator);
    }
    template<auto memberPtr>
    constexpr auto input(FixedStringView name) {
        return declareGraphicsPtrEffectVariable<memberPtr>(name, &inputIndicator);
    }
    template<auto memberPtr>
    constexpr auto output(FixedStringView name) {
        return declareGraphicsPtrEffectVariable<memberPtr>(name, &outputIndicator);
    }
    template<auto memberPtr>
    constexpr auto subpassInput(FixedStringView name) {
        const SubpassIndexModifier& indexMod = subpassIndexMods.pushBack({ subpassCounter++ });
        return declareEffectVariable<memberPtr, Vec4, 2>(name, { &subpassIndicator, &indexMod });
    }
};

struct EffectDriver {
    using Storage = GraphicsDriver::Storage;
    Storage storage;
    EffectStageDriver vert { storage };
    EffectStageDriver frag { storage };

    template<typename T>
    constexpr VaryingReturnType<T> varying(FixedStringView name) {
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
struct CrossEffectVariable : DriverVariable<T, 1> {
    using Base = DriverVariable<T, 1>;
    CumulativeUpdateOp cumOp;
    constexpr CrossEffectVariable(std::string_view name, CumulativeUpdateOp cumOp = CumulativeUpdateOp::NONE)
        : DriverVariable<T, 1>(name, { &crossEffectIndicator }, [](const VarDecl*) constexpr {}, &globalPseudoVarDeclContext), cumOp(cumOp) { }
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
