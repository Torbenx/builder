#pragma once

#include "event/Timeline.hpp"
#include "ui2/IntTypes.hpp"
#include "util/Callable.hpp"
#include <vector>

namespace Tag {
struct Graphics {
    static constexpr char tag[] = "graphics";
    static constexpr bool verifyEnabled = true;
};
}

namespace gfx {

enum class Api {
    Software,
    OpenGL,
    Vulkan,
};

struct FrameItemBase {
};
struct FrameItemRecord {
    bool (*compatible)(const FrameItemBase* oldItem, const FrameItemBase* newItem) = nullptr;
    void (*deallocate)(FrameItemBase*) = nullptr;
};
struct FrameItemHandle {
    const FrameItemRecord* record = nullptr;
    const FrameItemBase* base = nullptr;
};

struct FrameLayout {
    std::vector<FrameItemHandle> items;
};

struct InstantiatedFrameItem {
    const FrameItemRecord* record = nullptr;
    FrameItemBase* base = nullptr;
    void* impl = nullptr;
};
struct Instance {
    stx::DynArray<InstantiatedFrameItem> frameItems;
    void setFrameLayout(const FrameLayout& layout) {
    }
};

struct FrameLayoutMapping {
    const InstantiatedFrameItem* sourceItem = nullptr;
};
int_t matchLayouts(ArrayView<const InstantiatedFrameItem> oldItems, ArrayView<const FrameItemHandle> newItems, ArrayView<FrameLayoutMapping> outMappings) {
    auto tail = []<typename T>(ArrayView<T> view) -> ArrayView<T> {
        return { view.data() + 1, view.size() - 1 };
    };
    if (oldItems.empty() || newItems.empty())
        return oldItems.size() + newItems.size();
    if (oldItems[0].record == newItems[0].record) {
        const FrameItemRecord* record = oldItems[0].record;
        if (record->compatible(oldItems[0].base, newItems[0].base)) {
            outMappings[0].sourceItem = &oldItems[0];
            return matchLayouts(tail(oldItems), tail(newItems), tail(outMappings));
        }
    }
    stx::DynArray<FrameLayoutMapping> map1 { newItems.size() };
    int_t score1 = matchLayouts(tail(oldItems), newItems, map1);

    stx::DynArray<FrameLayoutMapping> map2 { newItems.size() };
    int_t score2 = matchLayouts(oldItems, tail(newItems), tail((ArrayView<FrameLayoutMapping>)map2));

    ArrayView<FrameLayoutMapping> source = score1 < score2 ? map1 : map2;
    std::copy_n(source.data(), source.size(), outMappings.data());
    return std::min(score1, score2) + 1;
}

}

// ----------------------------------------------

/*namespace gfx::sw {

inline constexpr Api API = Api::Software;

struct Resolver {
    Resolver(const Resolver&) = delete;
    std::vector<stx::BasicCallback<Resolver&>> updateQueue;
    Instance* instance = nullptr;
    Instance* operator->() const { return instance; }
};

struct BufferGeometry {
    int stride = 0;
    ui::IntVec dimensions;
};
struct BufferChain {
    uint32_t id = 0;
};

struct BufferUsageInfo {
    bool consume = false;
};

struct VisitInfo {
    int_t index = -1;
    BufferUsageInfo usage;
    BufferChain* outChain = nullptr;
    evt::Target* scheduleTarget = nullptr;
};

struct BufferProducerBase { };
struct BufferProducerRecord {
    void (*visit)(BufferProducerBase* base, VisitInfo);
};
template<typename Impl>
struct BufferProducer : BufferProducerBase {
};
struct BufferProducerHandle {
    const BufferProducerRecord* record = nullptr;
    BufferProducerBase* base = nullptr;

    void visit(VisitInfo info) const {
        record->visit(base, info);
    }
};
struct BufferPromise {
    sw::BufferProducerHandle producer;
    int_t outputIndex;

    void visit(BufferUsageInfo usage, BufferChain* outChain, evt::Target* target) {
        producer.visit({ outputIndex, usage, outChain, target });
    }
};

}

namespace gfx {

template<Api api>
auto withApi(GenericBufferProducerImplHandle handle) {
    if constexpr (api == Api::Software) {
        return sw::BufferProducerHandle { (sw::BufferProducerRecord*)handle.record, (sw::BufferProducerBase*)handle.base };
    }
}

}

#include "ui2/Instance.hpp"

namespace gfx {

namespace sw {
    struct UiPainter : BufferProducer<UiPainter> {

        Instance* instance = nullptr;
        BufferPromise inputPromise;
        BufferChain inputChain;
        std::vector<VisitInfo> usages;
        int_t lastUpdatedTick = 0;
        std::vector<evt::Target*> targets;
        evt::Target paintTarget { stx::WithContaining<&UiPainter::paintTarget, stx::CallMember<&UiPainter::paint>>() };
        stx::DynArray<ui::IntVec> blockOffsets;
        int_t blockLayers = 0;

        UiPainter(Instance* instance, BufferPromise inputPromise)
            : instance(instance), inputPromise(inputPromise) {
            paintTarget.incrementDependencyCount();
        }

        void visit(Resolver& resolver, VisitInfo info) {
            VERIFY(Graphics, info.index == 0);

            if (lastUpdatedTick != evt::tick()) {
                lastUpdatedTick = evt::tick();

                inputPromise.visit({ .consume = true }, &inputChain, &paintTarget);
                resolver.updateQueue.push_back({ *this, stx::CallMember<&UiPainter::update>() });
            }
            usages.push_back(info);
        }
        void update(Resolver& resolver) {
            VERIFY(Graphics, usages.size() == 1);
            *usages[0].outChain = inputChain;

            std::vector<evt::Target*> newTargets;
            newTargets.reserve(usages.size());
            for (const auto& use : usages) {
                use.scheduleTarget->incrementDependencyCount();
                newTargets.push_back(use.scheduleTarget);
            }
            for (evt::Target* target : targets)
                target->decrementDependencyCount({});
            targets = std::move(newTargets);

            usages.clear();
        }
        void paint() {

            ui::TextureHandle atlas = instance->buffer(atlasChain);
            ui::TextureHandle framebuffer = instance->buffer(inputChain);

            auto multiply = [](uint32_t in, uint32_t alpha) {
                return (((in * alpha / 255) & 0xff0000) | (((in & 0xff00) * alpha / 255) & 0xff00) | ((in & 0xff) * alpha / 255));
            };
            int_t blockCount = blockOffsets.size() / (blockLayers + 1);
            for (int_t k = 0; k < blockCount; k++) {
                ui::IntVec* block = blockOffsets.data() + k * (blockLayers + 1);
                ui::IntVec screenPos = block[0];
                for (int_t j = 0; j < 16; j++) {
                    for (int_t i = 0; i < 16; i++) {
                        uint32_t color = (framebuffer.view() + screenPos)[j][i];
                        for (int_t layer = 0; layer < blockLayers; layer++) {
                            ui::IntVec layerPos = block[layer + 1];
                            if (layerPos == ui::IntVec {})
                                break;
                            uint32_t in = (atlas.view() + layerPos)[j][i];
                            uint32_t alpha = in >> 24;
                            color = multiply(in & 0xff'ff'ff, alpha) + multiply(color, 255 - alpha);
                            // color = in & 0xff'ff'ff;
                        }
                        (framebuffer.view() + screenPos)[j][i] = color;
                    }
                }
            }

            for (evt::Target* target : targets)
                target->applySignal(scheduler);
        }

        BufferChain atlasChain;
        evt::Target atlasTarget { stx::WithContaining<&UiPainter::atlasTarget, stx::CallMember<&UiPainter::renderAtlas>>() };
        stx::DynArray<std::vector<std::byte>> incrementalRenderOutput;
        struct RenderHistoryItem {
            int_t chainIndex = -1;
            stx::DynArray<ui::IntRect> atlasRects;
        };
        evt::RingBuffer<RenderHistoryItem, 8> renderHistory;
        void applyRenderHistory() {
            int_t chainIndex = instance->chainIndex();
            ui::TextureHandle atlas = instance->buffer(atlasChain);

            int_t copyFromIndex = renderHistory[evt::tick() - 1].chainIndex;
            if (copyFromIndex == -1)
                return;
            ui::TextureHandle copyFrom = instance->buffer(atlasChain, copyFromIndex);

            int_t ticksSinceRender = 0;
            for (; ticksSinceRender < renderHistory.size() - 1; ticksSinceRender++) {
                if (renderHistory[evt::tick() - ticksSinceRender - 1].chainIndex == chainIndex)
                    break;
            }

            if (ticksSinceRender == renderHistory.size() - 1) {
                // no previous render, must make a full copy
                for (int_t j = 0; j < copyFrom.dimensions.y(); j++) {
                    for (int_t i = 0; j < copyFrom.dimensions.x(); i++) {
                        atlas.view()[j][i] = copyFrom.view()[j][i];
                    }
                }
                return;
            }
            // incremental copy
            for (int_t i = evt::tick() - ticksSinceRender; i > evt::tick(); i--) {
                auto& item = renderHistory[i];
                for (ui::IntRect rect : item.atlasRects) {
                    ui::TextureView in = copyFrom.view() + rect.bottomLeft();
                    ui::TextureView out = atlas.view() + rect.bottomLeft();
                    for (int_t j = 0; j < rect.height(); j++) {
                        for (int_t i = 0; i < rect.width(); i++) {
                            out[j][i] = in[j][i];
                        }
                    }
                }
            }
        }
        void renderAtlas() {
            applyRenderHistory();

            int_t chainIndex = instance->chainIndex();
            ui::TextureHandle atlas = instance->buffer(atlasChain);

            int_t totalRenders = 0;
            for (int_t i = 0; i < incrementalRenderOutput.size(); i++)
                totalRenders += incrementalRenderOutput[i].size();
            stx::DynArray<ui::IntRect> renderedRects { totalRenders };
            int_t renderedRectsOffset = 0;

            for (int_t i = 0; i < incrementalRenderOutput.size(); i++) {
                auto& outputs = incrementalRenderOutput[i];
                const ui::RendererRecord& record = ui::RendererBase::record(i);
                int_t stride = ui::RenderOutputUtil::outputSize(i);
                for (auto it = outputs.begin(); it != outputs.end(); it += stride) {
                    ui::RenderOutputUtil output(&*it);
                    ui::IntRect paddedRect {
                        ui::BottomLeft(output.atlasRect().bottomLeft() - 16),
                        ui::TopRight(output.atlasRect().topRight() + 16),
                    };
                    ui::TextureState texture { atlas.view() + output.atlasRect().bottomLeft(), output.atlasRect().dimensions() };
                    ui::TextureState paddedTexture { atlas.view() + paddedRect.bottomLeft(), paddedRect.dimensions() };
                    for (int_t j = 0; j < paddedTexture.dimensions.y(); j++) {
                        for (int_t i = 0; i < paddedTexture.dimensions.x(); i++) {
                            paddedTexture.texture[j][i] = 0;
                        }
                    }
                    record.render(texture, output.renderDataPtr());
                    renderedRects[renderedRectsOffset++] = paddedRect;
                }
            }
            VERIFY(Graphics, renderedRectsOffset == renderedRects.size());

            renderHistory[evt::tick()] = { chainIndex, std::move(renderedRects) };

            paintTarget.applySignal(scheduler);
        }

        ~UiPainter() {
            VERIFY(Graphics, targets.empty());
        }
    };
}

struct UiPainter : BufferProducer<UiPainter> {

    BufferPromise input;
    Instance* instance = nullptr;
    void* impl = nullptr;

    void* getImpl(Api api) {
        VERIFY(Graphics, api == Api::Software);
        if (!impl)
            impl = new sw::UiPainter { instance, { input.producer.getImpl<Api::Software>(), input.outputIndex } };
        return impl;
    }
};

struct Present {
    BufferPromise input;
};
}*/