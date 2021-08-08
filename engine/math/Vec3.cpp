#include "math/Vec3.hpp"
#include "util/log.hpp"
#include <cmath>

void Vec3::debug() {
    log<Tag::Math>("Vec3({}, {}, {})", x, y, z);
}
