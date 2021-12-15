#pragma once

#include "GL/Framebuffer.hpp"
#include "math/Box.hpp"
#include "math/Mat4x4.hpp"
#include "containers/IdAllocator.hpp"

struct GraphicsObject {
    Mat4x4 modelMatrix;
    DrawMode drawMode;
    VertexAttribHelper<Vec3> vertices;
    VertexAttribHelper<Vec3> normals;
    IndexBuffer indices;

    ~GraphicsObject([[context]] GAllocator* allocator) { }
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

    const GraphicsObject& operator[](id_t id) { return *objects[id]; }

    id_t insertObject(const GraphicsObject& obj) {
        id_t id = idAllocator.allocateId();
        if (id == objects.size()) {
            objects.emplace_back(&obj);
        }
        return id;
    }

private:
    IdAllocator<> idAllocator;
    std::vector<const GraphicsObject*> objects;
};
