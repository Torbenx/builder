#pragma once

#include "BuildDriver.hpp"
#include "CompressedAST.hpp"
#include "GLSLGenerator.hpp"
#include "Shader.hpp"
#include "graphics/Buffer.hpp"

namespace shadergen {

struct Offset {
    template<typename T, typename B, T B::*member>
    static inline const int_t actualOffset = []() {
        B* base = reinterpret_cast<B*>((void*)nullptr);
        T* obj = &((*base).*member);
        return (std::byte*)obj - (std::byte*)base;
    }();
    const int_t* ptr = nullptr;
    constexpr Offset() = default;
    template<typename T, typename B, T B::*member>
    constexpr Offset(std::integral_constant<T B::*, member>)
        : ptr(&actualOffset<T, B, member>) { }
    operator int_t() const { return *ptr; }
};
struct CBinding : BindingBase {
    Offset offset;
    operator Binding() const { return { (const BindingBase&)*this, (int_t)offset }; }
};

namespace code {
    enum MemQual : uint32_t {
        CanAlias = 1,
        NoRead = 2,
        NoWrite = 4,
    };
}
inline constexpr VarDeclModifier memoryCanAlias { "can-alias" };
inline constexpr VarDeclModifier memoryNoRead { "no-read" };
inline constexpr VarDeclModifier memoryNoWrite { "no-write" };

inline constexpr VarDeclModifier builtinIndicator { "builtin" };
inline constexpr VarDeclModifier inputIndicator { "input" };
inline constexpr VarDeclModifier outputIndicator { "output" };

inline constexpr VarDeclModifier uniformBufferIndicator { "uniform-buffer" };
inline constexpr VarDeclModifier storageBufferIndicator { "storage-buffer" };
inline constexpr VarDeclModifier sampledImageIndicator { "sampled-image" };
inline constexpr VarDeclModifier storageImageIndicator { "storage-image" };

struct BindingModifier : VarDeclModifier {
    int_t index = -1;
    constexpr BindingModifier() = default;
    constexpr BindingModifier(int_t index)
        : VarDeclModifier("binding", true), index(index) { }
};

template<typename>
struct CXXTypeFromMember;
template<typename T, typename Base>
struct CXXTypeFromMember<gfx::BufferView<T> Base::*> {
    using type = T;
};
struct ShaderDriver : ShaderDeclDriver {
    int bindingIndices[4] = {};
    PseudoVector<CBinding> bindings;
    PseudoVector<BindingModifier> bindingModifiers;

    constexpr ShaderDriver(ASTStorage storage, PseudoVector<CBinding> bindings, PseudoVector<BindingModifier> bindingModifiers)
        : ShaderDeclDriver(storage), bindings(bindings), bindingModifiers(bindingModifiers) { }

    template<auto member>
    constexpr const BindingModifier* makeBinding(Binding::Type type, const char* name) {
        int index = bindingIndices[stx::to_underlying(type)]++;
        bindings.pushBack({ { name, index, type }, stx::constant<member>() });
        return &bindingModifiers.pushBack({ index });
    }

    template<auto member>
    constexpr auto uniformBuffer(const char* name) {
        using CXXType = typename CXXTypeFromMember<decltype(member)>::type;
        return declareUniform<CXXType, 2>(name, { &uniformBufferIndicator, makeBinding<member>(Binding::Type::UniformBuffer, name) });
    }
    template<auto member>
    constexpr auto storageBuffer(const char* name, code::MemQual qual) {
        using CXXType = typename CXXTypeFromMember<decltype(member)>::type;
        const VarDeclModifier* mods[5] = { &shadergen::storageBufferIndicator, makeBinding<member>(Binding::Type::StorageBuffer, name) };
        int_t offset = 2;
        if (qual & code::MemQual::CanAlias)
            mods[offset++] = &memoryCanAlias;
        if (qual & code::MemQual::NoRead)
            mods[offset++] = &memoryNoRead;
        if (qual & code::MemQual::NoWrite)
            mods[offset++] = &memoryNoWrite;
        return declareUniform<CXXType[], 5>(name, { mods, offset });
    }
    /*template<typename T>
    constexpr auto sampledImage(const char* name) {
        return declareUniform<T, 2>(name, { &shadergen::sampledImageIndicator, makeBinding(Binding::Type::SampledImage, name) });
    }
    template<typename T>
    constexpr auto storageImage(const char* name, code::MemQual qual) {
        const VarDeclModifier* mods[5] = { &shadergen::storageImageIndicator, makeBinding(Binding::Type::StorageImage, name) };
        int_t offset = 2;
        if (qual & code::MemQual::CanAlias)
            mods[offset++] = &memoryCanAlias;
        if (qual & code::MemQual::NoRead)
            mods[offset++] = &memoryNoRead;
        if (qual & code::MemQual::NoWrite)
            mods[offset++] = &memoryNoWrite;
        return declareUniform<T, 4>(name, { mods, offset });
    }*/
};

struct StageDriver : StageDeclDriver {
    using StageDeclDriver::StageDeclDriver;

    template<typename T, int_t N = 0>
    constexpr auto builtin(std::string_view name, ArrayView<const VarDeclModifier* const> modsIn = {}) {
        stx::Array<const VarDeclModifier*, N + 1> mods = { &builtinIndicator };
        std::copy_n(modsIn.data(), modsIn.size(), mods.data() + 1);
        return declareGlobal<T, N + 1>(name, { mods.data(), modsIn.size() + 1 });
    }
};

struct ComputeGroupSizeModifier : VarDeclModifier {
    static constexpr stx::FixedString NAME = "compute-group-size";
    int_t x = 1, y = 1, z = 1;
    constexpr ComputeGroupSizeModifier()
        : VarDeclModifier { NAME, true } { }
    static constexpr const ComputeGroupSizeModifier* find(const StageDecl* stage) {
        const ComputeGroupSizeModifier* ret = nullptr;
        stage->globals.decls.forEachEntry([&](const VarDecl* decl) {
            if (decl->modifiers.empty())
                return;
            const VarDeclModifier* mod = decl->modifiers.front();
            if (mod->name == NAME)
                ret = static_cast<const ComputeGroupSizeModifier*>(mod);
        });
        return ret;
    }
};
struct ComputeDriver : ShaderDriver, StageDriver {
    DriverVariable<wrapped_vector, 2> localIndex;
    DriverVariable<wrapped_vector, 2> globalIndex;
    ComputeGroupSizeModifier groupSizeMod;

    constexpr ComputeDriver(ASTStorage storage, PseudoVector<CBinding> bindings, PseudoVector<BindingModifier> bindingModifiers)
        : ShaderDriver(storage, bindings, bindingModifiers)
        , StageDriver(storage, StageKind::Compute, &shaderDecl()->uniforms)
        , localIndex(builtin<iVec3, 1>(varName<"gl_LocalInvocationID">(), { &glsl::needsCast }))
        , globalIndex(builtin<iVec3, 1>(varName<"gl_WorkGroupID">(), { &glsl::needsCast })) {
        addStage(stageDecl());
        {
            VarDecl decl { varName<"">(), &VoidType, nullptr };
            VarDecl* newDecl = createVarDecl(&decl, &stageDecl()->globals);
            newDecl->prevDecl = nullptr;
            storage.pushBackOnList(newDecl->modifiers, &builtinIndicator);
            storage.pushBackOnList(newDecl->modifiers, (const VarDeclModifier*)&groupSizeMod);
        }
    }
};

namespace glsl {

    struct StageGenerator : StmtGenerator<StageGenerator> {
        using Base = StmtGenerator<StageGenerator>;
        using Base::Base;

        constexpr void emitUserTypes(const ShaderDecl* shader, const StageDecl* stage) {
            const UserType* userTypes[1000] = {};
            int_t userTypeCount = 0;
            UserTypeRecorder recorder { { userTypes, &userTypeCount } };
            recorder.visitDeclContext(&shader->uniforms);
            recorder.visitStageDecl(stage);
            TypeGenerator typeGenerator = TypeGenerator::fork(*this);
            for (int_t i = 0; i < userTypeCount; i++)
                typeGenerator.visitType(userTypes[i]);
        }

        constexpr void emitUniforms(const VarDeclContext* declContext) {
            declContext->decls.forEachEntry([&](const VarDecl* decl) {
                VERIFY(ShaderGen, decl->modifiers.entryCount >= 2);
                int_t binding = static_cast<const BindingModifier*>(decl->modifiers.get(1))->index;
                uint32_t qual = 0;
                using enum code::MemQual;
                decl->modifiers.forEachEntry([&](const VarDeclModifier* mod) constexpr {
                    if (mod == &memoryCanAlias)
                        qual |= CanAlias;
                    if (mod == &memoryNoRead)
                        qual |= NoRead;
                    if (mod == &memoryNoWrite)
                        qual |= NoWrite;
                });
                auto emitQual = [&]() {
                    if ((qual & CanAlias) == 0)
                        out() << "restrict ";
                    if (qual & NoRead)
                        out() << "writeonly ";
                    if (qual & NoWrite)
                        out() << "readonly ";
                };
                if (decl->modifiers.front() == &uniformBufferIndicator) {
                    out() << Indentation() << "layout(std140, binding=" << binding << ") ";
                    out() << "uniform _uniform_block" << binding << " {\n";
                    indent += 1;
                    out() << Indentation() << decl << ";\n";
                    indent -= 1;
                    out() << Indentation() << "};\n";
                } else if (decl->modifiers.front() == &storageBufferIndicator) {
                    out() << Indentation() << "layout(std140, binding=" << binding << ") ";
                    emitQual();
                    out() << "buffer _buffer_block" << binding << " {\n";
                    indent += 1;
                    out() << Indentation() << decl << ";\n";
                    indent -= 1;
                    out() << Indentation() << "};\n";
                } else if (decl->modifiers.front() == &sampledImageIndicator) {
                    out() << Indentation() << "layout(binding=" << binding << ") ";
                    out() << "uniform " << decl << ";\n";
                } else if (decl->modifiers.front() == &storageImageIndicator) {
                    out() << Indentation() << "layout(rgba8ui, binding=" << binding << ") ";
                    emitQual();
                    out() << "uniform " << decl << ";\n";
                } else {
                    VERIFY_NOT_REACHED(ShaderGen);
                }
            });
        }

        constexpr void emitStage(const ShaderDecl* shader, const StageDecl* stage) {
            emitUserTypes(shader, stage);
            emitUniforms(&shader->uniforms);
            if (stage->kind == StageKind::Compute) {
                const auto* groupSize = ComputeGroupSizeModifier::find(stage);
                if (groupSize) {
                    out() << Indentation()
                          << "layout(local_size_x=" << groupSize->x
                          << ", local_size_y=" << groupSize->y
                          << ", local_size_z=" << groupSize->z << ") in;\n";
                }
            }
            emitEntryPoint(stage->entryPoint);
        }
    };
}

struct ShaderGenInput {
    BasicASTStorage<10000, 10000> storage;
    OversizedArray<CBinding, 1000> bindings;
    OversizedArray<char, 10000> sourceStorage;
    struct SourceRange {
        int_t beginOffset = 0;
        int_t size = 0;
    };
    OversizedArray<SourceRange, 100> sources;
    const ShaderDecl* shaderDecl = nullptr;

    constexpr void setShader(const ShaderDecl* shader) {
        shaderDecl = shader;
        generateSource();
        strip();
    }

    constexpr void generateSource() {
        BasicASTStorage<10000, 10000> transformStorage;
        glsl::StmtTransform transform(transformStorage);
        glsl::StageGenerator generator(sourceStorage);
        ShaderDecl* newShader = transform.createShaderDecl(*shaderDecl);
        newShader->stages.forEachEntry([&](const StageDecl* stage) {
            auto begin = generator.beginSegment();
            generator.emitStage(newShader, stage);
            std::string_view source = generator.completeSegment(begin);
            sources.pushBack({ source.data() - sourceStorage.data(), (int_t)source.size() });
        });
    }

    constexpr void strip() {
        PrevDeclStripper stripper1;
        stripper1.visitShaderDecl(shaderDecl);
        TemporaryModifierStripper stripper2;
        stripper2.visitShaderDecl(shaderDecl);
    }

    template<typename Callback>
    constexpr ShaderGenInput(Callback callback) {
        callback(*this);
    }
};

template<const ShaderGenInput& input>
struct ShaderGenerator : ShaderRecord {
    static constexpr auto makeStages() {
        const char* sourceOutput = staticView<input.sourceStorage>().data();
        stx::Array<StageRecord, input.shaderDecl->stages.entryCount> output;
        int_t index = 0;
        input.shaderDecl->stages.forEachEntry([&](const StageDecl* in) {
            auto& out = output[index];
            out.kind = in->kind;
            const auto& sourceRange = input.sources[index];
            out.source = { sourceOutput + sourceRange.beginOffset, (size_t)sourceRange.size };
            index += 1;
        });
        return output;
    }
    static constexpr auto stagesStatic = makeStages();

    template<typename>
    struct BindingsHolder;
    template<int_t... I>
    struct BindingsHolder<stx::int_sequence<I...>> {
        static inline const stx::Array<Binding, sizeof...(I)> value { { input.bindings[I]... } };
    };

    constexpr ShaderGenerator() {
        shaderDecl = compress<input.shaderDecl>();
        bindings = BindingsHolder<stx::make_int_sequence<input.bindings.size()>>::value;
        stages = stagesStatic;
    }
};

template<void (*func)(ComputeDriver&)>
constexpr ShaderGenInput computeGenInput { [](ShaderGenInput& gen) {
    OversizedArray<BindingModifier, 1000> bindingModifiers;
    ComputeDriver driver(gen.storage, gen.bindings, bindingModifiers);
    func(driver);
    gen.setShader(driver.shaderDecl());
} };
template<void (*func)(ComputeDriver&)>
constexpr ShaderRecord generate() {
    return ShaderGenerator<computeGenInput<func>>();
}

/*template<typename T>
struct VaryingReturnType {
    DriverVariable<get_wrapper<T>, 1> in;
    DriverVariable<get_wrapper<T>, 1> out;
    constexpr VaryingReturnType(std::string_view name, GraphicsStageDriver& inStage, GraphicsStageDriver& outStage)
        : in { inStage.output<T>(name) }, out { outStage.input<T>(name) } { }
};*/

}

#define DEFINE_SHADER(name) \
    template<>              \
    constinit const shadergen::ShaderRecord shadergen::ShaderRecordHolder<name>::value = shadergen::generate<name::defineShader>();
