#include "util/log.hpp"
#include "conditions.hpp"

namespace evt {

void test() {
    CompiledCondition cond { atLeast<3>("unbound"_ev) && upperBound<1>("bound"_ev) };
    VDUMP(cond.output.startMask, ":");
    VDUMP(cond.output.keyMask, ":");
    for (auto leaf : cond.output.leafs) {
        VDUMP(leaf.mask, ":");
    }
}

}