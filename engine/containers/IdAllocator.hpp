#pragma once

#include "util/types.hpp"
#include "util/log.hpp"
#include <iostream>
#include <set>
#include <cstring>

template<typename T = id_t, size_t N = 20>
class IdAllocator {
public:
    IdAllocator(T min = 0, T max = std::numeric_limits<T>::max()) {
        buckets.emplace(min, max - 1);
    }

    T allocateId() {
        auto it = buckets.begin();
        T id = it->takeFront();
        if (it->size == 0) {
            buckets.erase(it);
        }
        return id;
    }

    void debug() {
        for (auto& b : buckets) {
            b.debug();
        }
    }

    void freeId(T id) {
        auto it = buckets.lower_bound(id);
        // it should never be end()
        if (it->blub(id)) {
            mergeBoth(it);
            return;
        }
        if ((!it->isRange) && id > it->first()) {
            split(it, id);
            return;
        }
        if (it == buckets.begin()) {
            if (!it->isRange) {
                split(it, id);
                return;
            }
            buckets.emplace_hint(it, id);
            return;
        }
        auto prevIt = it;
        prevIt--;
        if (prevIt->isRange) {
            if (prevIt->last() + 1 == id) {
                prevIt->rangeExtendBackBy(1);
                return;
            }
            if (prevIt->size < N) {
                //optimize
                prevIt->expandRange();
                prevIt->pushBack(id);
                return;
            }
            if (!it->isRange) {
                split(it, id);
                return;
            }
            buckets.emplace_hint(it, id);
            return;
        }
        if (prevIt->size < N) {
            prevIt->pushBack(id);
            return;
        }
        split(prevIt, id);
    }

    size_t bucketCount() {
        return buckets.size();
    }

private:
    struct Bucket {
        mutable T elements[N];
        mutable size_t size;
        mutable bool isRange;

        Bucket(T start, T end) {
            size = end - start + 1;
            elements[N - 1] = end;
            isRange = true;
        }

        Bucket(T id) {
            size = 1;
            elements[N - 1] = id;
            isRange = true;
        }

        T& last() const { return elements[N - 1]; }
        T& first() const { return elements[N - size]; }
        T rangeFirst() const { return last() - size + 1; }

        bool operator<(const Bucket& other) const { return last() < other.last(); }
        bool operator<(T id) const { return last() < id; }

        void expandRange() const {
            for (size_t i = 0; i < size; i++) {
                elements[N - size + i] = rangeFirst() + i;
            }
            isRange = false;
        }

        bool blub(T id) const {
            if (isRange) {
                if (id + 1 == rangeFirst()) {
                    rangeExtendFrontBy(1);
                    return true;
                }
                if (size < N) {
                    expandRange();
                    size++;
                    elements[N - size] = id;
                    return true;
                }
                return false;
            }
            if (std::min(id, elements[N - size]) + size == last()) {
                size++;
                isRange = true;
                return true;
            }
            if (size < N) {
                insert(id);
                return true;
            }
            return false;
        }

        void rangeExtendBackBy(T by) const {
            last() += by;
            size += by;
        }

        void rangeExtendFrontBy(T by) const {
            size += by;
        }

        void insert(T id) const {
            size_t offset = std::distance(&elements[N - size], std::lower_bound(&elements[N - size], &elements[N], id));
            std::memmove(&elements[N - size - 1], &elements[N - size], offset * sizeof(T));
            elements[N - size + offset - 1] = id;
            size++;
        }

        void pushBack(T id) const {
            std::memmove(&elements[N - size - 1], &elements[N - size], size * sizeof(T));
            elements[N - 1] = id;
            size++;
        }

        void updateRange() const {
            isRange = first() + size - 1 == last();
        }

        void debug() const {
            if (isRange) {
                std::cout << rangeFirst() << " - " << last() << "\n";
            } else {
                for (size_t i = N - size; i < N; i++) {
                    std::cout << elements[i] << ", ";
                }
                std::cout << "\n";
            }
        }

        T takeFront() const {
            T id;
            if (isRange) {
                id = rangeFirst();
            } else {
                id = first();
            }
            size--;
            return id;
        }
    };

    void merge(typename std::set<Bucket>::iterator it1, typename std::set<Bucket>::iterator it2) {
        if (it1->isRange && it2->isRange && it1->last() + 1 == it2->rangeFirst()) {
            it2->rangeExtendFrontBy(it1->size);
            buckets.erase(it1);
        }
    }

    void mergePrev(typename std::set<Bucket>::iterator it) {
        if (it != buckets.begin()) {
            auto prevIt = it;
            prevIt--;
            merge(prevIt, it);
        }
    }

    void mergeNext(typename std::set<Bucket>::iterator it) {
        auto nextIt = it;
        nextIt++;
        if (nextIt != buckets.end()) {
            merge(it, nextIt);
        }
    }

    void mergeBoth(typename std::set<Bucket>::iterator it) {
        mergePrev(it);
        mergeNext(it);
    }

    void split(typename std::set<Bucket>::iterator it2, T id) {
        size_t size1 = N >> 1;
        size_t size2 = N - size1;
        T pivot = it2->elements[size1 - 1];
        auto it1 = buckets.emplace_hint(it2, pivot);
        std::memcpy(&it1->elements[N - size1], &it2->elements[0], (size1 - 1) * sizeof(T));
        it1->size = size1;
        it2->size = size2;
        if (id < pivot) {
            it1->insert(id);
        } else {
            it2->insert(id);
        }
        it1->updateRange();
        it2->updateRange();
        mergePrev(it1);
        mergeNext(it2);
    }

    std::set<Bucket> buckets;
};
