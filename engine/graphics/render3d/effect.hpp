#pragma once

#include "graphics/shadergen/BuildDriver.hpp"
#include "graphics/shadergen/GLSLGenerator.hpp"

template<typename T>
struct GPtr { };

template<typename T, int_t N>
struct OversizedArray {
    T data[N] = {};
    int_t size = 0;

    constexpr void pushBack(T item) {
        data[size++] = item;
    }
};

namespace shadergen::effect {

template<typename T, typename E>
inline int_t offsetOfMemberPtr(T E::*ptr) {
    const E* e = nullptr;
    const T* t = &(e->*ptr);
    return (const std::byte*)t - (const std::byte*)e;
}

template<typename T>
struct CXXTypeFromMemberPtr;
template<typename T, typename E>
struct CXXTypeFromMemberPtr<GPtr<T> E::*> {
    using type = T;
};

namespace {
    template<auto memberPtr>
    struct StaticOffsetHolder {
        static inline const int_t offset = offsetOfMemberPtr(memberPtr);
    };
};

// Driver
struct StageDriver : BlockDriver {
    struct EffectVariableOffset {
        const VarDecl* decl = nullptr;
        const int_t* offsetPtr = nullptr;
    };

    StageDecl* stageDecl;
    OversizedArray<EffectVariableOffset, 300> offsets;

    constexpr StageDriver(Storage* storage)
        : BlockDriver(storage) {
        stageDecl = createEmptyStageDecl();
        currentBlock = newBlock(&stageDecl->outputs);
        stageDecl->entryPoint = currentBlock;
        currentDeclContext = &currentBlock->declContext;
    }

    constexpr const int_t* findOffset(const VarDecl* decl) const {
        for (int_t i = 0; i < offsets.size; i++) {
            if (offsets.data[i].decl == decl)
                return offsets.data[i].offsetPtr;
        }
        VERIFY_NOT_REACHED(ShaderGen);
        return nullptr;
    }
    template<auto memberPtr>
    constexpr DriverVariable<typename CXXTypeFromMemberPtr<decltype(memberPtr)>::type> declareEffectVariable(std::string_view name, VarDeclContext* declContext) {
        return {
            name,
            [&](const VarDecl* driverDecl) constexpr {
                createVarDecl(driverDecl, declContext);
                auto& off = offsets.data[offsets.size++];
                off.decl = driverDecl;
                off.offsetPtr = &StaticOffsetHolder<memberPtr>::offset;
            }
        };
    }
    template<auto memberPtr>
    constexpr auto uniform(std::string_view name) {
        return declareEffectVariable<memberPtr>(name, &stageDecl->uniforms);
    }
    template<auto memberPtr>
    constexpr auto input(std::string_view name) {
        return declareEffectVariable<memberPtr>(name, &stageDecl->inputs);
    }
    template<auto memberPtr>
    constexpr auto output(std::string_view name) {
        return declareEffectVariable<memberPtr>(name, &stageDecl->outputs);
    }
};
struct EffectDriver {
    using Storage = BlockDriver::Storage;
    Storage storage;
    StageDriver vert { &storage };
    StageDriver frag { &storage };
};

// constexpr descriptor
struct ConstVariableDescriptor {
    const Type* type;
    const int_t* offsetInStruct;
};
struct ConstStageDescriptor {
    OversizedArray<ConstVariableDescriptor, 100> uniforms;
    OversizedArray<char, 10000> source;
};
struct ConstEffectDescriptor {
    OversizedArray<ConstVariableDescriptor, 100> attributes;
    ConstStageDescriptor vert;
    ConstStageDescriptor frag;
};
constexpr int_t fillConstVariableArray(ConstVariableDescriptor* end, const StageDriver& stage, const VarDeclContext& declContext) {
    ConstVariableDescriptor* begin = end;
    declContext.decls.forEachEntry([&](const VarDecl* decl) {
        *(end++) = { decl->type, stage.findOffset(decl->prevDecl) };
    });
    return end - begin;
}
constexpr void fillConstStageDescriptor(ConstStageDescriptor& out, const StageDriver& in) {
    out.uniforms.size = fillConstVariableArray(out.uniforms.data, in, in.stageDecl->uniforms);

    BlockDriver::Storage storage;
    GLSLStmtTransform transform { &storage };
    StageDecl* transformed = transform.createStageDecl(*in.stageDecl);
    GLSLStmtGenerator generator { out.source.data };
    generator.dispatchVisit(transformed->entryPoint);
    out.source.size = generator.end - out.source.data;
}
using DefineFunc_t = void (*)(EffectDriver&);
constexpr ConstEffectDescriptor createConstEffectDescriptor(DefineFunc_t defineFunc) {
    EffectDriver effect;
    defineFunc(effect);
    ConstEffectDescriptor out;
    out.attributes.size = fillConstVariableArray(out.attributes.data, effect.vert, effect.vert.stageDecl->inputs);
    fillConstStageDescriptor(out.vert, effect.vert);
    fillConstStageDescriptor(out.frag, effect.frag);
    return out;
}

// descriptor
struct VariableDescriptor {
    const Type* type;
    int_t offsetInStruct;
};
struct StageDescriptor {
    ArrayView<const VariableDescriptor> uniforms;
    std::string_view source;
};
struct EffectDescriptor {
    ArrayView<const VariableDescriptor> attributes;
    StageDescriptor vert;
    StageDescriptor frag;
};
namespace {
    template<ConstVariableDescriptor... descriptors>
    struct StaticVariableDescriptorArray {
        static inline const VariableDescriptor array[] = { { descriptors.type, *descriptors.offsetInStruct }... };
    };
    template<auto oversized, int_t... I>
    constexpr ArrayView<const VariableDescriptor> createVariableDescriptorArray_helper(stx::int_sequence<I...>) {
        return StaticVariableDescriptorArray<oversized.data[I]...>::array;
    }
    template<auto oversized>
    constexpr ArrayView<const VariableDescriptor> createVariableDescriptorArray() {
        if constexpr (oversized.size == 0) // this is necessary because 0 sized arrays aren't allowed
            return {};
        else
            return createVariableDescriptorArray_helper<oversized>(stx::make_int_sequence<oversized.size>());
    }
    template<auto oversized>
    constexpr auto createSource_helper() {
        std::array<char, oversized.size> array;
        std::copy_n(oversized.data, oversized.size, array.data());
        return array;
    }
    template<auto in>
    constexpr const auto& makeStatic() {
        return in;
    }
    template<auto oversized>
    constexpr std::string_view createSource() {
        const auto& array = makeStatic<createSource_helper<oversized>()>();
        return { array.data(), array.size() };
    }
    template<ConstStageDescriptor stage>
    constexpr StageDescriptor createStageDescriptor() {
        return { createVariableDescriptorArray<stage.uniforms>(), createSource<stage.source>() };
    }
}
template<typename E>
constexpr EffectDescriptor createEffectDescriptor() {
    constexpr ConstEffectDescriptor effect = createConstEffectDescriptor(E::defineEffect);
    return { createVariableDescriptorArray<effect.attributes>(), createStageDescriptor<effect.vert>(), createStageDescriptor<effect.frag>() };
}
template<typename E>
constexpr EffectDescriptor descriptorStore = {};

}

DEFINE_SECTION(const shadergen::effect::EffectDescriptor, effects)
#define DEFINE_EFFECT(effect_type) \
    template<>                     \
    constinit IN_SECTION(effects) const shadergen::effect::EffectDescriptor shadergen::effect::descriptorStore<effect_type> = shadergen::effect::createEffectDescriptor<effect_type>();

// in .hpp

struct SomeEffect {
    GPtr<Vec3[]> attribute;
    struct VertexUniform {
        BEGIN_GEN_STRUCT(VertexUniform)
        Vec3 GEN_FIELD(position);
        END_GEN_STRUCT()
    };
    GPtr<VertexUniform> vertexUniform;

    static constexpr void defineEffect(shadergen::effect::EffectDriver&);
};

// in .cpp

constexpr void SomeEffect::defineEffect(shadergen::effect::EffectDriver& prog) {
    using namespace shadergen::code;
    auto attr = prog.vert.uniform<&SomeEffect::vertexUniform>("a_attr");
    prog.vert, attr.position += constant(1.0f, 1.0f, 1.0f);
}

DEFINE_EFFECT(SomeEffect)
