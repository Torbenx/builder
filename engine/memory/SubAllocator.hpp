#pragma once

#include "memory/Allocator.hpp"

template<typename SubAllocator, typename BaseAllocator, typename uintptr_t_p = typename BaseAllocator::uintptr_t, typename RawPtr_p = BasicRawPtr<uintptr_t_p>>
struct SubAllocatorBase : AllocatorBase<SubAllocator, uintptr_t_p, RawPtr_p> {
    // SubAllocator should have a constructor taking Data& and BaseAllocator
    struct Inline : SubAllocator {
        struct Object : BaseAllocator::Object, SubAllocator::Data { };
        using Parameter = typename BaseAllocator::Parameter;

        Inline(Object& object, Parameter param)
            : SubAllocator((typename SubAllocator::Data&)object, BaseAllocator { object, param }) { }
    };

    /*struct InlinePtr : SubAllocator {
        struct Object : BaseAllocator::Object {
            BaseAllocator::Ptr<SubAllocator::Data> subAllocatorPtr;
        };
        using Parameter = typename BaseAllocator::Parameter;

        InlinePtr(Object& object, BaseAllocator alloc)
            : SubAllocator(*alloc[object.subAllocatorPtr], alloc) { }
        InlinePtr(Object& object, Parameter param)
            : InlinePtr(object, BaseAllocator { object, param }) { }
    };*/

    struct ParameterRef : SubAllocator {
        using Object = typename BaseAllocator::Object;
        struct Parameter : BaseAllocator::Parameter {
            typename SubAllocator::Data& data;
            Parameter(typename SubAllocator::Data& data, typename BaseAllocator::Parameter param = {})
                : BaseAllocator::Parameter(param), data(data) { }
        };

        ParameterRef(Object& object, Parameter param)
            : SubAllocator(param.data, BaseAllocator { object, param }) { }
    };

    using uintptr_t = uintptr_t_p;
    using RawPtr = RawPtr_p;
    template<typename T = void>
    using Ptr = ::Ptr<RawPtr, T>;

    BaseAllocator baseAllocator;

    SubAllocatorBase(BaseAllocator baseAllocator)
        : baseAllocator(baseAllocator) { }

    template<typename T>
    T* operator[](Ptr<T> ptr) {
        return baseAllocator[ptr];
    }
};
