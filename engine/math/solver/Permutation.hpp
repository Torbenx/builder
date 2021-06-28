#pragma once

#include <vector>

template<typename size_type>
class Permutation {
public:
    size_type& operator[](size_type in) {
        return map[in];
    }

    void setIdentity(size_type size) {
        map.resize(size);
        inv.resize(size);
        for (size_type i = 0; i < size; i++) {
            map[i] = i;
            inv[i] = i;
        }
    }

    size_type inverse(size_type in) {
        return inv[in];
    }

    void swap(size_type in1, size_type in2) {
        // map[inv[in1]] => in1;
        // map[inv[in2]] => in2;

        std::swap(inv[in1], inv[in2]);

        // map[inv[in1]] => in2;
        // map[inv[in2]] => in1;

        map[inv[in1]] = in1;
        map[inv[in2]] = in2;
    }

    size_type size() {
        return map.size();
    }

    void expandBy(size_type increase) {
        size_type oldSize = size();
        map.resize(oldSize + increase);
        inv.resize(oldSize + increase);
        for (size_type i = oldSize; i < oldSize + increase; i++) {
            map[i] = i;
            inv[i] = i;
        }
    }

private:
    std::vector<size_type> map, inv;
};
