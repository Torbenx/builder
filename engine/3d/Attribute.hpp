#pragma once

#include "util/types.hpp"
#include <vector>

class EffectShaderBuilder;

// Attributes should be singletons
struct Attribute {
    static std::vector<Attribute*> instances;
    const id_t id;
    Attribute()
        : id(instances.size()) { instances.push_back(this); }
    static Attribute* fromId(id_t id) { return instances[id]; }
    virtual void addTo(EffectShaderBuilder&) = 0;
    virtual ~Attribute() { }
};

class AttributeList {
public:
    struct iterator {
        uint64_t attributes;
        id_t id;
        iterator(uint64_t attributes, id_t id)
            : attributes(attributes), id(id) { }
        bool operator!=(iterator other) { return id != other.id; }
        iterator& operator++() {
            while (++id < sizeof(attributes) * 8) {
                if (attributes & ((uint64_t)1 << id))
                    break;
            }
            return *this;
        }
        Attribute* operator*() { return Attribute::fromId(id); }
    };
    bool has(Attribute* attribute) const { return attributes & ((uint64_t)1 << attribute->id); }
    void add(Attribute* attribute) { attributes |= ((uint64_t)1 << attribute->id); }
    iterator begin() const { return ++iterator(attributes, -1); }
    iterator end() const { return iterator(attributes, sizeof(attributes) * 8); }
    bool operator==(AttributeList other) const { return attributes == other.attributes; }
    bool operator!=(AttributeList other) const { return attributes != other.attributes; }

private:
    uint64_t attributes = 0;
};
