#pragma once

#include "graphics/helpers.hpp"
#include "ui/renderer/ComputeShader.hpp"
#include "ui/renderer/Tile.hpp"

namespace ui_renderer {

class Renderer {
public:
    Renderer(int_t layers, int_t width, int_t height, GAllocator& allocator)
        : layers(layers), shader(layers), width(width), height(height), tileCountX((width + 255) >> 8), tileCountY((height + 255) >> 8) {
        tiles.reset(new Tile[tileCountX * tileCountY]);
        for (int_t y = 0; y < tileCountY; y++) {
            for (int_t x = 0; x < tileCountX; x++) {
                tiles[y * tileCountX + x] = std::move(Tile(layers, x, y));
            }
        }
        size_t alignment = uniformAlignment();
        tileStride = alignmentCeil(alignment, Tile::requiredBytes(layers));
        allocator.allocateDynamic(&allocation.ptr, alignment, tileCountX * tileCountY * tileStride);
    }

    void add(uint32_t id, IntRect rect) {
        rect.left = std::max((int_t)0, rect.left);
        rect.bottom = std::max((int_t)0, rect.bottom);
        rect.right = std::min(width - 1, rect.right);
        rect.top = std::min(height - 1, rect.top);
        IntRect tileRect = rect;
        tileRect.left >>= 5;
        tileRect.bottom >>= 5;
        tileRect.right >>= 5;
        tileRect.top >>= 5;
        IntRect rendererRect = rect;
        rendererRect.left >>= 8;
        rendererRect.bottom >>= 8;
        rendererRect.right >>= 8;
        rendererRect.top >>= 8;
        for (int_t y = rendererRect.bottom; y <= rendererRect.top; y++) {
            for (int_t x = rendererRect.left; x <= rendererRect.right; x++) {
                tiles[y * tileCountX + x].add(id, tileRect);
            }
        }
    }

    void update(GAllocator& allocator) {
        std::byte* mapped = (std::byte*)allocator.mapping(allocation.ptr);
        for (int_t i = 0; i < tileCountX * tileCountY; i++) {
            if (tiles[i].update()) {
                std::memcpy(mapped + i * tileStride, tiles[i].data.get(), Tile::requiredBytes(layers));
            }
        }
    }

    void draw(Texture texture, GAllocator& allocator) {
        shader.use();
        //shader.uniformTexture(shader.baseTexture, texture);
        glBindImageTexture(0, texture.glId, 0, false, 0, GL_READ_WRITE, GL_RGBA8UI);
        for (int_t i = 0; i < tileCountX * tileCountY; i++) {
            if (tiles[i].used()) {
                auto pair = allocator.buffer(allocation.ptr);
                pair.second += i * tileStride;
                shader.uniformBuffer(shader.tileUniform, { pair, Tile::requiredBytes(layers) });
                glDispatchCompute(tiles[i].usedWidth, tiles[i].usedHeight, 1);
            }
        }
    }

    void release([[context]] GAllocator* allocator) {
        allocation.release();
    }

    ~Renderer([[context]] GAllocator* allocator) { }

private:
    GAllocationHelper<> allocation;
    size_t tileStride;
    int_t layers;
    ComputeShader shader;
    int_t width = 0, height = 0;
    int_t tileCountX = 0, tileCountY = 0;
    std::unique_ptr<Tile[]> tiles;
};

}
