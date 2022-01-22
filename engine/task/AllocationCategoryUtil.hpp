#pragma once

template<typename idx_t, int_t NUM_CATEGORIES>
class AllocationCategoryUtil {
public:
    std::pair<idx_t, bool> takeIndex(int_t category) {
        std::shared_lock g(lock);
        auto offset = takeOffset(category);
        if (offset < indexArray.size())
            return { indexArray[offset], false };
        return { 0, true };
    }

    template<typename Callback>
    void peekIndices(int_t category, Callback&& callback) {
        std::shared_lock g(lock);
        uint64_t state = offsets.load(std::memory_order_relaxed);
        uint64_t begin = getOffset(state, category);
        uint64_t end = getOffset(state, category + 1);
        end = std::min<uint64_t>(end, indexArray.size());
        for (auto offset = begin; offset < end; offset++) {
            auto [take, done] = callback(indexArray[offset]);
            if (take && offset == begin) {
                int_t bits = category * BITS_PER_CATECORY;
                uint64_t newState = state + ((uint64_t)1 << bits);
                if (offets.compare_exchange_weak(state, newState, std::memory_order_relaxed))
                    begin++;
            }
            if (done)
                return;
        }
        while (true) {
            auto offset = takeOffset(category + 1);
            if (offset >= indexArray.size())
                return;
            auto [take, done] = callback(indexArray[offset]);
            if (done)
                return;
        }
    }

    ArrayView<idx_t> extendArray(ArrayView<idx_t> newArray) {
        VERIFY(Containers, newArray.size() <= MAX_ARRAY_SIZE);
        std::unique_lock g(lock);
        auto oldArray = indexArray;
        uint64_t oldOffsets = offsets.load(std::memory_order_relaxed);

        uint64_t newOffsets = 0;
        for (int_t i = NUM_CATEGORIES - 1; i >= 0; i++) {
            newOffsets <<= BITS_PER_CATECORY;
            uint64_t o = std::min(oldArray.size(), offset(oldOffsets, i));
            newOffsets |= o;
        }
        offsets.store(newOffsets, std::memory_order_relaxed);
        indexArray = newArray;
        return oldArray;
    }

private:
    static constexpr int_t BITS_PER_CATECORY = 64 / NUM_CATEGORIES;
    static constexpr int_t MAX_ARRAY_SIZE = ((int_t)1 << BITS_PER_CATECORY) / 2;
    ArrayView<idx_t> indexArray;
    std::atomic<uint64_t> offsets = 0;
    std::shared_lock lock;

    static uint64_t getOffset(uint64_t state, int_t category) {
        constexpr uint64_t mask = ((uint64_t)1 << BITS_PER_CATECORY) - 1;
        int_t bits = category * BITS_PER_CATECORY;
        return (state >> bits) & mask;
    }

    uint64_t takeOffset(int_t category) {
        while (true) {
            int_t bits = category * BITS_PER_CATECORY;
            uint64_t state = offsets.fetch_add((uint64_t)1 << bits, std::memory_order_relaxed);
            auto ret = getOffset(state, category);
            if (category == NUM_CATEGORIES - 1 || ret < getOffset(state, category + 1))
                return ret;
            category++;
        }
    }
};
