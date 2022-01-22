#pragma once

class BindingSolver : public LinearProgramming {
public:
    struct Usage {
        uint32_t constraintIndex = 0;
        uint32_t resources[];
    };

    struct Resource {
        
    };
};
