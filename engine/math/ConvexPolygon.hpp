#pragma once

#include "geometry.hpp"

namespace newmath {

struct ConvexPolygon {
    std::vector<Vec2> vertices;

    static bool compareAngles(Vec2 l, Vec2 r) {
        bool lPositive = l.y >= 0.0f;
        bool rPositive = r.y >= 0.0f;
        if (lPositive != rPositive) {
            // l.y > 0 and r.y < 0 => l < y
            // l.y < 0 and r.y > 0 => l > y
            return lPositive;
        }
        // when y > 0 greater x are smaller
        // when y < 0 smaller x are smaller
        float lx = negateIf(l.x, lPositive);
        float rx = negateIf(r.x, lPositive);
        return lx < rx;
    }
    static float negateIf(float f, bool b) { return b ? f : -f; }

    int_t vertexCount() const { return vertices.size(); }
    int_t edgeCount() const { return vertexCount() - 1; }
    bool empty() const { return vertices.empty(); }

    Vec2 intersection(Surface2 plane, Vec2 v1, Vec2 v2) {
        float t = (plane * v2) / (plane.normal * (v2 - v1));
        VERIFY(Math, t >= 0.0f && t <= 1.0f);
        return t * v1 + (1.0f - t) * v2;
    }
    void extendBy(Surface2 plane) {
        std::vector<Vec2> newVerts;
        newVerts.reserve(vertices.size() + 1);
        Vec2 prev = vertices.back();
        for (Vec2 cur : vertices) {
            if ((plane * cur >= 0) != (plane * prev >= 0))
                newVerts.push_back(intersection(plane, prev, cur));
            if (plane * cur >= 0)
                newVerts.push_back(cur);
            prev = cur;
        }
        vertices = newVerts;
    }

    static ConvexPolygon fromOrderedVertices(ArrayView<const Vec2> vertices) {
        ConvexPolygon result;
        result.vertices.insert(result.vertices.end(), vertices.begin(), vertices.end());
        return result;
    }
    static ConvexPolygon fromUnorderedVertices(ArrayView<const Vec2> vertices) {
        ConvexPolygon result;
        result.vertices.insert(result.vertices.end(), vertices.begin(), vertices.end());
        Vec2 avg { 0.0f, 0.0f };
        for (Vec2 v : vertices)
            avg += v;
        avg /= vertices.size();
        for (Vec2& v : result.vertices)
            v -= avg;
        std::sort(result.vertices.begin(), result.vertices.end(), compareAngles);
        for (Vec2& v : result.vertices)
            v += avg;
        return result;
    }
};

}
