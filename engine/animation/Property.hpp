#pragma once

#include "containers/ArrayBuilder.hpp"

class Property {
public:
    enum Format {
        Scalar,
        Vec2,
        Vec3,
        Vec4,
        Mat4,
    };
    /*struct Destination {
        uint32_t buffer;
        uint32_t offset
    };*/
    struct Instance {
        uint32_t representation;
        uint32_t buffer;
        uint32_t offset;
    };

    template<class P>
    struct Representation {
        Representation(Format format)
            : format(format) {
            static int_t nextId = 0;
            id = nextId++;
        }
        Format format;
        int_t id;
    };

private:
    struct Op {

    };

    ArrayBuilder<Op> ops;
};
