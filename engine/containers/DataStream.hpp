#pragma once

#include "containers/ArrayView.hpp"
#include "util/types.hpp"
#include <vector>

template<typename T>
concept StreamCopyable = std::is_trivially_copyable_v<T>;

class DataStream {
public:
    template<StreamCopyable T>
    void writeN(T* t, int_t n) {
        std::memcpy(growBy(n * sizeof(T)), t, n * sizeof(T));
    }

    template<StreamCopyable T>
    void write(T& t) { writeN<T>(&t, 1); }

    template<StreamCopyable T>
    void writeN(ArrayView<T> array) { writeN<T>(array.data(), array.size()); }

    void clear() { m_data.clear(); }

    class Reader {
    public:
        Reader(const DataStream& stream, int_t offset = 0)
            : stream(stream), offset(offset) { }

        template<StreamCopyable T>
        void readN(T* t, int_t n) {
            std::memcpy(t, stream.m_data.data() + offset, n * sizeof(T));
            offset += n * sizeof(T);
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
        m_data.resize(m_data.size() + size);
        return m_data.data() + (int_t)m_data.size() - size;
    }

    std::vector<std::byte> m_data;
};
