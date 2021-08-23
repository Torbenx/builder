#pragma once

#include "animation/Property.hpp"

struct Color : Property {
    static inline const Representation<Color> vector = { Property::Format::Vec3 };
};
