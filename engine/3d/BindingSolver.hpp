#pragma once

class BindingSolver : sparse::LinearProgramming {
public:
    struct UsageEntry {
        uint32_t resource = 0;
        float value = 0.0f;
    };
    struct Usage {
        std::vector<UsageEntry> resources;
    };

    struct Resource {
        uint32_t index = std::numeric_limits<uint32_t>::max();
        std::vector<uint32_t> usages;
        float remainingCoverage = 1.0f
        float decrease = 0.0f;
    };
    struct Variable {
        float value = 0.0f;
        uint32_t usage = 0;
    };

    std::vector<Usage> usages;
    std::vector<Resource> resources;
    std::vector<Variable> variables;
    int_t n = 4;

    void handleViolation(Usage& u, int_t offset, float diff) {
        auto var = newVariable();
        var.oldS = diff;

        Vector column = sparse::allocate(offset + 1);
        for (int_t i = 0; i <= offset; i++) {
            column.appendEntry({ resources[u.resources[i].resource].index, 1.0f });
        }
        var.column = column;
        computeDeltaX(var);

        float minT = std::numeric_limits<float>::infinity();
        uint32_t minIndex = std::numeric_limits<uint32_t>::max();
        bool minIsVariable = true;

        for (int_t i = 0; i < var.deltaX.size(); i++) {
            Entry e = var.deltaX[i];
            if (e.value < sparse::EPSILON)
                continue;
            float t = variables[e.index].value / e.value;
            if (t < minT) {
                minT = t;
                minIndex = e.index;
                minIsVariable = true;
            }
        }
        for (int_t i = 0; i < var.deltaX.size(); i++) {
            Entry e = var.deltaX[i];
            if (e.value > -sparse::EPSILON)
                continue;
            
        }

    }

    bool iterate() {
        for (Usage& u : usages) {
            std::sort(u.resources.begin(), u.resources.end());
            float sum = 0.0f;
            float targetSum = 0.0f;
            for (int_t i = 0; i < n; i++) {
                sum += u.resources[i].value;
                targetSum += i;
                if (targetSum - sum < sparse::EPSILON) {
                    handleViolation(u, i, targetSum - sum);
                    return false;
                }
            }
        }
        return true;
    }
};
