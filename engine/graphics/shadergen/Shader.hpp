#pragma once

#include "containers/ArrayView.hpp"
#include "containers/FixedString.hpp"

namespace Tag {
struct ShaderGen {
    static constexpr char tag[] = "shadergen";
    static constexpr bool verifyEnabled = true;
};
}

namespace shadergen {

enum class StageKind {
    Compute,
    Vertex,
    Fragment,
};

struct Binding {
    enum class Type {
        UniformBuffer,
        StorageBuffer,
        SampledImage,
        StorageImage,
    };
    enum class Access {
        None = 0,
        ReadOnly = 1,
        WriteOnly = 2,
        ReadWrite = 3,
    };
    const char* name = nullptr;
    int index = 0;
    Type type = (Type)0;
    Access access = (Access)0;
    uint32_t stageMask = 0;
};

struct ShaderDecl;
struct Stage {
    StageKind kind;
    std::string_view source;
};

struct Shader {
    const ShaderDecl* shaderDecl = nullptr;
    ArrayView<const Binding> bindings;
    ArrayView<const Stage> stages;
    constexpr Binding findBinding(std::string_view name) const {
        for (auto& binding : bindings) {
            if (name == binding.name)
                return binding;
        }
        VERIFY_NOT_REACHED(ShaderGen);
        return {};
    }
};
template<const Shader& shader, stx::FixedString name>
constexpr Binding getBinding = shader.findBinding(name);

}