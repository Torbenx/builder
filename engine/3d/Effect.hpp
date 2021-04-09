#pragma once

#include "GL/Shader.hpp"

class EffectShaderBuilder;
class EffectShader;

struct EffectLocationHandle {
    int handle;
};

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
    uint64_t attributes;
};

// Effects should be singletons
struct Effect {
    virtual EffectLocationHandle addTo(EffectShaderBuilder&, const AttributeList&) = 0;
    virtual ~Effect() { }
};

class EffectList {
public:
    AttributeList attributes;
    bool operator==(const EffectList&) const;
    void add(Effect*);

private:
    struct Entry {
        Effect* effect;
        int multiplicity;
        bool operator<(Effect* effect) const { return this->effect < effect; }
    };
    std::vector<Entry> entries;
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
    EffectLocationHandle addEffect(EffectBindings bindings) {
        int handle = effectBindings.size();
        effectBindings.push_back(bindings);
        return { handle };
    }

private:
    std::vector<EffectBindings> effectBindings;
    friend EffectShader;
};

class EffectShader : public Shader {
public:
    EffectShader(EffectShaderBuilder builder)
        : Shader(builder), effectBindings(builder.effectBindings) { }

    void applyEffect(EffectLocationHandle location, EffectInstance& effect) {
        effect.applyTo(*this, effectBindings[location.handle]);
    }

private:
    std::vector<EffectBindings> effectBindings;
};
