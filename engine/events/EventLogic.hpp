#pragma once

#include <tuple>

namespace event_logic {

struct LogicSet {
    int_t indices[64] = {};
    int_t offsets[65] = {};
    int_t size = 0;

    constexpr uint64_t bitmask(int_t index) const {
        int_t varCount = offsets[size];
        uint64_t r = 0;
        for (int_t i = 0; i < varCount; i++) {
            if (indices[i] == index) {
                r |= 1 << i;
            }
        }
        return r;
    }
};

constexpr LogicSet operator||(const LogicSet& a, const LogicSet& b) {
    LogicSet r;
    r.size = a.size + b.size;

    int_t aIndexCount = a.offsets[a.size];
    int_t bIndexCount = b.offsets[b.size];

    for (int_t i = 0; i < aIndexCount; i++)
        r.indices[i] = a.indices[i];
    for (int_t i = 0; i < a.size; i++)
        r.offsets[i] = a.offsets[i];

    for (int_t i = 0; i < bIndexCount; i++)
        r.indices[aIndexCount + i] = b.indices[i];
    for (int_t i = 0; i < b.size + 1; i++)
        r.offsets[a.size + i] = aIndexCount + b.offsets[i];

    return r;
}

constexpr LogicSet operator&&(const LogicSet& a, const LogicSet& b) {
    LogicSet r;
    r.size = a.size * b.size;

    int_t offset = 0;
    for (int_t i = 0; i < a.size; i++) {
        for (int_t j = 0; j < b.size; j++) {
            r.offsets[i * b.size + j] = offset;
            for (int_t k = a.offsets[i]; k < a.offsets[i + 1]; k++)
                r.indices[offset++] = a.indices[k];
            for (int_t k = b.offsets[j]; k < b.offsets[j + 1]; k++)
                r.indices[offset++] = b.indices[k];
        }
    }
    r.offsets[r.size] = offset;
    return r;
}

template<LogicSet ls, typename... Holders>
struct Set {
    static constexpr LogicSet LS = ls;
    std::tuple<Holders...> tuple;
};

template<int_t id, typename T>
struct Holder {
    static constexpr int_t ID = id;
    T t;
};

template<int_t id, typename T>
struct InitialHolder : Set<LogicSet { .indices = { id }, .offsets = { 0, 1 }, .size = 1 }, Holder<id, T>> {
    static constexpr int_t ID = id;
    InitialHolder(T t) {
        this->tuple = std::make_tuple(Holder<id, T> { t });
    }
};

template<int_t N, std::array<int_t, N> ids, int_t b = 2>
constexpr auto uniqueIdIndices() {
    if constexpr (b == 0) {
        return 0;
    } else {
        std::array<int_t, uniqueIdIndices<N, ids, b - 1>()> out;
        int_t offset = 0;
        for (int_t i = 0; i < N; i++) {
            int_t j = 0;
            for (; j < i; j++) {
                if (ids[j] == ids[i])
                    break;
            }
            if (i == j) {
                if constexpr (b == 2)
                    out[offset] = i;
                offset++;
            }
        }
        if constexpr (b == 2)
            return out;
        else
            return offset;
    }
}

template<int_t n, std::array<int_t, n> indices, typename... Holders, size_t... I>
constexpr auto deduplicate_helper(std::tuple<Holders...> tuple, std::index_sequence<I...>) {
    return std::make_tuple(std::get<indices[I]>(tuple)...);
}

template<typename... Types, int_t... ids>
auto deduplicate(std::tuple<Holder<ids, Types>...> tuple) {
    constexpr std::array<int_t, sizeof...(ids)> idList { ids... };
    constexpr auto indices = uniqueIdIndices<sizeof...(ids), idList>();
    constexpr int_t n = std::tuple_size<decltype(indices)>::value;
    return deduplicate_helper<n, indices>(tuple, std::make_index_sequence<n>());
}

template<LogicSet ls, typename... Holders>
auto makeSet(std::tuple<Holders...> tuple) {
    return Set<ls, Holders...> { tuple };
}

template<LogicSet ls1, typename... Holders1, LogicSet ls2, typename... Holders2>
auto operator||(const Set<ls1, Holders1...>& s1, const Set<ls2, Holders2...>& s2) {
    constexpr LogicSet ls = ls1 || ls2;
    auto tuple = deduplicate(std::tuple_cat(s1.tuple, s2.tuple));
    return makeSet<ls>(tuple);
}

template<LogicSet ls1, typename... Holders1, LogicSet ls2, typename... Holders2>
auto operator&&(const Set<ls1, Holders1...>& s1, const Set<ls2, Holders2...>& s2) {
    constexpr LogicSet ls = ls1 && ls2;
    auto tuple = deduplicate(std::tuple_cat(s1.tuple, s2.tuple));
    return makeSet<ls>(tuple);
}

}
