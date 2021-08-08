#include "math/Vec4.hpp"
#include "math/constants.hpp"
#include "util/log.hpp"
#include <algorithm>
#include <cmath>

void Vec4::clampToW() {
    x = std::clamp(x, -std::abs(w), std::abs(w));
    y = std::clamp(y, -std::abs(w), std::abs(w));
    z = std::clamp(z, -std::abs(w), std::abs(w));
}

void Vec4::debug() {
    log<Tag::Math>("Vec4({} ,{} ,{} ,{} )", x, y, z, w);
}
