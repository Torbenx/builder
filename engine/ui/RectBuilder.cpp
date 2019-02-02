#include "RectBuilder.hpp"
#include <math.h>
#include "../util/log.hpp"

#define PROP(s) (isnan(c##s) ? (s##F == 0 ? NAN : c##s = s##F(s##D)) : c##s)
#define WIDTH PROP(width)
#define HEIGHT PROP(height)
#define TOP PROP(top)
#define BOTTOM PROP(bottom)
#define LEFT PROP(left)
#define RIGHT PROP(right)

float RectBuilder::getWidth(){
    float r = WIDTH;
    if(isnan(r))return RIGHT - LEFT;
    return r;
}

float RectBuilder::getHeight(){
    float r = HEIGHT;
    if(isnan(r))return TOP - BOTTOM;
    return r;
}

float RectBuilder::getTop(){
    float r = TOP;
    if(isnan(r))return BOTTOM + HEIGHT;
    return r;
}

float RectBuilder::getBottom(){
    float r = BOTTOM;
    if(isnan(r))return TOP - HEIGHT;
    return r;
}

float RectBuilder::getLeft(){
    float r = LEFT;
    if(isnan(r))return RIGHT - WIDTH;
    return r;
}

float RectBuilder::getRight(){
    float r = RIGHT;
    if(isnan(r))return LEFT + WIDTH;
    return r;
}

Rect RectBuilder::build(){
    return Rect::bltr(getBottom(), getLeft(), getTop(), getRight());
}


float rb_f(RectBuilderData data){
    return data.f;
}

void RectBuilder::width(float width){
    widthF = rb_f;
    widthD.f = width;
}

void RectBuilder::height(float height){
    heightF = rb_f;
    heightD.f = height;
}

void RectBuilder::top(float top){
    topF = rb_f;
    topD.f = top;
}

void RectBuilder::bottom(float bottom){
    bottomF = rb_f;
    bottomD.f = bottom;
}

void RectBuilder::left(float left){
    leftF = rb_f;
    leftD.f = left;
}

void RectBuilder::right(float right){
    rightF = rb_f;
    rightD.f = right;
}


#define DEFrb_rel(s) \
float rb_rel##s(RectBuilderData data){ \
    return ((RectBuilder*)data.rel.b)->get##s() + data.rel.pad; \
}
DEFrb_rel(Width);
DEFrb_rel(Height);
DEFrb_rel(Top);
DEFrb_rel(Bottom);
DEFrb_rel(Left);
DEFrb_rel(Right);

void RectBuilder::above(RectBuilder *b, float padding){
    bottomF = rb_relTop;
    bottomD.rel.b = b;
    bottomD.rel.pad = padding;
}

void RectBuilder::below(RectBuilder *b, float padding){
    topF = rb_relBottom;
    topD.rel.b = b;
    topD.rel.pad = -padding;
}

void RectBuilder::toLeftOf(RectBuilder *b, float padding){
    rightF = rb_relLeft;
    rightD.rel.b = b;
    rightD.rel.pad = -padding;
}

void RectBuilder::toRightOf(RectBuilder *b, float padding){
    leftF = rb_relRight;
    leftD.rel.b = b;
    leftD.rel.pad = padding;
}


void RectBuilder::alignTop(RectBuilder *b, float padding){
    topF = rb_relTop;
    topD.rel.b = b;
    topD.rel.pad = padding;
}

void RectBuilder::alignBottom(RectBuilder *b, float padding){
    bottomF = rb_relBottom;
    bottomD.rel.b = b;
    bottomD.rel.pad = -padding;
}

void RectBuilder::alignLeft(RectBuilder *b, float padding){
    leftF = rb_relLeft;
    leftD.rel.b = b;
    leftD.rel.pad = -padding;
}

void RectBuilder::alignRight(RectBuilder *b, float padding){
    rightF = rb_relRight;
    rightD.rel.b = b;
    rightD.rel.pad = padding;
}


float rb_cenFFLeft(RectBuilderData data){
    float b = data.cenFF.big;
    float s = data.cenFF.small;
    return s + (b - s - ((RectBuilder*)data.cenFF.thiz)->getWidth()) / 2.0f;
}

float rb_cenFFTop(RectBuilderData data){
    float b = data.cenFF.big;
    float s = data.cenFF.small;
    return b - (b - s - ((RectBuilder*)data.cenFF.thiz)->getHeight()) / 2.0f;
}

float rb_cenBFLeft(RectBuilderData data){
    float b = ((RectBuilder*)data.cenBF.big)->getLeft();
    float s = data.cenBF.small;
    return s + (b - s - ((RectBuilder*)data.cenBF.thiz)->getWidth()) / 2.0f;
}

float rb_cenBFTop(RectBuilderData data){
    float b = ((RectBuilder*)data.cenBF.big)->getBottom();
    float s = data.cenBF.small;
    return b - (b - s - ((RectBuilder*)data.cenBF.thiz)->getHeight()) / 2.0f;
}

float rb_cenFBLeft(RectBuilderData data){
    float b = data.cenFB.big;
    float s = ((RectBuilder*)data.cenFB.small)->getRight();
    return s + (b - s - ((RectBuilder*)data.cenFB.thiz)->getWidth()) / 2.0f;
}

float rb_cenFBTop(RectBuilderData data){
    float b = data.cenFB.big;
    float s = ((RectBuilder*)data.cenFB.small)->getTop();
    return b - (b - s - ((RectBuilder*)data.cenFB.thiz)->getHeight()) / 2.0f;
}

float rb_cenBBLeft(RectBuilderData data){
    float b = ((RectBuilder*)data.cenBB.big)->getLeft();
    float s = ((RectBuilder*)data.cenBB.small)->getRight();
    return s + (b - s - ((RectBuilder*)data.cenBB.thiz)->getWidth()) / 2.0f;
}

float rb_cenBBTop(RectBuilderData data){
    float b = ((RectBuilder*)data.cenBB.big)->getBottom();
    float s = ((RectBuilder*)data.cenBB.small)->getTop();
    return b - (b - s - ((RectBuilder*)data.cenBB.thiz)->getHeight()) / 2.0f;
}

void RectBuilder::centerVertical(float above, float below){
    topF = rb_cenFFTop;
    topD.cenFF.thiz = this;
    topD.cenFF.big = above;
    topD.cenFF.small = below;
}

void RectBuilder::centerVertical(RectBuilder *above, float below){
    topF = rb_cenBFTop;
    topD.cenBF.thiz = this;
    topD.cenBF.big = above;
    topD.cenBF.small = below;
}

void RectBuilder::centerVertical(float above, RectBuilder *below){
    topF = rb_cenFBTop;
    topD.cenFB.thiz = this;
    topD.cenFB.big = above;
    topD.cenFB.small = below;
}

void RectBuilder::centerVertical(RectBuilder *above, RectBuilder *below){
    topF = rb_cenBBTop;
    topD.cenBB.thiz = this;
    topD.cenBB.big = above;
    topD.cenBB.small = below;
}

void RectBuilder::centerHorizontal(float left, float right){
    leftF = rb_cenFFLeft;
    leftD.cenFF.thiz = this;
    leftD.cenFF.big = right;
    leftD.cenFF.small = left;
}

void RectBuilder::centerHorizontal(RectBuilder *left, float right){
    leftF = rb_cenFBLeft;
    leftD.cenFB.thiz = this;
    leftD.cenFB.big = right;
    leftD.cenFB.small = left;
}

void RectBuilder::centerHorizontal(float left, RectBuilder *right){
    leftF = rb_cenBFLeft;
    leftD.cenBF.thiz = this;
    leftD.cenBF.big = right;
    leftD.cenBF.small = left;
}

void RectBuilder::centerHorizontal(RectBuilder *left, RectBuilder *right){
    leftF = rb_cenBBLeft;
    leftD.cenBB.thiz = this;
    leftD.cenBB.big = right;
    leftD.cenBB.small = left;
}