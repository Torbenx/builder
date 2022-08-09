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

    // https://tildesites.bowdoin.edu/~ltoma/teaching/cs3250-CompGeom/spring17/Lectures/cg-convexintersection.pdf
    // https://www.cs.jhu.edu/~misha/Spring16/ORourke82.pdf
    static ConvexPolygon intersection(const ConvexPolygon& poly1, const ConvexPolygon& poly2) {
        log("----------------------");
        if (poly1.empty() || poly2.empty())
            return {};

        int_t i1 = 0, i2 = 0;
        for (int_t counter = 0; counter < 2 * (poly1.size() + poly2.size()); counter++) {
            Vec2 v1 = poly1[i1];
            Vec2 v2 = poly2[i2];
            Vec2 e1 = v1 - poly1[i1 - 1];
            Vec2 e2 = v2 - poly2[i2 - 1];

            Vec2 deltaV = v2 - v1;
            Vec2 n1 = Vec2::rotateNegative(e1); // rotates 90 degress clockwise
            Vec2 n2 = Vec2::rotateNegative(e2);
            float factor = 1.0f / Vec2::dot(e1, n2);
            float t1 = factor * Vec2::dot(n2, deltaV);
            float t2 = factor * Vec2::dot(n1, deltaV);
            // log("({}, {}) -> ({}, {})", e1.x, e1.y, v1.x, v1.y);
            // log("({}, {}) -> ({}, {})", e2.x, e2.y, v2.x, v2.y);
            if (t1 >= -1.0f && t1 <= 0.0f && t2 >= -1.0f && t2 <= 0.0f) {
                log("intersection {}, {}", t1, t2);
                (v1 + e1 * t1).debug();
                break;
            } else {
                bool s1 = std::signbit(t1);
                if (s1 != std::signbit(t2)) {
                    i1 += s1 ? 1 : 0;
                    i2 += s1 ? 0 : 1;
                } else {
                    bool outside = Vec2::dot(n1, deltaV) > 0.0f;
                    i1 += outside ? 0 : 1;
                    i2 += outside ? 1 : 0;
                }
            }
        }
        return {};
    }

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
