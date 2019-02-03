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

void recty_debug(RectY r, RectY r2){
    LOGD("%s.%s+%f = %s.%s+%f", r.rect ? ((RectBuilder*)r.rect)->name.c_str() : "null", r.baseBottom ? "b" : "t", r.dy, r2.rect ? ((RectBuilder*)r2.rect)->name.c_str() : "null", r2.baseBottom ? "b" : "t", r2.dy);
}

RectY recty_deref(RectY r, bool forHeight=false){
    RectBuilder *b = (RectBuilder*)r.rect;
    if(b == nullptr)return r;
    if(r.baseBottom){
        if(b->bottomF != 0){
            if(forHeight && !b->useBottomForHeight)return r;
            RectY r2 = recty_deref(b->bottomF(b->bottomD), forHeight);
            r2.dy += r.dy;
            //recty_debug(r, r2);
            return r2;
        }
        return r;
    }else{
        if(b->topF != 0){
            if(forHeight && !b->useTopForHeight)return r;
            RectY r2 = recty_deref(b->topF(b->topD), forHeight);
            r2.dy += r.dy;
            //recty_debug(r, r2);
            return r2;
        }
        return r;
    }
}

RectX rectx_deref(RectX r, bool forWidth=false){
    RectBuilder *b = (RectBuilder*)r.rect;
    if(b == nullptr)return r;
    if(r.baseLeft){
        if(b->leftF != 0){
            if(forWidth && !b->useLeftForWidth)return r;
            RectX r2 = rectx_deref(b->leftF(b->leftD), forWidth);
            r2.dx += r.dx;
            return r2;
        }
        return r;
    }else{
        if(b->rightF != 0){
            if(forWidth && !b->useRightForWidth)return r;
            RectX r2 = rectx_deref(b->rightF(b->rightD), forWidth);
            r2.dx += r.dx;
            return r2;
        }
        return r;
    }
}

float RectBuilder::getWidth(){
    RectX left,right;
    left.rect = this;
    left.baseLeft = true;
    left.dx = 0.0f;
    left = rectx_deref(left, true);
    right.rect = this;
    right.baseLeft = false;
    right.dx = 0.0f;
    right = rectx_deref(right, true);
    if(left.rect == right.rect && left.baseLeft == right.baseLeft){
        return right.dx - left.dx;
    }
    LOGW("getWidth failed");
    return NAN;
}

float RectBuilder::getHeight(){
    RectY top,bottom;
    top.rect = this;
    top.baseBottom = false;
    top.dy = 0.0f;
    top = recty_deref(top, true);
    bottom.rect = this;
    bottom.baseBottom = true;
    bottom.dy = 0.0f;
    bottom = recty_deref(bottom, true);
    if(top.rect == bottom.rect && top.baseBottom == bottom.baseBottom){
        return top.dy - bottom.dy;
    }
    LOGW("getHeight failed");
    return NAN;
}

float RectBuilder::getTop(){
    RectY top;
    top.rect = this;
    top.baseBottom = false;
    top.dy = 0.0f;
    top = recty_deref(top);
    if(top.rect == nullptr){
        if(top.baseBottom){
            return top.dy - 1.0f;
        }else{
            return 1.0f + top.dy;
        }
    }
    LOGW("getTop failed");
    return NAN;
}

float RectBuilder::getBottom(){
    RectY bottom;
    bottom.rect = this;
    bottom.baseBottom = true;
    bottom.dy = 0.0f;
    bottom = recty_deref(bottom);
    if(bottom.rect == nullptr){
        if(bottom.baseBottom){
            return bottom.dy - 1.0f;
        }else{
            return 1.0f + bottom.dy;
        }
    }
    LOGW("getBottom failed");
    return NAN;
}

float RectBuilder::getLeft(){
    RectX left;
    left.rect = this;
    left.baseLeft = true;
    left.dx = 0.0f;
    left = rectx_deref(left);
    if(left.rect == nullptr){
        if(left.baseLeft){
            return left.dx - 1.0f;
        }else{
            return 1.0f + left.dx;
        }
    }
    LOGW("getLeft failed");
    return NAN;
}

float RectBuilder::getRight(){
    RectX right;
    right.rect = this;
    right.baseLeft = false;
    right.dx = 0.0f;
    right = rectx_deref(right);
    if(right.rect == nullptr){
        if(right.baseLeft){
            return right.dx - 1.0f;
        }else{
            return 1.0f + right.dx;
        }
    }
    LOGW("getLeft failed");
    return NAN;
}

Rect RectBuilder::build(){
    return Rect::bltr(getBottom(), getLeft(), getTop(), getRight());
}


RectY rb_rely(RectBuilderData data){
    return data.rely;
}
RectX rb_relx(RectBuilderData data){
    return data.relx;
}

void RectBuilder::width(float width){
    if(leftF == 0 || rightF != 0){
        leftF = rb_relx;
        leftD.relx.rect = this;
        leftD.relx.baseLeft = false;
        leftD.relx.dx = -width;
        useLeftForWidth = true;
    }
    if(rightF == 0){
        rightF = rb_relx;
        rightD.relx.rect = this;
        rightD.relx.baseLeft = true;
        rightD.relx.dx = width;
        useRightForWidth = true;
    }
}

void RectBuilder::height(float height){
    if(topF == 0 || bottomF != 0){
        topF = rb_rely;
        topD.rely.rect = this;
        topD.rely.baseBottom = true;
        topD.rely.dy = height;
        useTopForHeight = true;
    }
    if(bottomF == 0){
        bottomF = rb_rely;
        bottomD.rely.rect = this;
        bottomD.rely.baseBottom = false;
        bottomD.rely.dy = -height;
        useBottomForHeight = true;
    }
}


void RectBuilder::above(RectBuilder *b, float padding){
    bottomF = rb_rely;
    bottomD.rely.rect = b;
    bottomD.rely.baseBottom = false;
    bottomD.rely.dy = padding;
    useBottomForHeight = true;
}

void RectBuilder::below(RectBuilder *b, float padding){
    topF = rb_rely;
    topD.rely.rect = b;
    topD.rely.baseBottom = true;
    topD.rely.dy = -padding;
    useTopForHeight = true;
}

void RectBuilder::toLeftOf(RectBuilder *b, float padding){
    rightF = rb_relx;
    rightD.relx.rect = b;
    rightD.relx.baseLeft = true;
    rightD.relx.dx = -padding;
    useRightForWidth = true;
}

void RectBuilder::toRightOf(RectBuilder *b, float padding){
    leftF = rb_relx;
    leftD.relx.rect = b;
    leftD.relx.baseLeft = false;
    leftD.relx.dx = padding;
    useLeftForWidth = true;
}


void RectBuilder::alignTop(RectBuilder *b, float padding){
    topF = rb_rely;
    topD.rely.rect = b;
    topD.rely.baseBottom = false;
    topD.rely.dy = padding;
    useTopForHeight = true;
}

void RectBuilder::alignBottom(RectBuilder *b, float padding){
    bottomF = rb_rely;
    bottomD.rely.rect = b;
    bottomD.rely.baseBottom = true;
    bottomD.rely.dy = -padding;
    useBottomForHeight = true;
}

void RectBuilder::alignLeft(RectBuilder *b, float padding){
    leftF = rb_relx;
    leftD.relx.rect = b;
    leftD.relx.baseLeft = true;
    leftD.relx.dx = -padding;
    useLeftForWidth = true;
}

void RectBuilder::alignRight(RectBuilder *b, float padding){
    rightF = rb_relx;
    rightD.relx.rect = b;
    rightD.relx.baseLeft = false;
    rightD.relx.dx = padding;
    useRightForWidth = true;
}


RectY rb_cenTop(RectBuilderData data){
    RectY r;
    RectBuilder *b = (RectBuilder*)data.cen.rect;
    r.rect = b;
    r.baseBottom = true;
    r.dy = data.cen.fract * (b == nullptr ? 2.0f : b->getHeight()) + 0.5f * ((RectBuilder*)data.cen.thiz)->getHeight();
    return r;
}

RectY rb_cenBottom(RectBuilderData data){
    RectY r;
    RectBuilder *b = (RectBuilder*)data.cen.rect;
    r.rect = b;
    r.baseBottom = true;
    r.dy = data.cen.fract * (b == nullptr ? 2.0f : b->getHeight()) - 0.5f * ((RectBuilder*)data.cen.thiz)->getHeight();
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

RectX rb_cenLeft(RectBuilderData data){
    RectX r;
    RectBuilder *b = (RectBuilder*)data.cen.rect;
    r.rect = b;
    r.baseLeft = true;
    r.dx = data.cen.fract * (b == nullptr ? 2.0f : b->getWidth()) - 0.5f * ((RectBuilder*)data.cen.thiz)->getWidth();
    return r;
}

RectX rb_cenRight(RectBuilderData data){
    RectX r;
    RectBuilder *b = (RectBuilder*)data.cen.rect;
    r.rect = b;
    r.baseLeft = true;
    r.dx = data.cen.fract * (b == nullptr ? 2.0f : b->getWidth()) + 0.5f * ((RectBuilder*)data.cen.thiz)->getWidth();
    return r;
}

void RectBuilder::centerHorizontalInside(RectBuilder *b, float fraction){
    if(leftF == 0){
        leftF = rb_cenLeft;
        leftD.cen.thiz = this;
        leftD.cen.rect = b;
        leftD.cen.fract = fraction;
        useLeftForWidth = false;
    }
    if(rightF == 0 || leftF != 0){
        rightF = rb_cenRight;
        rightD.cen.thiz = this;
        rightD.cen.rect = b;
        rightD.cen.fract = fraction;
        useRightForWidth = false;
    }
}