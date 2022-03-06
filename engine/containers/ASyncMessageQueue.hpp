#pragma once

template<typename T>
class ASyncMessageQueue {
    struct Header : TrailingArray<Header, T> {
        int_t size = 0;
        Header* nextBlock = nullptr;
    };
    struct Handle {
        ASyncMessageQueue* queue = nullptr;
        int_t capacity = 0;
        Header* block = nullptr;

        void pushBack(const T& item) {
            if (block->size < capacity) {
                std::construct_at(block->array() + block->size, item);
                block->size += 1;
                return;
            }
        }
    };
    Header* block = nullptr;

    ArrayView<T> take() {
        ArrayView<T> ret { block->array(), block->size };
        block = block->nextBlock;
        return ret;
    }
};
