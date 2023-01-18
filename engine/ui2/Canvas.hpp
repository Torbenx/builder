#pragma once

#include "FixedPoint.hpp"
#include "Renderer.hpp"

namespace ui {

class Canvas {
private:
    ImageView m_image;
    using u16 = uint16_t;

    void output(u16 x, u16 y, Register<Color> src) {
        uint32_t* pos = &image()[{ x, y }];
        Register<Color> dst = Register<Color>::load(pos);
        blendStraightToPremultiplied(src, dst).store(pos);
    }

public:
    Canvas(ImageView image)
        : m_image(image) { }
    ImageView image() const { return m_image; }

    void rect(Color color) {
        Register<Color> full { color };
        auto dim = image().dimensions();
        u16 maxX = dim.x() - 1;
        u16 maxY = dim.y() - 1;
        float radius = 10.0f;
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

    void rectAA(Color colorIn, float radius) {
        Register<Color> color { colorIn };
        auto dim = image().dimensions();
        u16 maxX = dim.x() - 1;
        u16 maxY = dim.y() - 1;
        // (x - r)^2 + (y - r)^2 <= r^2
        // x >= r - sqrt(r^2 - (y - r)^2)
        struct Range {
            u16 start;
            u16 stop;
        };

        float scaledRadius = 2.0f * radius;
        u16 scaledMaxX = 2 * maxX;

        auto computeStartStop = [scaledRadius, scaledMaxX](float yDistFromBorder) -> Range {
            float diff = yDistFromBorder + 0.5f - scaledRadius;
            float sqrt = std::sqrt(scaledRadius * scaledRadius - diff * diff);
            Range r {
                (u16)(scaledRadius + 0.5f - sqrt),
                (u16)(scaledMaxX - scaledRadius + 0.5f + sqrt),
            };
            if (yDistFromBorder > scaledRadius)
                r = { 0, scaledMaxX };
            return r;
        };

        for (u16 y = 0; y < (u16)dim.y(); y++) {
            auto bot1 = computeStartStop(y * 2);
            auto bot2 = computeStartStop(y * 2 + 1);
            auto top1 = computeStartStop((maxY - y) * 2);
            auto top2 = computeStartStop((maxY - y) * 2 + 1);
            Range r1 { std::max(bot1.start, top1.start), std::min(bot1.stop, top1.stop) };
            Range r2 { std::max(bot2.start, top2.start), std::min(bot2.stop, top2.stop) };

            for (u16 x0 = 0; x0 <= maxX; x0 += 16) {
                auto x = Register<u16>::indices() + x0;

                Register<u16> count = 0;
                count += Register<u16> { 255 } & (x * 2 >= r1.start) & (x * 2 <= r1.stop + 1);
                count += Register<u16> { 255 } & (x * 2 + 1 >= r1.start) & (x * 2 <= r1.stop);
                count += Register<u16> { 255 } & (x * 2 >= r2.start) & (x * 2 <= r2.stop + 1);
                count += Register<u16> { 255 } & (x * 2 + 1 >= r2.start) & (x * 2 <= r2.stop);
                count >>= 2;
                Register<FixedPoint> alpha { count.simd };
                output(x0, y, { color.opaquePart(), color.a * alpha });
            }
        }
    }
};

}