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

struct BindingBase {
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

}