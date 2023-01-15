#include "graphics/shadergen/compute.hpp"

#include "AsyncInstance.hpp"
#include "event/Awaitable.hpp"
#include "math/sparse/LUDecomposition.hpp"
#include "util/log.hpp"
#include <immintrin.h>

namespace ui {

Instance::Instance(const evt::Source<evt::WindowStateUpdate>& stateUpdate, const evt::Source<evt::ResizeEvent>& resizeEvent, AtlasAllocator* atlasAlloc)
    : windowState(stateUpdate)
    , atlasAlloc(atlasAlloc)
    , rendererUpdates(RendererBase::recordCount())
    , incrementalRenderOutput(RendererBase::recordCount())
    , styleConsumerUpdates(StyleConsumerBase::recordCount()) {

    windowRect = newRect();

    resizeEventScript = [](Instance* instance, const evt::Source<evt::ResizeEvent>& resizeEvent) -> evt::Script {
        co_await evt::onTimeline(instance);
        IntVec initialDim = resizeEvent.currentDimensions;
        log("initialDim: {} x {}", initialDim.x(), initialDim.y());
        if (initialDim.x() > 0 && initialDim.y() > 0)
            instance->resize();
        while (true) {
            await { resizeEvent };
            log("resize!");
            instance->windowDimensions = resizeEvent.currentDimensions;
            instance->resize();
        }
    }(this, resizeEvent);

    auto fix = [&](VarHandle var) -> EquationHandle {
        EquationHandle eq = newEquation();
        setEquation(eq, { { var.id, 1.0f } });
        return eq;
    };
    windowEquations[0] = fix(windowRect.left());
    windowEquations[1] = fix(windowRect.right());
    windowEquations[2] = fix(windowRect.bottom());
    windowEquations[3] = fix(windowRect.top());
}

Instance::~Instance() {
    for (int_t i = 0; i < 4; i++)
        removeEqn(windowEquations[i]);
    deleteVariable(windowRect);
    atlasInUse.destroy();
    afterAtlasUpdateChain.destroy();
}

uint32_t Instance::allocateVariableIds(int_t count) {
    uint32_t id = nextVarId;
    nextVarId += count;
    varValues.resize(nextVarId);
    return id;
}

VarHandle Instance::newVar() { return { allocateVariableIds(1) }; }
RangeVarHandle Instance::newRange() { return { allocateVariableIds(2) }; }
RectVarHandle Instance::newRect() { return { allocateVariableIds(4) }; }

EquationHandle Instance::newEquation() {
    EquationHandle eq = { eqnIdAlloc.allocateId() };
    if (inhomTyped.size() <= eq.id)
        inhomTyped.resize(eq.id + 1);
    if (inhomValues.size() <= eq.id)
        inhomValues.resize(eq.id + 1);
    if (eq.id == matrix.size())
        matrix.appendColumn({});
    return eq;
}

void Instance::setEquation(EquationHandle eq, ArrayView<const sparse::Entry> terms) {
    sparse::Vector vec = sparse::allocate(terms.size());
    std::copy(terms.begin(), terms.end(), vec.begin());
    vec.setSize(terms.size());
    vec.sortAndDedup();
    matrix[eq.id] = std::move(vec);
}

void Instance::updateInhom(EquationHandle eq, Size inhom) {
    inhomTyped[eq.id] = inhom;
}

void Instance::removeEqn(EquationHandle eqn) {
    matrix[eqn.id].resizeTo(0);
    eqnIdAlloc.freeId(eqn.id);
}
void Instance::deleteVariable(VarHandle) {
    VERIFY_NOT_REACHED(Ui);
}
void Instance::deleteVariable(RangeVarHandle) {
    VERIFY_NOT_REACHED(Ui);
}
void Instance::deleteVariable(RectVarHandle) {
    VERIFY_NOT_REACHED(Ui);
}

void Instance::dumpVarValues() {
    for (int_t i = 0; i < (int_t)varValues.size(); i++) {
        log<Tag::Ui>("{} -> {}", i, varValues[i]);
    }
}
void Instance::dumpShiftValues() {
    for (int_t i = 0; i < (int_t)inhomValues.size(); i++) {
        log<Tag::Ui>("{} -> {}", i, inhomValues[i]);
    }
}
void Instance::dumpShiftSizes() {
    for (int_t i = 0; i < (int_t)inhomTyped.size(); i++) {
        log<Tag::Ui>("{} -> {}{}{}", i, inhomTyped[i].value(), inhomTyped[i].unitBit() ? "pt" : "px", inhomTyped[i].payloadBit() ? " +1px" : "");
    }
}

struct Instance::AtlasJob {
    Instance* instance = nullptr;
    IntVec atlasDimensions;
    stx::DynArray<std::vector<std::byte>> atlasUpdates;
    sw::Semaphore signal;

    void updateAtlas() {
        if (atlasDimensions != instance->atlas.dimensions())
            instance->atlas = ui::ImageView::allocate(atlasDimensions);
        ImageView atlas = instance->atlas;

        for (int_t i = 0; i < atlasUpdates.size(); i++) {
            auto& updates = atlasUpdates[i];
            const ui::RendererRecord& record = ui::RendererBase::record(i);
            int_t stride = ui::RenderOutputUtil::outputSize(i);
            for (auto it = updates.begin(); it != updates.end(); it += stride) {
                ui::RenderOutputUtil output(&*it);
                ui::IntRect paddedRect {
                    ui::BottomLeft(output.atlasRect().bottomLeft() - 16),
                    ui::TopRight(output.atlasRect().topRight() + 16),
                };
                ImageView texture { atlas + output.atlasRect().bottomLeft(), output.atlasRect().dimensions() };
                ImageView paddedTexture { atlas + paddedRect.bottomLeft(), paddedRect.dimensions() };
                for (int j = 0; j < paddedTexture.dimensions().y(); j++) {
                    for (int i = 0; i < paddedTexture.dimensions().x(); i++) {
                        paddedTexture[{ i, j }] = 0;
                    }
                }
                record.render(texture, output.renderDataPtr());
            }
        }

        signal.signal();
        delete this;
    }
};

struct Instance::SWPainter::Job : Instance::PainterJob {
    ImageView framebuffer;
    sw::Semaphore signal;

    void paint() {
        auto multiply = [](uint32_t in, uint32_t alpha) {
            return (((in * alpha / 255) & 0xff0000) | (((in & 0xff00) * alpha / 255) & 0xff00) | ((in & 0xff) * alpha / 255));
        };

        struct UnpackedLine {
            __m256i b, g, r, a;
        };

        // { 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 }
        const __m128i halfShuffle1 = _mm_set_epi8(15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0);
        const __m256i shuffle1 = _mm256_set_m128i(halfShuffle1, halfShuffle1);
        // { 0, 4, 1, 5, 2, 6, 3, 7 }
        const __m256i shuffle2 = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
        auto unpack = [&](uint32_t* line) -> UnpackedLine {
            // input: { b, g, r, a }
            __m256i i1 = _mm256_loadu_si256((__m256i*)line);
            __m256i i2 = _mm256_loadu_si256((__m256i*)line + 1);
            i1 = _mm256_shuffle_epi8(i1, shuffle1);
            i2 = _mm256_shuffle_epi8(i2, shuffle1);
            i1 = _mm256_permutevar8x32_epi32(i1, shuffle2);
            i2 = _mm256_permutevar8x32_epi32(i2, shuffle2);
            __m256i br = _mm256_unpacklo_epi64(i1, i2);
            __m256i ga = _mm256_unpackhi_epi64(i1, i2);
            UnpackedLine out;
            out.b = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(br, 0));
            out.r = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(br, 1));
            out.g = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(ga, 0));
            out.a = _mm256_cvtepu8_epi16(_mm256_extracti128_si256(ga, 1));
            return out;
        };

        auto pack = [&](uint32_t* out, UnpackedLine l) {
            __m256i bg = _mm256_or_si256(l.b, _mm256_slli_epi16(l.g, 8));
            __m256i ra = _mm256_or_si256(l.r, _mm256_slli_epi16(l.a, 8));
            __m256i i1 = _mm256_unpacklo_epi16(bg, ra);
            __m256i i2 = _mm256_unpackhi_epi16(bg, ra);
            _mm_storeu_si128((__m128i*)out, _mm256_extracti128_si256(i1, 0));
            _mm_storeu_si128((__m128i*)out + 1, _mm256_extracti128_si256(i2, 0));
            _mm_storeu_si128((__m128i*)out + 2, _mm256_extracti128_si256(i1, 1));
            _mm_storeu_si128((__m128i*)out + 3, _mm256_extracti128_si256(i2, 1));
        };

        /*
         *  2^(2n) / (2^n - 1) - (2^n + 1)
         *  = (2^(2n) - (2^n + 1)(2^n - 1)) / (2^n - 1)
         *  = 1 / (2^n - 1)
         *
         *  d = (x + 2^(n-1)) * (2^n + 1) / 2^(2n) - (x *  / (2^n - 1) + 1/2)
         *
         *  d * 2^(2n) = (x + 2^(n-1)) * (2^n + 1) - (x * 2^(2n) / (2^n - 1) + 2^(2n - 1))
         *             = (x + 2^(n-1)) * (2^n + 1) - (x * (2^n + 1 + 1 / (2^n - 1)) + 2^(2n - 1))
         *             = x * (2^n + 1) + 2^(2n - 1) + 2^(n-1) - x * (2^n + 1) - x / (2^n - 1) - 2^(2n - 1)
         *             = 2^(n-1) - x / (2^n - 1)
         *
         *  d = (2^(n-1) - x / (2^n - 1)) / 2^(2n)
         *    = 2^(-n-1) - x / 2^(2n) / (2^n - 1)
         *
         *  x >= 0
         *  d <= 2^(-n-1)
         *
         *  x <= (2^n - 1) * 2^n
         *  d >= 1 / 2^(n+1) - 1 / 2^n = -2^(-n-1)
         */
        const __m256i multiply1 = _mm256_set1_epi16(257);
        const __m256i add1 = _mm256_set1_epi16(128);
        auto blend = [&](__m256i src, __m256i dst, __m256i srcA, __m256i oneMinusSrcA) {
            __m256i im = _mm256_add_epi16(_mm256_mullo_epi16(src, srcA), _mm256_mullo_epi16(dst, oneMinusSrcA));
            // divide by 255
            return _mm256_mulhi_epu16(_mm256_add_epi16(im, add1), multiply1);
        };

        int_t blockCount = blocks.blockOffsets.size() / (blocks.blockLayers + 1);
        for (int_t k = 0; k < blockCount; k++) {
            const IntVec* block = blocks.blockOffsets.data() + k * (blocks.blockLayers + 1);
            IntVec screenPos = block[0];
            int_t h = std::min(16, framebuffer.dimensions().y() - screenPos.y());
            for (int_t j = 0; j < h; j++) {
                /*for (int_t i = 0; i < 16; i++) {
                    uint32_t color = (framebuffer.pos() + screenPos)[j][i];
                    for (int_t layer = 0; layer < blockLayers; layer++) {
                        IntVec layerPos = block[layer + 1];
                        if (layerPos == IntVec {})
                            break;
                        uint32_t in = (atlas.pos() + layerPos)[j][i];
                        uint32_t alpha = in >> 24;
                        color = multiply(in & 0xff'ff'ff, alpha) + multiply(color, 255 - alpha);
                        // color = in & 0xff'ff'ff;
                    }
                    (framebuffer.pos() + screenPos)[j][i] = color;
                }*/
                uint32_t* buffer = (framebuffer + screenPos).row(j);
                UnpackedLine color = unpack(buffer);
                for (int_t layer = 0; layer < blocks.blockLayers; layer++) {
                    IntVec layerPos = block[layer + 1];
                    if (layerPos == IntVec {})
                        break;
                    UnpackedLine in = unpack((atlas + layerPos).row(j));
                    __m256i oneMinusA = _mm256_sub_epi16(_mm256_set1_epi16(255), in.a);
                    color.b = blend(in.b, color.b, in.a, oneMinusA);
                    color.g = blend(in.g, color.g, in.a, oneMinusA);
                    color.r = blend(in.r, color.r, in.a, oneMinusA);
                }
                pack(buffer, color);
            }
        }

        doneUsingAtlas.signal();
        signal.signal();
        delete this;
    }
};

void Instance::SWPainter::paint(sw::Semaphore waitFor, sw::Semaphore signal) {
    Job* job = new Job;
    job->framebuffer = framebuffer.get();
    job->signal = std::move(signal);
    waitFor.setAsyncCallback({ *job, stx::CallMember<&Job::paint>() });
    instance->paintEvents().push_back({
        .atlasReadyForUse = std::move(waitFor),
        .painter = job,
    });
}

void Instance::SWPainter::setFramebuffer(gfx::SWImageHandle<IMAGE_FORMAT> framebuffer) {
    this->framebuffer = framebuffer;
}

struct Instance::HWPainter::Job : Instance::PainterJob {
    sw::Semaphore signal;
    sw::Semaphore uploadLock;
    IntVec* positionsBuffer = nullptr;
    gfx::Shader<Shader> shader;

    void paint() {
        std::copy_n(blocks.blockOffsets.data(), blocks.blockOffsets.size(), positionsBuffer);
        uploadLock.signal();

        // painter->shader;

        signal.signal();
        doneUsingAtlas.signal();
        delete this;
    }
};

constexpr void Instance::HWPainter::Shader::defineShader(shadergen::ComputeDriver& prog) {
    using namespace shadergen::code;
    auto data = prog.uniformBuffer<&Shader::data>("data");
    auto positions = prog.storageBuffer<&Shader::positions>("positions", MemQual::NoWrite);
    auto screen = prog.storageImage<&Shader::screen>("screen", (MemQual)0);
    auto atlas = prog.storageImage<&Shader::atlas>("atlas", MemQual::NoWrite);

    AUTO_VARIABLE(prog, offset) = prog.globalIndex.x * (data.maxLayers + constant(1));
    AUTO_VARIABLE(prog, screenPos) = positions[offset] + prog.localIndex.xy();
    AUTO_VARIABLE(prog, color) = imageLoad(screen, screenPos).xyz();
    prog.range(constant(0), data.maxLayers, [&](auto& i) {
        AUTO_VARIABLE(prog, atlasPos) = positions[i + constant(1)] + prog.localIndex.xy();
        AUTO_VARIABLE(prog, inColor) = imageLoad(atlas, atlasPos);
        prog, color = inColor.xyz() * inColor.w + color * (constant(1.0f) - inColor.w);
    });
    prog, imageStore(screen, screenPos, compose(color, constant(1.0f)));
}

}
DEFINE_SHADER(ui::Instance::HWPainter::Shader);
namespace ui {

Instance::HWPainter::HWPainter(AsyncInstance* instance)
    : instance(instance) {
    metadataBuffer = decltype(metadataBuffer)::create();
    metadataBuffer.staticUpdate();
    *metadataBuffer.mapping() = { .maxLayers = 1 };

    positionsBuffer = decltype(positionsBuffer)::create();
    positionsBuffer.beginDynamicUpdates(10000);

    shader.mutate() = Shader {
        .data = metadataBuffer,
        .positions = positionsBuffer,
        .atlas = atlasTexture.image(),
    };
}

void Instance::HWPainter::paint(sw::Semaphore signal) {
    Job* job = new Job;
    job->signal = std::move(signal);
    job->positionsBuffer = positionsBuffer.mapping();
    job->uploadLock = gfx::allocator()->uploadLock();
    sw::Semaphore beginPaint = sw::Semaphore::create();
    beginPaint.setAsyncCallback({ *job, stx::CallMember<&Job::paint>() });
    instance->paintEvents().push_back({
        .atlasReadyForUse = std::move(beginPaint),
        .painter = job,
    });
}

void Instance::HWPainter::setFramebuffer(gfx::Texture<IMAGE_FORMAT> framebuffer) {
    shader.mutate().screen = framebuffer.image();
}

void Instance::doTick() {
    beginTick();
    executeScripts();

    for (int_t i = 0; i < styleConsumerUpdates.size(); i++) {
        auto& updates = styleConsumerUpdates[i];
        const auto& record = StyleConsumerBase::record(i);
        for (StyleConsumerBase* toUpdate : updates)
            record.updateStyle(toUpdate);
        updates.clear();
    }
    for (int_t i = 0; i < rendererUpdates.size(); i++) {
        auto& updates = rendererUpdates[i];
        const auto& record = RendererBase::record(i);
        for (uintptr_t& toUpdateUint : updates) {
            RendererBase* toUpdate = (RendererBase*)toUpdateUint;
            RenderOutputUtil output = allocateRenderOutput(renders[toUpdate->id]);
            record.updateStyleAndPrepareRender(toUpdate, output.renderDataPtr());
            toUpdateUint = (uintptr_t)toUpdate->id;
        }
    }

    updateLayout(false);

    for (int_t i = 0; i < rendererUpdates.size(); i++) {
        auto& updates = rendererUpdates[i];
        for (uintptr_t toUpdate : updates) {
            auto& render = renders[toUpdate];
            RenderOutputUtil output = findRenderOutput(render);
            IntVec dim = intRect(render.rect).dimensions();
            IntRect atlasRect { BottomLeft(render.allocPosition), Dimensions(dim) };
            output.atlasRect() = atlasRect;
        }
        updates.clear();
    }

    int_t totalUpdates = 0;
    for (auto& updates : incrementalRenderOutput)
        totalUpdates += updates.size();
    bool atlasDirty = totalUpdates > 0;
    if (paintEvents().size() > 0) {
        if (atlasDirty) {
            AtlasJob* job = new AtlasJob;
            job->atlasUpdates = std::move(incrementalRenderOutput);
            incrementalRenderOutput = stx::DynArray<std::vector<std::byte>> { RendererBase::recordCount() };
            outputGeneration += 1;
            job->instance = this;
            job->atlasDimensions = atlasDimensions;

            // run job after prev user is done using the atlas
            sw::Semaphore param[] = { std::move(atlasInUse), std::move(afterAtlasUpdateChain) };
            sw::Semaphore::merge(param).thenAsync({ *job, stx::CallMember<&AtlasJob::updateAtlas>() });
            atlasInUse = sw::Semaphore::create();
            afterAtlasUpdateChain = sw::Semaphore::create();
            job->signal = afterAtlasUpdateChain.fork();
        }
        for (auto& event : paintEvents()) {
            event.painter->doneUsingAtlas = atlasInUse.fork();
            event.painter->blocks = buildBlocks();
        }
        struct DistJob {
            std::vector<PaintEvent> events;
            Instance* instance;
            sw::Semaphore signal;
            void run() {
                for (auto& event : events) {
                    event.painter->atlas = instance->atlas;
                    event.atlasReadyForUse.signal();
                }
                signal.signal();
                delete this;
            }
        };
        sw::Semaphore nextChain = sw::Semaphore::create();
        DistJob* distJob = new DistJob { std::move(paintEvents()), this, nextChain.fork() };
        afterAtlasUpdateChain.thenSync({ *distJob, stx::CallMember<&DistJob::run>() });
        afterAtlasUpdateChain = std::move(nextChain);
    }

    endTick();
}

void Instance::resize() {
    updateInhom(windowEquations[1], windowDimensions.x() * 1_px);
    updateInhom(windowEquations[3], windowDimensions.y() * 1_px);

    IntVec windowBlocks = (windowDimensions + 15) / 16;
    blockMap.clear();
    blockMap.resize(windowBlocks.x() * windowBlocks.y());
}

void Instance::updateLayout(bool clearTextures) {
    sparse::LUDecomposition lu;
    lu.u = matrix.copy();
    int_t varCount = nextVarId;
    lu.decompose(varCount);

    Size::asPixels(inhomTyped.data(), inhomValues.data(), inhomTyped.size(), ptScale, 1.0f);

    std::vector<float> newVarValues(varCount);
    lu.inverseTranspose(inhomValues.data(), newVarValues.data());

    auto updateRenderTexture = [this](Render& render, IntRect screenRect) {
        RenderOutputUtil output;
        if (render.lastOutputGen == outputGeneration) {
            output = findRenderOutput(render);
        } else {
            output = allocateRenderOutput(render);
            render.handle.prepareRender(output.renderDataPtr());
        }
        IntRect atlasRect { BottomLeft(render.allocPosition), Dimensions(screenRect.dimensions()) };
        output.atlasRect() = atlasRect;
    };

    if (clearTextures) {
        deallocateAllTextures();
        for (int_t i = 1; i < (int_t)renders.size(); i++) {
            auto& render = renders[i];
            IntRect rect = intRect(render.rect, newVarValues.data());
            render.allocPosition = allocateTexture(rect.dimensions());
            updateRenderTexture(render, rect);
        }
    } else {
        for (int_t i = 1; i < (int_t)renders.size(); i++) {
            auto& render = renders[i];
            IntRect oldRect = intRect(render.rect, varValues.data());
            IntRect newRect = intRect(render.rect, newVarValues.data());
            if (newRect.dimensions() == oldRect.dimensions())
                continue;
            if (render.allocPosition != IntVec {}) {
                deallocateTexture({ BottomLeft(render.allocPosition), Dimensions(oldRect.dimensions()) });
                render.allocPosition = {};
            }
            render.allocPosition = allocateTexture(newRect.dimensions());
            updateRenderTexture(render, newRect);
        }
    }

    varValues = std::move(newVarValues);
}

Instance::Blocks Instance::buildBlocks() {
    std::vector<IntVec> blockOffsets;
    int_t blockLayers = 1;
    static constexpr uint32_t INVALID_BLOCK_ID = -1;
    for (uint32_t& u : blockMap)
        u = INVALID_BLOCK_ID;
    IntVec blockDimensions = (windowDimensions + 15) / 16;
    for (int_t k = 1; k < (int_t)renders.size(); k++) {
        auto& render = renders[k];
        auto rect = intRect(render.rect);
        IntRect blockRect {
            BottomLeft(rect.bottomLeft() / 16),
            TopRight(rect.topRight() / 16)
        };
        // log("blockRect: ({}, {}) - ({}, {})", blockRect.left(), blockRect.bottom(), blockRect.right(), blockRect.top());
        for (int j = 0; j < blockRect.height(); j++) {
            for (int i = 0; i < blockRect.width(); i++) {
                IntVec block = blockRect.bottomLeft() + IntVec { i, j };
                if (!IntRect(blockDimensions).contains(block))
                    continue;
                uint32_t& blockId = blockMap[block.y() * blockDimensions.x() + block.x()];
                // log("block: ({}, {}), id = {:#x}", block.x(), block.y(), blockId);
                if (blockId == INVALID_BLOCK_ID) {
                    blockId = blockOffsets.size() / (blockLayers + 1);
                    VERIFY(Ui, blockOffsets.size() % (blockLayers + 1) == 0);
                    blockOffsets.push_back(block * 16);
                    blockOffsets.resize(blockOffsets.size() + blockLayers);
                }
                uint32_t blockIndex = blockId * (blockLayers + 1);
                int_t layer = 0;
                for (; layer < blockLayers; layer++) {
                    if (blockOffsets[blockIndex + 1 + layer] == IntVec {})
                        break;
                    log("layer {} used: ({}, {})", layer, blockOffsets[blockIndex + 1 + layer].x(), blockOffsets[blockIndex + 1 + layer].y());
                }
                if (layer == blockLayers) {
                    VERIFY_NOT_REACHED(Ui);
                    continue;
                }
                blockOffsets[blockIndex + 1 + layer] = render.allocPosition - rect.bottomLeft() + block * 16;
            }
        }
    }
    return { blockOffsets, blockLayers };
}

std::byte* Instance::findRenderOutput(const Render& render) {
    VERIFY(Ui, render.lastOutputGen == outputGeneration);
    return incrementalRenderOutput[render.handle.classIndex].data() + render.outputOffsetBytes;
}
std::byte* Instance::allocateRenderOutput(Render& render) {
    int_t stride = RenderOutputUtil::outputSize(render.handle.classIndex);

    std::vector<std::byte>& vec = incrementalRenderOutput[render.handle.classIndex];
    if (render.lastOutputGen != outputGeneration) {
        render.lastOutputGen = outputGeneration;
        render.outputOffsetBytes = vec.size();
        vec.resize(vec.size() + stride);
    }
    return vec.data() + render.outputOffsetBytes;
}
void Instance::draw(RendererHandle handle, RectVarHandle rect) {
    if (handle->id == 0) {
        handle->id = renders.size();
        renders.push_back({ handle, rect });
    } else {
        renders[handle->id].rect = rect;
    }
}
void Instance::cancelDraw(RendererHandle handle) {
    VERIFY(Ui, handle->id != 0);

    Render back = renders.back();
    renders.pop_back();
    back.handle->id = handle->id;
    handle->id = 0;

    renders[back.handle->id] = back;
}

IntVec Instance::allocateTexture(IntVec dimensions) {
    VERIFY(Ui, dimensions.x() <= atlasDimensions.x() || atlasDimensions.x() == 0);
    IntVec allocPos { atlasPadding, nextAtlasAllocationHeight + atlasPadding };
    nextAtlasAllocationHeight += atlasPadding + dimensions.y();
    int_t neededHeight = nextAtlasAllocationHeight + atlasPadding;
    while (neededHeight >= atlasDimensions.y()) {
        IntVec newAtlasDimensions = atlasAlloc->nextAtlasSize(atlasDimensions);
        VERIFY(Ui, newAtlasDimensions.x() == atlasDimensions.x() || atlasDimensions.x() == 0);
        atlasDimensions = newAtlasDimensions;
    }
    return allocPos;
}
void Instance::deallocateTexture(IntRect) {
    VERIFY_NOT_REACHED(Ui);
}

void Instance::deallocateAllTextures() {
    nextAtlasAllocationHeight = 0;
}

void StyleConsumerBase::invalidateStyleInternal(int_t index) {
    instance()->styleConsumerUpdates[index].push_back(this);
}
void RendererBase::invalidateStyleInternal(int_t index) {
    instance()->rendererUpdates[index].push_back((uintptr_t)this);
}

void AsyncInstance::doTick() {
    timeline()->doTick();
}

}