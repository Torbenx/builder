#pragma once

#include "FixedPoint.hpp"
#include "Renderer.hpp"

namespace ui {

class Canvas {
private:
    ui::ImageView m_image;
    using u16  = uint16_t;

public:
    Canvas(ui::ImageView image)
        : m_image(image) { }
    ui::ImageView image() const { return m_image; }

    template<typename Callback>
    void forEachRegister(Callback callback) {
        u16 w = image().dimensions().x();
        u16 h = image().dimensions().y();
        uint32_t* pos = image().data();
        int_t lineAdvance = image().stride() - image().dimensions().x();
        for (u16 y = 0; y < h; y++, pos += lineAdvance) {
            for (u16 x = 0; x < w; x += 16, pos += 16) {
                callback(pos, Register<u16>::indices() + x, y);
            }
        }
    }

    void rect(Color color) {
        Register<Color> full { color };
        auto dim = image().dimensions();
        u16 maxX = dim.x() - 1;
        u16 maxY = dim.y() - 1;
        float radius = 20;
        // (x - r)^2 + (y - r)^2 <= r^2
        // x >= r - sqrt(r^2 - (y - r)^2)

        for (u16 y = 0; y < (u16)dim.y(); y++) {
            float yBotDiff = y + 0.5f - radius;
            float botSqrt = std::sqrt(radius * radius - yBotDiff * yBotDiff);
            u16 botStartX = (u16)(radius + 0.5f - botSqrt);
            u16 botStopX = (u16)(maxX - radius + 0.5f + botSqrt);
            if (y > radius) {
                botStartX = 0;
                botStopX = maxX;
            }
            float yTopDiff = maxY - y + 0.5f - radius;
            float topSqrt = std::sqrt(radius * radius - yTopDiff * yTopDiff);
            u16 topStartX = (u16)(radius + 0.5f - topSqrt);
            u16 topStopX = (u16)(maxX - radius + 0.5f + topSqrt);
            if (maxY - y > radius) {
                topStartX = 0;
                topStopX = maxX;
            }
            u16 startX = std::max(botStartX, topStartX);
            u16 stopX = std::min(botStopX, topStopX);
            
            for (u16 x0 = 0; x0 < (u16)dim.x(); x0++) {
                auto dst = Register<Color>::load(&image()[{ x0, y }]);
                auto x = Register<u16>::indices() + x0;
                choose((x >= startX) & (x <= stopX), full, dst)
                    .store(&image()[{ x0, y }]);
            }
        }
    }
};

}