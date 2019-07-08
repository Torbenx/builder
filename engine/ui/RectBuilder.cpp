#include "RectBuilder.hpp"
#include <math.h>
#include "../util/log.hpp"

#define PROP(s,b) (s##F == 0 ? NAN : s##F(b, s##D))
#define WIDTH(b) PROP(width,b)
#define HEIGHT(b) PROP(height,b)
#define TOP(b) PROP(top,b)
#define BOTTOM(b) PROP(bottom,b)
#define LEFT(b) PROP(left,b)
#define RIGHT(b) PROP(right,b)

#define RECT_BUILDER_DEBUG 0

RectY recty_deref(RectY r, ScreenMetrics m, bool forHeight=false){
    RectBuilder *b = (RectBuilder*)r.rect;
    if(b == nullptr)return r;
    if(RECT_BUILDER_DEBUG)LOGD("deref %s.%s",b->name.c_str(),r.baseBottom?"b":"t");
    if(r.baseBottom){
        if(b->bottomF != 0){
            if(forHeight && !b->useBottomForHeight)return r;
            RectY r2 = recty_deref(b->bottomF(b->bottomD, m), m, forHeight);
            r2.dy = TypedSize::add(r.dy, r2.dy, TYPED_SIZE_GL, m.y);
            return r2;
        }
        return r;
    }else{
        if(b->topF != 0){
            if(forHeight && !b->useTopForHeight)return r;
            RectY r2 = recty_deref(b->topF(b->topD, m), m, forHeight);
            r2.dy = TypedSize::add(r.dy, r2.dy, TYPED_SIZE_GL, m.y);
            return r2;
        }
        return r;
    }
}

RectX rectx_deref(RectX r, ScreenMetrics m, bool forWidth=false){
    RectBuilder *b = (RectBuilder*)r.rect;
    if(b == nullptr)return r;
    if(RECT_BUILDER_DEBUG)LOGD("deref %s.%s",b->name.c_str(),r.baseLeft?"l":"r");
    if(r.baseLeft){
        if(b->leftF != 0){
            if(forWidth && !b->useLeftForWidth)return r;
            RectX r2 = rectx_deref(b->leftF(b->leftD, m), m, forWidth);
            r2.dx = TypedSize::add(r.dx, r2.dx, TYPED_SIZE_GL, m.x);
            return r2;
        }
        return r;
    }else{
        if(b->rightF != 0){
            if(forWidth && !b->useRightForWidth)return r;
            RectX r2 = rectx_deref(b->rightF(b->rightD, m), m, forWidth);
            r2.dx = TypedSize::add(r.dx, r2.dx, TYPED_SIZE_GL, m.x);
            return r2;
        }
        return r;
    }
}

float RectBuilder::getWidth(ScreenMetrics m){
    if(RECT_BUILDER_DEBUG)LOGD("%s.getWidth",name.c_str());
    RectX left,right;
    left.rect = this;
    left.baseLeft = true;
    left.dx = TypedSize::zero;
    left = rectx_deref(left, m, true);
    right.rect = this;
    right.baseLeft = false;
    right.dx = TypedSize::zero;
    right = rectx_deref(right, m, true);
    if(left.rect == right.rect && left.baseLeft == right.baseLeft){
        return right.dx.toGL(m.x) - left.dx.toGL(m.x);
    }
    LOGW("%s.getWidth failed",name.c_str());
    return NAN;
}

float RectBuilder::getHeight(ScreenMetrics m){
    if(RECT_BUILDER_DEBUG)LOGD("%s.getHeight",name.c_str());
    RectY top,bottom;
    top.rect = this;
    top.baseBottom = false;
    top.dy = TypedSize::zero;
    top = recty_deref(top, m, true);
    bottom.rect = this;
    bottom.baseBottom = true;
    bottom.dy = TypedSize::zero;
    bottom = recty_deref(bottom, m, true);
    if(top.rect == bottom.rect && top.baseBottom == bottom.baseBottom){
        return top.dy.toGL(m.y) - bottom.dy.toGL(m.y);
    }
    LOGW("%s.getHeight failed",name.c_str());
    return NAN;
}

float RectBuilder::getTop(ScreenMetrics m){
    if(RECT_BUILDER_DEBUG)LOGD("%s.getTop",name.c_str());
    RectY top;
    top.rect = this;
    top.baseBottom = false;
    top.dy = TypedSize::zero;
    top = recty_deref(top, m);
    if(top.rect == nullptr){
        if(top.baseBottom){
            return top.dy.toGL(m.y) + parent.getBottom();
        }else{
            return top.dy.toGL(m.y) + parent.getTop();
        }
    }
    LOGW("%s.getTop failed",name.c_str());
    return NAN;
}

float RectBuilder::getBottom(ScreenMetrics m){
    if(RECT_BUILDER_DEBUG)LOGD("%s.getBottom",name.c_str());
    RectY bottom;
    bottom.rect = this;
    bottom.baseBottom = true;
    bottom.dy = TypedSize::zero;
    bottom = recty_deref(bottom, m);
    if(bottom.rect == nullptr){
        if(bottom.baseBottom){
            return bottom.dy.toGL(m.y) + parent.getBottom();
        }else{
            return bottom.dy.toGL(m.y) + parent.getTop();
        }
    }
    LOGW("%s.getBottom failed",name.c_str());
    return NAN;
}

float RectBuilder::getLeft(ScreenMetrics m){
    if(RECT_BUILDER_DEBUG)LOGD("%s.getLeft",name.c_str());
    RectX left;
    left.rect = this;
    left.baseLeft = true;
    left.dx = TypedSize::zero;
    left = rectx_deref(left, m);
    if(left.rect == nullptr){
        if(left.baseLeft){
            return left.dx.toGL(m.x) + parent.getLeft();
        }else{
            return left.dx.toGL(m.x) + parent.getRight();
        }
    }
    LOGW("%s.getLeft failed",name.c_str());
    return NAN;
}

float RectBuilder::getRight(ScreenMetrics m){
    if(RECT_BUILDER_DEBUG)LOGD("%s.getRight",name.c_str());
    RectX right;
    right.rect = this;
    right.baseLeft = false;
    right.dx = TypedSize::zero;
    right = rectx_deref(right, m);
    if(right.rect == nullptr){
        if(right.baseLeft){
            return right.dx.toGL(m.x) + parent.getLeft();
        }else{
            return right.dx.toGL(m.x) + parent.getRight();
        }
    }
    LOGW("%s.getRight failed", name.c_str());
    return NAN;
}

Rect RectBuilder::build(ScreenMetrics m){
    return Rect::bltr(getBottom(m), getLeft(m), getTop(m), getRight(m));
}


RectY rb_rely(RectBuilderData data, ScreenMetrics m){
    return data.rely;
}
RectX rb_relx(RectBuilderData data, ScreenMetrics m){
    return data.relx;
}

void RectBuilder::width(TypedSize width){
    if(rightF == 0 || leftF != 0){
        rightF = rb_relx;
        rightD.relx.rect = this;
        rightD.relx.baseLeft = true;
        rightD.relx.dx = width;
        useRightForWidth = true;
    }
    if(leftF == 0){
        leftF = rb_relx;
        leftD.relx.rect = this;
        leftD.relx.baseLeft = false;
        leftD.relx.dx = -width;
        useLeftForWidth = true;
    }
}

void RectBuilder::height(TypedSize height){
    if(bottomF == 0 || topF != 0){
        bottomF = rb_rely;
        bottomD.rely.rect = this;
        bottomD.rely.baseBottom = false;
        bottomD.rely.dy = -height;
        useBottomForHeight = true;
    }
    if(topF == 0){
        topF = rb_rely;
        topD.rely.rect = this;
        topD.rely.baseBottom = true;
        topD.rely.dy = height;
        useTopForHeight = true;
    }
}

RectX rb_lazyx(RectBuilderData data, ScreenMetrics m){
    RectX r;
    r.rect = data.lazyx.rect;
    r.baseLeft = data.lazyx.baseLeft;
    r.dx = TypedSize::add(data.lazyx.dx.get(), data.lazyx.padding, TYPED_SIZE_GL, m.x);
    if(!r.baseLeft)r.dx = -r.dx;
    return r;
}

RectY rb_lazyy(RectBuilderData data, ScreenMetrics m){
    RectY r;
    r.rect = data.lazyy.rect;
    r.baseBottom = data.lazyy.baseBottom;
    r.dy = TypedSize::add(data.lazyy.dy.get(), data.lazyy.padding, TYPED_SIZE_GL, m.y);
    if(!r.baseBottom)r.dy = -r.dy;
    return r;
}

void RectBuilder::width(TypedSizeGetter width, TypedSize padding){
    if(rightF == 0 || leftF != 0){
        rightF = rb_lazyx;
        rightD.lazyx.rect = this;
        rightD.lazyx.baseLeft = true;
        rightD.lazyx.dx = width;
        rightD.lazyx.padding = padding;
        useRightForWidth = true;
    }
    if(leftF == 0){
        leftF = rb_lazyx;
        leftD.lazyx.rect = this;
        leftD.lazyx.baseLeft = false;
        leftD.lazyx.dx = width;
        leftD.lazyx.padding = padding;
        useLeftForWidth = true;
    }
}

void RectBuilder::height(TypedSizeGetter height, TypedSize padding){
    if(bottomF == 0 || topF != 0){
        bottomF = rb_lazyy;
        bottomD.lazyy.rect = this;
        bottomD.lazyy.baseBottom = false;
        bottomD.lazyy.dy = height;
        bottomD.lazyy.padding = padding;
        useBottomForHeight = true;
    }
    if(topF == 0){
        topF = rb_lazyy;
        topD.lazyy.rect = this;
        topD.lazyy.baseBottom = true;
        topD.lazyy.dy = height;
        topD.lazyy.padding = padding;
        useTopForHeight = true;
    }
}


void RectBuilder::above(RectBuilder *b, TypedSize padding){
    bottomF = rb_rely;
    bottomD.rely.rect = b;
    bottomD.rely.baseBottom = false;
    bottomD.rely.dy = padding;
    useBottomForHeight = true;
}

void RectBuilder::below(RectBuilder *b, TypedSize padding){
    topF = rb_rely;
    topD.rely.rect = b;
    topD.rely.baseBottom = true;
    topD.rely.dy = -padding;
    useTopForHeight = true;
}

void RectBuilder::toLeftOf(RectBuilder *b, TypedSize padding){
    rightF = rb_relx;
    rightD.relx.rect = b;
    rightD.relx.baseLeft = true;
    rightD.relx.dx = -padding;
    useRightForWidth = true;
}

void RectBuilder::toRightOf(RectBuilder *b, TypedSize padding){
    leftF = rb_relx;
    leftD.relx.rect = b;
    leftD.relx.baseLeft = false;
    leftD.relx.dx = padding;
    useLeftForWidth = true;
}


void RectBuilder::alignTop(RectBuilder *b, TypedSize padding){
    topF = rb_rely;
    topD.rely.rect = b;
    topD.rely.baseBottom = false;
    topD.rely.dy = padding;
    useTopForHeight = true;
}

void RectBuilder::alignBottom(RectBuilder *b, TypedSize padding){
    bottomF = rb_rely;
    bottomD.rely.rect = b;
    bottomD.rely.baseBottom = true;
    bottomD.rely.dy = -padding;
    useBottomForHeight = true;
}

void RectBuilder::alignLeft(RectBuilder *b, TypedSize padding){
    leftF = rb_relx;
    leftD.relx.rect = b;
    leftD.relx.baseLeft = true;
    leftD.relx.dx = -padding;
    useLeftForWidth = true;
}

void RectBuilder::alignRight(RectBuilder *b, TypedSize padding){
    rightF = rb_relx;
    rightD.relx.rect = b;
    rightD.relx.baseLeft = false;
    rightD.relx.dx = padding;
    useRightForWidth = true;
}


RectY rb_cenTop(RectBuilderData data, ScreenMetrics m){
    RectY r;
    RectBuilder *b = (RectBuilder*)data.cen.rect;
    RectBuilder *thiz = (RectBuilder*)data.cen.thiz;
    r.rect = b;
    r.baseBottom = true;
    r.dy = TypedSize::gl(data.cen.fract * (b == nullptr ? thiz->parent.getHeight() : b->getHeight(m)) + 0.5f * thiz->getHeight(m));
    return r;
}

RectY rb_cenBottom(RectBuilderData data, ScreenMetrics m){
    RectY r;
    RectBuilder *b = (RectBuilder*)data.cen.rect;
    RectBuilder *thiz = (RectBuilder*)data.cen.thiz;
    r.rect = b;
    r.baseBottom = true;
    r.dy = TypedSize::gl(data.cen.fract * (b == nullptr ? thiz->parent.getHeight() : b->getHeight(m)) - 0.5f * thiz->getHeight(m));
    return r;
}

void RectBuilder::centerVerticalInside(RectBuilder *b, float fraction){
    if(topF == 0 || bottomF != 0){
        topF = rb_cenTop;
        topD.cen.thiz = this;
        topD.cen.rect = b;
        topD.cen.fract = fraction;
        useTopForHeight = false;
    }
    if(bottomF == 0){
        bottomF = rb_cenBottom;
        bottomD.cen.thiz = this;
        bottomD.cen.rect = b;
        bottomD.cen.fract = fraction;
        useBottomForHeight = false;
    }
}

RectX rb_cenLeft(RectBuilderData data, ScreenMetrics m){
    RectX r;
    RectBuilder *b = (RectBuilder*)data.cen.rect;
    RectBuilder *thiz = (RectBuilder*)data.cen.thiz;
    r.rect = b;
    r.baseLeft = true;
    r.dx = TypedSize::gl(data.cen.fract * (b == nullptr ? thiz->parent.getWidth() : b->getWidth(m)) - 0.5f * thiz->getWidth(m));
    return r;
}

RectX rb_cenRight(RectBuilderData data, ScreenMetrics m){
    RectX r;
    RectBuilder *b = (RectBuilder*)data.cen.rect;
    RectBuilder *thiz = (RectBuilder*)data.cen.thiz;
    r.rect = b;
    r.baseLeft = true;
    r.dx = TypedSize::gl(data.cen.fract * (b == nullptr ? thiz->parent.getWidth() : b->getWidth(m)) + 0.5f * thiz->getWidth(m));
    return r;
}

void RectBuilder::centerHorizontalInside(RectBuilder *b, float fraction){
    if(leftF == 0 || rightF != 0){
        leftF = rb_cenLeft;
        leftD.cen.thiz = this;
        leftD.cen.rect = b;
        leftD.cen.fract = fraction;
        useLeftForWidth = false;
    }
    if(rightF == 0){
        rightF = rb_cenRight;
        rightD.cen.thiz = this;
        rightD.cen.rect = b;
        rightD.cen.fract = fraction;
        useRightForWidth = false;
    }
}
