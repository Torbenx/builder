#pragma once

#include "BigStyleTable.hpp"
#include "SmallStyleTable.hpp"
#include "StyleKey.hpp"
#include "handles.hpp"

namespace ui {

struct Resolver {
    Ref<BigStyleTable> table;
};
template<typename T>
T resolveLeaf(const Ref<T>& in, const Resolver& resolver, const T& base) {
    Ref<T> current = in;
    while (current.isPtr()) {
        Ptr<T> ptr = current.ptr();
        if (!ptr)
            return base;
        current = resolver.table.get(ptr);
    }
    return current.value();
}

struct Substituter {
    BigStyleTable table;
};
template<typename T>
T substituteLeaf(const Ref<T>& in, const Substituter& subs, const T& base) {
    if (!in.isPtr())
        return in.value();
    Ptr<T> ptr = in.ptr();
    if (!ptr)
        return base;
    return subs.table.get(ptr);
}

template<typename T>
T computeStyle(BigStyleTable bigTable, const Key<T>& baseStyle, const Ref<T>& localStyle) {
    return localStyle.substitute({ bigTable }, bigTable.get(baseStyle));
}

template<typename S>
class Stylable {
public:
    using Style = S;

    void setTable(BigStyleTable table) {
        bigTable = table;
    }
    void setStyle(Ptr<Style> style) {
        baseStyle = style;
    }
    Ref<Style>& local() { return localStyle; }
    const Ref<Style>& local() const { return localStyle; }
    Ptr<Style> style() const { return baseStyle; }

protected:
    const Style& computed() const { return computedStyle(); }

private:
    BigStyleTable bigTable;
    Ptr<Style> baseStyle;
    Ref<Style> localStyle;
    Style computedStyle;
};

DECLARE_STYLE_KEY(TextSize, Size);
using TextStyle = SmallStyleTable<TextSize>;

}