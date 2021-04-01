#ifndef HEADER_GRAPHIC_FRAMEBUFFER_H
#define HEADER_GRAPHIC_FRAMEBUFFER_H

#include "Texture.hpp"
#include "../math/Vec4.hpp"
#include "CubeMap.hpp"

class Framebuffer{
public:
    static Framebuffer DEFAULT;

    //this should be private
    int offsetY, offsetX, width, height;

    Framebuffer() : Framebuffer(false, 0) {}
    ~Framebuffer();
    void bind();

    void clear();

    void colorAttachment(Texture& a);
    void colorAttachment(GLenum target,CubeMap& a);
    void depthAttachment(Texture& a);
    void depthAttachment(GLenum target,CubeMap& a);
    void stencilAttachment(Texture& a);
    void stencilAttachment(GLenum target, CubeMap& a);

    void clearColor(float r, float g, float b, float a);
    void clearColor(Vec4 color);
    void viewport(int width, int height);
    void viewport(int offsetX, int offsetY, int width, int height);
    void colorMask(bool r, bool g, bool b, bool a);
    void depthMask(bool d);
    void stencilMask(GLbitfield s);

private:
    struct Attachment{
        enum Type{
            Empty,
            TextureAttachemnt,
            CubeMapAttachment,
        } type;

        union{
            Texture *texture;
            struct{
                CubeMap *cubeMap;
                GLenum target;
            } cube;
        } data;

        Attachment() : type(Type::Empty) {}
    };

    static Framebuffer *currentFramebuffer;

    GLuint glId;
    bool isDefault;
    Attachment m_colorAttachment, m_depthAttachment, m_stencilAttachment;
    Vec4 m_clearColor;
    bool rMask = true, gMask = true, bMask = true, aMask = true, dMask = true;
    GLbitfield sMask = 0xffffffff;
    GLbitfield clearMask = 0;

    Framebuffer(bool isDefault, GLbitfield clearMask);
    Framebuffer(const Framebuffer& other);

    bool isCurrent() { return this == currentFramebuffer; }
    bool attach(GLenum, Attachment&);
};

#endif
