#pragma once

#include "math/lp/LinearProgram.hpp"

class BindingSolver : public sparse::UnderConstraintLinearProgram {
private:
    struct Usage {
        std::array<uint32_t, 4> resources;
    };

    struct Resource {
        float value = 0.0f;
    };

    std::vector<Usage> usages;
    std::vector<Resource> resources;
    static constexpr int_t n = 4;

    void handleViolation(Usage& u, int_t offset, float diff) {
        sparse::Vector column = sparse::allocate(offset + 1);
        for (int_t i = 0; i <= offset; i++) {
            column.appendEntry({ u.resources[i], 1.0f });
        }
        column.resizeTo(offset + 1);
        column.sort();
        sparse::Vector update = subOptimalVariable(std::move(column), -diff);
        for (auto e : update)
            resources[e.index].value += e.value;
    }

public:
    uint32_t addResource() {
        uint32_t res = addConstraint(1.0f);
        resources.resize(resources.size() + 1);
        return res;
    }

    void addUsage(ArrayView<const uint32_t> resources) {
        Usage& u = usages.emplace_back();
        std::copy(resources.begin(), resources.end(), u.resources.begin());
    }

    bool iterate() {
        bool anyViolations = false;
        for (Usage& u : usages) {
            bool anyViolationsInUsage;
            do {
                anyViolationsInUsage = false;
                std::sort(u.resources.begin(), u.resources.end(),
                    [&](const uint32_t& res1, const uint32_t& res2) {
                        return resources[res1].value < resources[res2].value;
                    });
                float sum = 0.0f;
                float targetSum = 0.0f;
                float maxDiff = 0.0f;
                int_t maxDiffOffset = n;
                for (int_t i = 0; i < n; i++) {
                    sum += resources[u.resources[i]].value;
                    targetSum += i;
                    float diff = targetSum - sum;
                    if (diff > maxDiff + sparse::EPSILON){
                        maxDiff = diff;
                        maxDiffOffset = i;
                    }
                }
                if (maxDiffOffset != n) {
                    handleViolation(u, maxDiffOffset, maxDiff);
                    anyViolations = true;
                    anyViolationsInUsage = true;
                    //debug();
                }
            } while(anyViolationsInUsage);
        }
        return anyViolations;
    }

    void debug() {
        sparse::UnderConstraintLinearProgram::debug();
        for (int_t i = 0; i < resources.size(); i++) {
            log("{}: {}", i, resources[i].value);
        }
    }
};
