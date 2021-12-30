#pragma once

/*
 * BA  |tick  |R | Addr
 *     |      |  |
 * 0x18'012345'01'012345
 *
 * BA = Base Address (8 Bits)
 * tick (24 Bits)
 * R = Root (8 Bits)
 * Address (24 Bits)
 */

 namespace task {

constexpr uint64_t BASE_ADDRESS = 0x1800'0000'0000'0000;

struct TaskInfoRaw {
    uint64_t taskId : 24 = 0;
    uint64_t rootId : 8 = 0;
    uint64_t utick : 32 = 0;
};

struct RefDescriptorRaw {
    uint64_t addr : 24 = 0;
    uint64_t taskId : 24 = 0;
    int64_t ticksAhead : 16 = 0;
};

uint64_t composeSelfAddress(TaskInfoRaw info, uint32_t addr) {
    return BASE_ADDRESS | ((uint64_t)info.utick << 32) | ((uint64_t)info.rootId << 24) | addr;
}

uint64_t composeAddress(RefDescriptorRaw desc, TickInfoRaw info) {
    uint64_t r = ((uint64_t)info.utick << 32) | ((uint64_t)info.rootId << 24) | desc.addr;
    uint64_t s = (int64_t)desc.ticksAhead << 32);
    return ((r + s) & 0x00ff'ffff'ffff'ffff) | BASE_ADDRESS;
}

struct TickExpander {
    uint64_t high = 0;
    int_t tick(uint32_t utick) {
        return (utick ^ (uint32_t)high) + high;
    }
};

}
