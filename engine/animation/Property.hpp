#pragma once

struct Property {
    struct Representation {
        sid_t id;
    };
    struct Destination {
        unsigned int buffer : 14;
        unsigned int offset : 18;
    };
    struct Instance {
        Representation rep;
        Destination dest;
    };
    std::vector<Instance> instances;
};
