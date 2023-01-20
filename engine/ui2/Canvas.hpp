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
        // x >= r - sqrt(r^2 - (y - r)^2) = r - sqrt(2*y*r - y^2) = r - sqrt(y * (2*r - y))
        struct Range {
            u16 start;
            u16 stop;
        };

        auto computeStartStop = [radius, maxX](float yDistFromBorder) -> std::pair<Range, Range> {
            float diff = radius - (yDistFromBorder + 0.5f);
            float sqrt = std::sqrt(radius * radius - diff * diff);
            Range rM {
                (u16)(radius - sqrt + 0.5f - 0.25f),
                (u16)(maxX - radius + sqrt + 0.5f - 0.25f),
            };
            Range rP {
                (u16)(radius - sqrt + 0.5f + 0.25f),
                (u16)(maxX - radius + sqrt + 0.5f + 0.25f),
            };
            if (yDistFromBorder + 0.5f >= radius) {
                rM = { 0, maxX };
                rP = { 0, maxX };
            }
            return { rM, rP };
        };

        for (u16 y = 0; y < (u16)dim.y(); y++) {
            auto [botMM, botPM] = computeStartStop(y - 0.25f);
            auto [botMP, botPP] = computeStartStop(y + 0.25f);
            auto [topMM, topPM] = computeStartStop((maxY - y) - 0.25f);
            auto [topMP, topPP] = computeStartStop((maxY - y) + 0.25f);
            Range rMM { std::max(botMM.start, topMM.start), std::min(botMM.stop, topMM.stop) };
            Range rPM { std::max(botPM.start, topPM.start), std::min(botPM.stop, topPM.stop) };
            Range rMP { std::max(botMP.start, topMP.start), std::min(botMP.stop, topMP.stop) };
            Range rPP { std::max(botPP.start, topPP.start), std::min(botPP.stop, topPP.stop) };

            for (u16 x0 = 0; x0 <= maxX; x0 += 16) {
                auto x = Register<u16>::indices() + x0;

                Register<u16> count = 0;
                count += Register<u16> { 255 } & (x >= rMM.start) & (x <= rMM.stop);
                count += Register<u16> { 255 } & (x >= rPM.start) & (x <= rPM.stop);
                count += Register<u16> { 255 } & (x >= rMP.start) & (x <= rMP.stop);
                count += Register<u16> { 255 } & (x >= rPP.start) & (x <= rPP.stop);
                count = (count + 2) >> 2;

                auto alpha = Register<FixedPoint>::fromRaw(count);
                output(x0, y, { color.opaquePart(), color.a * alpha });
            }
        }
    }
};

}