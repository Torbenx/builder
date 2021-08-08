#include "math/Vec2.hpp"
#include "util/log.hpp"

void Vec2::debug() {
    log<Tag::Math>("Vec2({}, {})", x, y);
}
