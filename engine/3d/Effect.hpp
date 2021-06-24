#pragma once

#include "GL/Shader.hpp"

class EffectShaderBuilder;
class EffectShader;

struct EffectBindings {
    Shader::LocationHandle handles[5];
    int textureSlot;
};

// Attributes should be singletons
struct Attribute {
    static std::vector<Attribute*> instances;
    const int id;
    Attribute()
        : id(instances.size()) { instances.push_back(this); }
    static Attribute* fromId(int id) { return instances[id]; }
    virtual void addTo(EffectShaderBuilder&) = 0;
    virtual ~Attribute() { }
};

class AttributeList {
public:
    struct iterator {
        int id;
        uint64_t attributes;
        iterator(uint64_t attributes, int id)
            : attributes(attributes), id(id) { }
        bool operator!=(iterator other) { return id != other.id; }
        iterator& operator++() {
            while (++id < sizeof(attributes) * 8) {
                if (attributes & (1 << id))
                    break;
            }
            return *this;
        }
        Attribute* operator*() { return Attribute::fromId(id); }
    };
    bool has(Attribute* attribute) const { return attributes & (1 << attribute->id); }
    void add(Attribute* attribute) { attributes |= (1 << attribute->id); }
    iterator begin() const { return ++iterator(attributes, -1); }
    iterator end() const { return iterator(attributes, sizeof(attributes) * 8); }
    bool operator==(AttributeList other) const { return attributes == other.attributes; }
    bool operator!=(AttributeList other) const { return attributes != other.attributes; }

private:
    uint64_t attributes = 0;
};

// Effects should be singletons
struct Effect {
    virtual EffectBindings addTo(EffectShaderBuilder&) = 0;
    virtual ~Effect() { }
};

struct EffectInstance {
    Effect* effect;
    EffectInstance(Effect* effect)
        : effect(effect) { }
    virtual void applyTo(EffectShader&, const EffectBindings&) = 0;
    virtual ~EffectInstance() { }
};

class EffectShaderBuilder : public ShaderBuilder {
public:
    const AttributeList attributes;

    Shader::LocationHandle positionAttrib, normalAttrib, modelUniform, viewProjectionUniform;
    EffectShaderBuilder(AttributeList attributes);

private:
    virtual std::string buildVertexSource() override;
    virtual std::string buildFragmentSource() override;
};

class EffectShader : public Shader {
public:
    static const std::string WORLD_POSITION;
    static const std::string WORLD_NORMAL;
    static const std::string SCREEN_POSITION;
    static const std::string LIGHT_INTENSITY;
    static const std::string RAW_POSITION;
    static const std::string RAW_NORMAL;
    static const std::string RAW_TO_WORLD_MATRIX;
    static const std::string WORLD_TO_SCREEN_MATRIX;
    static const std::string FRAGMENT_DIFFUSE_COLOR;

    Shader::LocationHandle positionAttrib, normalAttrib, modelUniform, viewProjectionUniform;
    EffectShader(EffectShaderBuilder& builder)
        : Shader(builder)
        , positionAttrib(builder.positionAttrib)
        , normalAttrib(builder.normalAttrib)
        , modelUniform(builder.modelUniform)
        , viewProjectionUniform(builder.viewProjectionUniform) { }
};
