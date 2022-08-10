#pragma once

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

    int_t size() const { return vertices.size(); }

    struct Plane {
        Vec2 normal;
        float w;
        bool check(Vec2 pos) const {
            return Vec2::dot(pos, normal) + w > 0.0f;
        }
        Vec2 intersection(Vec2 v1, Vec2 v2) const {
            float t = (Vec2::dot(v2, normal) + w) / Vec2::dot(normal, v2 - v1);
            VERIFY(Math, t >= 0.0f && t <= 1.0f);
            return t * v1 + (1.0f - t) * v2;
        }
    };
    void extendBy(Plane plane) {
        std::vector<Vec2> newVerts;
        newVerts.reserve(vertices.size() + 1);
        Vec2 prev = vertices.back();
        for (Vec2 cur : vertices) {
            if (plane.check(cur) != plane.check(prev))
                newVerts.push_back(plane.intersection(prev, cur));
            if (plane.check(cur))
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

    bool empty() const { return vertices.empty(); }

private:
    Vec2 operator[](int_t i) const {
        return vertices[(size() + i) % size()];
    }
};
