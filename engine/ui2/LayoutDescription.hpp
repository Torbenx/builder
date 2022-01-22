#pragma once

namespace ui {

template<typename Alloc>
class LayoutDescriptionBase {
private:
    Alloc alloc;

public:
    struct LinkPtr {
        uint16_t id = 0;
    };
    struct Link {
        uint16_t size = 0;
        LinkPtr next;
    };
    struct Variable {
        LinkPtr firstLink;
        LinkPtr lastLink;
    };
    struct VariablePtr {
        uint16_t id = 0;
    };
    constexpr VariablePtr CONSTANT_ZERO = { 1 };
    constexpr VariablePtr CONSTANT_ONE = { 2 };
    constexpr uint16_t FIRST_VARIABLE_ID = 3;
    constexpr uint16_t FIRST_LINK_ID = 1;

    std::vector<VariablePtr> paramters;

    constexpr VariablePtr newVariable() {
        return alloc.newVariable();
    }
    constexpr VariablePtr newParamter() {
        VariablePtr ptr = alloc.newVariable();
        paramters.push_back(ptr);
        return ptr;
    }
    constexpr Variable& variable(VariablePtr ptr) { return alloc.variable(ptr); }

    constexpr void addEquation(ArrayView<const VariablePtr> vars, ArrayView<const float> factors) {
        VERIFY(UI, vars.size() == factors.size());
        int_t linkSize = vars.size() - 1;
        for (int_t i = 0; i < vars.size(); i++) {
            LinkPtr linkPtr = alloc.newLink(linkSize);
            link(linkPtr).size = linkSize;

            ArrayView<VariablePtr> linkVars = alloc.variables(linkPtr, linkSize);
            ArrayView<float> linkFactors = alloc.factors(linkPtr, linkSize);
            float invFactor = -1.0f / factors[i];
            for (int_t j = 0; j < linkSize; j++) {
                int_t k = j < i ? j : j + 1;
                linkVars[j] = vars[k];
                linkFactors[j] = factors[k] * invFactor;
            }

            Variable& var = alloc.variable(vars[i]);
            if (var.lastLink)
                alloc.link(var.lastLink).next = linkPtr;
            else
                var.firstLink = linkPtr;
            var.lastLink = linkPtr;
        }
    }
};

struct ConstexprLayoutDescriptionAllocator {
    using LD = LayoutDescriptionBase<ConstexprLayoutDescriptionAllocator>;

    struct LinkStorage {
        LD::Link link;
        std::unique_ptr<uint16_t[]> variables;
        std::unique_ptr<float[]> factors;
    };
    std::vector<LinkStorage> links;
    std::vector<LD::Variable> vars;

    constexpr ConstexprLayoutDescriptionAllocator() {
        link.resize(LD::FIRST_LINK_ID);
        vars.resize(LD::FIRST_VARIABLE_ID);
    }
    constexpr ArrayView<LD::VariablePtr> variables(LD::LinkPtr ptr, int_t size) {
        return { links[ptr.id].variables, size };
    }
    constexpr ArrayView<float> factors(LD::LinkPtr ptr, int_t size) {
        return { links[ptr.id].factors, size };
    }
    constexpr LD::Link& link(LD::LinkPtr ptr) {
        return links[ptr.id].link;
    }
    constexpr LD::Variable& variable(LD::VariablePtr ptr) {
        return vars[ptr.id];
    }
    constexpr LD::VariablePtr newVariable() {
        LD::VariablePtr r = { (uint16_t)vars.size() };
        vars.resize(vars.size() + 1);
        return r;
    }
    constexpr LD::LinkPtr newLink(int_t size) {
        LD::LinkPtr r = { (uint16_t)links.size() };
        links.emplace_back({
            .variables = std::make_unique<uint16_t[]>(size),
            .factors = std::make_unique<float[]>(size),
        });
        return r;
    }
};

using ConstexprLayoutDescription = LayoutDescriptionBase<ConstexprLayoutDescriptionAllocator>;

template<typename Alloc>
struct Solver {
    using LD = typename LayoutDescriptionBase<Alloc>;
    using LinkPtr = LD::LinkPtr;
    using VariablePtr = LD::VariablePtr;

    struct Elimination {
        VariablePtr variable;
        VariablePtr constant;
        float variableFactor;
        float constantFactor;
    };
    struct Solution {
        bool isValid();
    };

    LD& desc;

    bool isValidSolution(float);
    bool isLocked(VariablePtr varPtr);
    void lock(VariablePtr varPtr);
    void unlock(VariablePtr varPtr);

    struct ScopedLocker;

    Solution solveFor(VariablePtr varPtr, LinkPtr from) {
        if (isLocked(varPtr))
            return Solution::identity(varPtr);
        ScopedLocker l(this, varPtr);

        Variable& var = desc.variable(varPtr);
        LinkPtr linkPtr = var.first;
        if (!linkPtr) [[unlikely]]
            // no eqution involves this variable
            // which also means we must be at top level
            throw ...;

        int_t equations = 0;
        int_t unknowns = 0;
        do {
            if (linkPtr == from)
                continue;
            Link& link = desc.link(linkPtr);
            auto vars = desc.variables(linkPtr, link.size);
            auto factors = desc.factors(linkPtr, link.size);

            bool anyInvalid = false;
            for (int_t i = 0; i < link.size; i++) {
                Solution sol = solveFor(vars[i], linkPtr);
                if (!sol.isValid()) {
                    anyInvalid = true;
                    break;
                }

            }
            if (anyInvalid)
                continue;
        } while (linkPtr = desc.link(linkPtr).next);
    }
};

}
