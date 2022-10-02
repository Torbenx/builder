#pragma once

#include "BigStyleTable.hpp"
#include "render.hpp"
#include "StyleKey.hpp"
#include "Rect.hpp"

namespace ui {

struct ZIndex;

template<typename Impl>
struct ZIndexHelper {
private:
    Impl* impl() { return static_cast<Impl*>(this); }
    ZIndex& front() { return impl()->front(); }
    ZIndex& back() { return impl()->back(); }
    ZIndex*& prev() { return front().prev; }
    ZIndex*& next() { return back().next; }

public:
    void detach() {
        if (prev())
            prev()->next = next();
        if (next())
            next()->prev = prev();
        prev() = nullptr;
        next() = nullptr;
    }
    void attachBefore(ZIndex& before) {
        detach();
        next() = &before;
        prev() = next()->prev;
        next()->prev = &back();
        if (prev())
            prev()->next = &front();
    }
    void attachAfter(ZIndex& after) {
        detach();
        prev() = &after;
        next() = prev()->next;
        prev()->next = &front();
        if (next())
            next()->prev = &back();
    }

    ~ZIndexHelper() { detach(); }
};

struct ZIndex : ZIndexHelper<ZIndex> {
    ZIndex* prev = nullptr;
    ZIndex* next = nullptr;

    ZIndex() = default;
    ZIndex(const ZIndex&) = delete;

    ZIndex& front() { return *this; }
    ZIndex& back() { return *this; }
};

struct ZRange : ZIndexHelper<ZRange> {
    ZIndex begin;
    ZIndex end;

    ZRange() { end.attachAfter(begin); }

    ZIndex& front() { return begin; }
    ZIndex& back() { return end; }

    void prepend(ZIndex& index) { index.attachAfter(begin); }
    void append(ZIndex& index) { index.attachBefore(end); }
    void prepend(ZRange& range) { range.attachAfter(begin); }
    void append(ZRange& range) { range.attachBefore(end); }
};

}