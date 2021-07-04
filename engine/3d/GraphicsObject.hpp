#pragma once

#include "GL/Framebuffer.hpp"
#include "math/Box.hpp"
#include "math/Mat4x4.hpp"
#include "util/IdAllocator.hpp"

struct GraphicsObject {
    Mat4x4 modelMatrix;
    DrawMode drawMode;
    VertexBuffer vertices;
    VertexBuffer normals;
    IndexBuffer indices;
    Box boundingBox;
};

class GraphicsObjectCollection {
public:
    id_t newObject() {
        id_t id = idAllocator.allocateId();
        if (id == objects.size()) {
            objects.resize(id + 1);
        }
        return id;
    }

    GraphicsObject& operator[](id_t id) { return objects[id]; }

    id_t insertObject(GraphicsObject obj) {
        id_t id = idAllocator.allocateId();
        if (id == objects.size()) {
            objects.emplace_back(obj);
        }
        return id;
    }

private:
    IdAllocator<> idAllocator;
    std::vector<GraphicsObject> objects;
};
