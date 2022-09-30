#include "util/log.hpp"
#include "conditions.hpp"

namespace evt {

void test() {
    BinaryState b1, b2, b3;
    CompiledCondition cond { (b1 && b2 || b3) && OnExecutor(nullptr) };
    VDUMP(cond.output.startMask, ":#b");
    VDUMP(cond.output.keyMask, ":#b");
}

}