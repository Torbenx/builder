#pragma once

#include "BuildDriver.hpp"
#include "GLSLGenerator.hpp"

namespace shadergen {

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

inline constexpr VarDeclModifier uniformIndicator { "uniform" };
inline constexpr VarDeclModifier uniformImageIndicator { "uniform-image" };
inline constexpr VarDeclModifier arrayBufferIndicator { "array-buffer" };

struct BindingModifier : VarDeclModifier {
    int_t binding = -1;
    constexpr BindingModifier() = default;
    constexpr BindingModifier(int_t binging)
        : VarDeclModifier("binding", true) { }
};

struct ComputeDriver : StageDriver {
    int_t uniformIndex = 0;
    int_t imageIndex = 0;
    int_t bufferIndex = 0;
    PseudoVector<BindingModifier> bindings;

    constexpr ComputeDriver(ASTStorage storage, PseudoVector<BindingModifier> bindings)
        : StageDriver(storage), bindings(bindings) { }

    template<typename T>
    constexpr auto uniformBlock(std::string_view name) {
        return declareGlobal<T, 2>(name, { &uniformIndicator, &bindings.pushBack({ uniformIndex++ }) });
    }
    template<typename T>
    constexpr auto uniformImage(std::string_view name, code::MemQual qual) {
        bindings.pushBack({ 1 });
        const VarDeclModifier* mods[5] = { &uniformImageIndicator, &bindings.pushBack({ imageIndex++ }) };
        int_t offset = 2;
        if (qual & code::MemQual::CanAlias)
            mods[offset++] = &memoryCanAlias;
        if (qual & code::MemQual::NoRead)
            mods[offset++] = &memoryNoRead;
        if (qual & code::MemQual::NoWrite)
            mods[offset++] = &memoryNoWrite;
        return declareGlobal<T, 4>(name, { mods, offset });
    }
    template<typename T>
    constexpr auto arrayBuffer(std::string_view name, code::MemQual qual) {
        const VarDeclModifier* mods[5] = { &arrayBufferIndicator, &bindings.pushBack({ bufferIndex++ }) };
        int_t offset = 2;
        if (qual & code::MemQual::CanAlias)
            mods[offset++] = &memoryCanAlias;
        if (qual & code::MemQual::NoRead)
            mods[offset++] = &memoryNoRead;
        if (qual & code::MemQual::NoWrite)
            mods[offset++] = &memoryNoWrite;
        return declareGlobal<T[], 4>(name, { mods, offset });
    }
};

namespace glsl {

    struct ComputeStageGenerator : StmtGenerator<ComputeStageGenerator> {
        using Base = StmtGenerator<ComputeStageGenerator>;
        using Base::Base;

        constexpr void emitUserTypes(const StageDecl* stage) {
            const UserType* userTypes[1000] = {};
            int_t userTypeCount = 0;
            UserTypeRecorder recorder { { userTypes, &userTypeCount } };
            recorder.visitStageDecl(stage);
            TypeGenerator typeGenerator = TypeGenerator::fork(*this);
            for (int_t i = 0; i < userTypeCount; i++)
                typeGenerator.visitType(userTypes[i]);
        }

        constexpr void emitGlobals(const VarDeclContext* declContext) {
            declContext->decls.forEachEntry([&](const VarDecl* decl) {
                VERIFY(ShaderGen, decl->modifiers.entryCount >= 1);
                if (decl->modifiers.front() == &uniformIndicator) {

                } else if (decl->modifiers.front() == &uniformImageIndicator) {

                } else if (decl->modifiers.front() == &arrayBufferIndicator) {
                }
            });
        }

        constexpr void visitStageDecl(const StageDecl* stage) {
            emitUserTypes(stage);
            emitGlobals(&stage->globals);
            out() << "void main() " << stage->entryPoint << "\n";
        }
    };

}

inline constexpr VarDeclModifier inputIndicator { "input" };
inline constexpr VarDeclModifier outputIndicator { "output" };

struct GraphicsStageDriver : StageDriver {
    constexpr GraphicsStageDriver(ASTStorage storage)
        : StageDriver(storage) { }

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
    template<typename T>
    constexpr auto builtin(std::string_view name) {
        return declareGlobal<T, 1>(name);
    }
};

template<typename T>
struct VaryingReturnType {
    DriverVariable<T, 1> in;
    DriverVariable<T, 1> out;
    constexpr VaryingReturnType(std::string_view name, GraphicsStageDriver& inStage, GraphicsStageDriver& outStage)
        : in { inStage.output<T>(name) }, out { outStage.input<T>(name) } { }
};

struct GraphicsDriver {
    StageDriver vert;
    StageDriver frag;

    constexpr GraphicsDriver(ASTStorage storage)
        : vert(storage), frag(storage) { }

    template<typename T>
    constexpr VaryingReturnType<T> varying(std::string_view name) {
        return { name, vert, frag };
    }
};

}