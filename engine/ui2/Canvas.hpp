#pragma once

#include "FixedPoint.hpp"
#include "Renderer.hpp"

namespace ui {

class Canvas {
private:
    ui::ImageView m_image;

public:
    Canvas(ui::ImageView image)
        : m_image(image) { }
    ui::ImageView image() const { return m_image; }
    template<typename Callback>
    void forEachLine(Callback callback) {
        uint16_t w = stx::alignmentCeil(16, image().dimensions().x());
        uint16_t h = image().dimensions().y();
        VDUMP(w);
        VDUMP(h);
        for (uint16_t y = 0; y < h; y++) {
            for (uint16_t x = 0; x < w; x += 16) {
                callback(&image()[{ x, y }], U16Line::indices() + x, y);
            }
        }
    }
    void rect(Color color) {
        RGBALine full { color };
        forEachLine([&](uint32_t* p, U16Line x, uint16_t) {
            choose(x < image().dimensions().x(), full, {}).store(p);
        });
    }
};

}