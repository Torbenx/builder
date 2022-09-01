#pragma once

#include "platform/Window.hpp"
#include "util/target.hpp"

/*namespace Tag {
struct GL {
    static constexpr char tag[] = "gl";
    static constexpr bool verifyEnabled = true;
};
}*/

#ifdef PLATFORM_WINDOWS
#include "platform/windows/glcore.hpp"
#endif

namespace gl {
void initialize();
void attachWindow(plt::Window* window);
}