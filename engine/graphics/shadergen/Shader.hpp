#pragma once

#include "containers/ArrayView.hpp"
#include "containers/FixedString.hpp"
#include "graphics/Buffer.hpp"

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

struct BindingBase {
    enum class Type {
        UniformBuffer,
        StorageBuffer,
        SampledImage,
        StorageImage,
    };
    enum class Access {
        None,
        ReadOnly,
        WriteOnly,
        ReadWrite,
    };
    const char* name = nullptr;
    int index = 0;
    Type type = (Type)0;
    Access access = (Access)0;
    gfx::TexelComponents imageFormat = (gfx::TexelComponents)0;
    uint32_t stageMask = 0;
};
struct Binding : BindingBase {
    int_t offset = 0;
};

struct ShaderDecl;
struct StageRecord {
    StageKind kind;
    std::string_view source;
};

struct ShaderRecord {
    const ShaderDecl* shaderDecl = nullptr;
    ArrayView<const Binding> bindings;
    ArrayView<const StageRecord> stages;
};
template<typename Impl>
struct ShaderRecordHolder {
    static constinit const ShaderRecord value;
};

#ifndef BEGIN_GEN_STRUCT
#define BEGIN_GEN_STRUCT(name)
#define GEN_FIELD(name) name;
#define END_GEN_STRUCT()
#endif

}