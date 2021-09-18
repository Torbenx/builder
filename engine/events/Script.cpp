#include "Script.hpp"

void init_script() {
    new (&script_detail::allocator.allocator) MemoryAllocator32();
}