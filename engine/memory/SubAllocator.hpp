#pragma once

#include "memory/Allocator.hpp"

template<typename SubAllocator, typename BaseAllocator>
class SubAllocatorBase : public AllocatorBase<SubAllocator> {
public:
    // SubAllocator should have a constructor taking Data& and BaseAllocator
    struct Inline : SubAllocator {
        struct Object : BaseAllocator::Object, SubAllocator::Data { };
        using Parameter = BaseAllocator::Parameter;

        Inline(Object& object, Parameter param)
            : SubAllocator((SubAllocator::Data&)object, BaseAllocator { object, param }) { }
    };
    
    struct InlinePtr : SubAllocator {
        struct Object : BaseAllocator::Object {
            BaseAllocator::Ptr<SubAllocator::Data> subAllocatorPtr;
        };
        using Parameter = BaseAllocator::Parameter;

        InlinePtr(Object& object, BaseAllocator alloc)
            : SubAllocator(*alloc[object.subAllocatorPtr], alloc) { }
        InlinePtr(Object& object, Parameter param)
            : InlinePtr(object, BaseAllocator { object, param }) { }
    };

    struct ParameterRef : SubAllocator {
        struct Object : BaseAllocator::Object { };
        struct Parameter : BaseAllocator::Parameter {
            SubAllocator::Data& data;
            Parameter(SubAllocator::Data& data, BaseAllocator::Parameter param = {})
                : BaseAllocator::Parameter(param), data(data) { }
        };

        ParameterRef(Object& object, Parameter param)
            : SubAllocator(param.data, BaseAllocator { object, param }) { }
    };
};
