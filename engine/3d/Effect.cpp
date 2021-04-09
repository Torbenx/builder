#include "3d/Effect.hpp"

bool EffectList::operator==(const EffectList& other) const {
    if (attributes != other.attributes)
        return false;
    if (entries.size() != other.entries.size())
        return false;
    for (int i = 0; i < entries.size(); i++) {
        if (entries[i].effect != other.entries[i].effect)
            return false;
        if (entries[i].multiplicity != other.entries[i].multiplicity)
            return false;
    }
    return true;
}

void EffectList::add(Effect* effect) {
    auto it = std::lower_bound(entries.begin(), entries.end(), effect);
    if (it->effect == effect) {
        it->multiplicity += 1;
    } else {
        entries.insert(it, { effect, 1 });
    }
}
