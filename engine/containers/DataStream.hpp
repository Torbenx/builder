#pragma once

#include "containers/ArrayView.hpp"
#include "util/types.hpp"
#include <vector>

namespace Tag {
struct DataStream {
    static constexpr char tag[] = "DStream";
    static constexpr bool verifyEnabled = true;
};
}

template<typename T>
concept StreamCopyable = std::is_trivially_copyable_v<T>;

class DataStream {
public:
    template<StreamCopyable T>
    void writeN(const T* t, int_t n) {
        if constexpr (Tag::DataStream::verifyEnabled) {
            size_t hash = typeid(T).hash_code();
            std::byte* addr = growBy(n * (sizeof(size_t) + sizeof(T)));
            for (int_t i = 0; i < n; i++) {
                *(size_t*)addr = hash;
                addr += sizeof(size_t);
                *(T*)addr = t[i];
                addr += sizeof(T);
            }
        } else {
            std::memcpy(growBy(n * sizeof(T)), t, n * sizeof(T));
        }
    }

    template<StreamCopyable T>
    void write(const T& t) { writeN<T>(&t, 1); }

    template<StreamCopyable T>
    void writeN(ArrayView<T> array) { writeN<T>(array.data(), array.size()); }

    void clear() { m_data.clear(); }

    class Reader {
    public:
        Reader(const DataStream& stream, int_t offset = 0)
            : stream(stream), offset(offset) { }

        template<StreamCopyable T>
        void readN(T* t, int_t n) {
            if constexpr (Tag::DataStream::verifyEnabled) {
                size_t hash = typeid(T).hash_code();
                const std::byte* addr = stream.m_data.data() + offset;
                for (int_t i = 0; i < n; i++) {
                    EXPECT(DataStream, *(const size_t*)addr, hash);
                    addr += sizeof(size_t);
                    t[i] = *(const T*)addr;
                    addr += sizeof(T);
                }
                offset += n * (sizeof(size_t) + sizeof(T));
            } else {
                std::memcpy(t, stream.m_data.data() + offset, n * sizeof(T));
                offset += n * sizeof(T);
            }
        }

        template<StreamCopyable T>
        T read() {
            T t;
            readN<T>(&t, 1);
            return t;
        }

    private:
        const DataStream& stream;
        int_t offset;
    };

    Reader beginRead() const { return { *this }; }

private:
    std::byte* growBy(int_t size) {
        int_t offset = m_data.size();
        m_data.resize(offset + size);
        return m_data.data() + offset;
    }

    std::vector<std::byte> m_data;
};
