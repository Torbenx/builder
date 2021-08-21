#pragma once

struct Property {
    /*struct Destination {
        uint32_t buffer;
        uint32_t offset
    };*/
    struct Instance {
        uint32_t representation;
        uint32_t buffer;
        uint32_t offset;
    };
    std::vector<Instance> instances;
};

/* property, representation -> propertyIndex, representation, format */

template<class T>
struct PropertyHolder : T {

};
