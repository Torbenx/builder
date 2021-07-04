#pragma once

#include "3d/Shaderpass.hpp"
#include "GL/Framebuffer.hpp"
#include "math/Mat4x4.hpp"
#include "util/Shaders.hpp"
#include <vector>

struct DepthRenderpass {
    struct Entry {
        id_t objectId;
    };
    Framebuffer* framebuffer;
    Mat4x4 viewProjectionMatrix;
    std::vector<Entry> entries;
    Resource<SimpleDepthShader> depthShader = Shaders::depth();

    void draw(GraphicsObjectCollection& objects) {
        framebuffer->bind();
        framebuffer->clear();
        for (Entry& entry : entries) {
            GraphicsObject& object = objects[entry.objectId];
            depthShader->uniformMat4x4(depthShader->mvpUniform, viewProjectionMatrix * object.modelMatrix);
            depthShader->attribPointer(depthShader->positionAttrib, object.vertices);
            framebuffer->draw(object.drawMode, object.indices);
        }
    }
};
