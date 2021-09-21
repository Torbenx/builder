#pragma once

#include "graphics/GAllocator.hpp"
#include "util/types.hpp"
#include <memory>
#include <algorithm>

namespace ui_renderer {

struct IntRect {
    int_t left, right, bottom, top;
    int_t width() { return right - left + 1; }
    int_t height() { return top - bottom + 1; }
};

struct Tile {
    struct DataPtr {
        uint32_t* ptr;

        uint32_t& textureCount() { return ptr[0]; }
        uint32_t& operator[](int_t i) { return ptr[i + 1]; }
        void add(uint32_t id) {
            (*this)[textureCount()++] = id;
        }
        void remove(uint32_t id) {
            int_t i;
            for (i = 0; i < textureCount(); i++) {
                if ((*this)[i] == id) {
                    break;
                }
            }
            textureCount()--;
            for (; i < textureCount(); i++) {
                (*this)[i] = (*this)[i + 1];
            }
        }
    };

    struct Head {
        int32_t offsetX;
        int32_t offsetY;
        int32_t beginX;
        int32_t beginY;
    };

    static constexpr int_t headOffset = sizeof(Head) / sizeof(uint32_t);

    uint64_t mask = 0;
    int_t layers;
    std::unique_ptr<uint32_t[]> data;
    int_t offsetX, offsetY;
    int_t usedWidth, usedHeight;

    static int_t dataStride(int_t layers) { return (layers / 4 + 1) * 4; }
    static int_t requiredBytes(int_t layers) {
        return (headOffset + 64 * dataStride(layers)) * sizeof(uint32_t);
    }

    Tile() { }
    Tile(int_t layers, int_t offsetX, int_t offsetY)
        : layers(layers), data(new uint32_t[requiredBytes(layers)]()), offsetX(offsetX), offsetY(offsetY) { }

    int_t dataStride() { return dataStride(layers); }

    DataPtr at(int_t x, int_t y) {
        return atIndex(y * 8 + x);
    }
    DataPtr atIndex(int_t i) {
        return { &data[headOffset + i * dataStride()] };
    }
    Head& head() { return *(Head*)data.get(); }

    void transformRect(IntRect& rect) {
        rect.left -= offsetX * 8;
        rect.right -= offsetX * 8;
        rect.bottom -= offsetY * 8;
        rect.top -= offsetY * 8;
        rect.left = std::max((int_t)0, rect.left);
        rect.right = std::min((int_t)7, rect.right);
        rect.bottom = std::max((int_t)0, rect.bottom);
        rect.top = std::min((int_t)7, rect.top);
    }

    void add(uint32_t id, IntRect rect) {
        log("rect: {}, {}, {}, {}", rect.bottom, rect.left, rect.top, rect.right);
        log("translate by {}, {}", offsetX, offsetY);
        transformRect(rect);
        log("rect transformed: {}, {}, {}, {}", rect.bottom, rect.left, rect.top, rect.right);
        for (int_t y = rect.bottom; y <= rect.top; y++) {
            for (int_t x = rect.left; x <= rect.right; x++) {
                at(x, y).add(id);
                mask |= (uint64_t)1 << (y * 8 + x);
            }
        }
    }

    void remove(uint32_t id, IntRect oldRect) {
        transformRect(oldRect);
        for (int_t y = oldRect.bottom; y <= oldRect.top; y++) {
            for (int_t x = oldRect.left; x <= oldRect.right; x++) {
                DataPtr d = at(x, y);
                d.remove(id);
                if (d.textureCount() == 0)
                    mask &= ~((uint64_t)1 << (y * 8 + x));
            }
        }
    }

    bool used() {
        return mask != 0;
    }

    IntRect usedRect() {
        int_t bottomLeft = std::countr_zero(mask);
        int_t topRight = (int_t)63 - std::countl_zero(mask);
        return { bottomLeft & 7, topRight & 7, bottomLeft >> 3, topRight >> 3 };
    }

    bool update() {
        if (!used()) {
            return false;
        }
        IntRect rect = usedRect();
        head().beginX = rect.left;
        head().beginY = rect.bottom;
        head().offsetX = offsetX * 256 + rect.left * 32;
        head().offsetY = offsetY * 256 + rect.bottom * 32;
        usedWidth = rect.width();
        usedHeight = rect.height();
        return true;
    }

    void clear() {
        mask = 0;
        for (int_t i = 0; i < 64; i++) {
            atIndex(i).textureCount() = 0;
        }
    }
};
}