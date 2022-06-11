#pragma once

struct ConvexPolygon {
    struct Plane {
        Vec2 normal;
        float w;
        void debug() const {
            Vec3 { normal, w }.debug();
        }
    };
    std::vector<Plane> planes;

    static Vec2 intersection(Plane p1, Plane p2) {
        float d = 1.0f / (p1.normal.x * p2.normal.y - p1.normal.y * p2.normal.x);
        return Vec2 { (p1.normal.y * p2.w - p2.normal.y * p1.w) * d, -(p1.normal.x * p2.w - p2.normal.x * p1.w) * d };
    }
    static bool checkIntersectionOrder(Plane on, Plane left, Plane check, Plane right) {
        if (1.0f - Vec2::dot(on.normal, left.normal) < 0.001f)
            return true;
        if (1.0f - Vec2::dot(on.normal, right.normal) < 0.001f)
            return true;
        if (Vec2::dot(on.normal, left.normal) + 1.0f < 0.001f)
            return true;
        if (Vec2::dot(on.normal, right.normal) + 1.0f < 0.001f)
            return true;
        Vec2 edge = Vec2::findOrthogonal(on.normal);
        float l = Vec2::dot(edge, intersection(on, left));
        float c = Vec2::dot(edge, intersection(on, check));
        float r = Vec2::dot(edge, intersection(on, right));
        // l -> c -> r => true
        // r -> l -> c => true
        // c -> r -> l => true
        // l -> r -> c => false
        // c -> l -> r => false
        // r -> c -> l => false
        int_t count = (l < c) + (c < r) + (r < l);
        log("{{{}, {}}} -> {{{}, {}}} -> {{{}, {}}}", intersection(on, left).x, intersection(on, left).y, intersection(on, check).x, intersection(on, check).y, intersection(on, right).x, intersection(on, right).y);
        log("{} -> {} -> {}", l, c, r);
        VERIFY(Math, count == 2 || count == 1);
        return count == 2;
    }
    static float negateIf(float f, bool b) { return b ? f : -f; }
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
    static bool compareNormalAngles(const Plane& l, const Plane& r) {
        return compareAngles(l.normal, r.normal);
    }

    static ConvexPolygon intersection(const ConvexPolygon& poly1, const ConvexPolygon& poly2) {
        if (poly1.empty() || poly2.empty())
            return {};

        ConvexPolygon result;
        result.planes.reserve(poly1.planes.size() + poly2.planes.size());
        result.planes = poly1.planes;
        auto removePlains = [&]() -> bool {
            for (int_t i = 0; i < result.planes.size() && result.planes.size() >= 4; i++) {
                auto plane = [&](int_t rel) -> Plane { return result.planes[(i + rel + result.planes.size()) % result.planes.size()]; };
                bool b1 = checkIntersectionOrder(plane(+1), plane(-1), plane(0), plane(+2));
                bool b2 = checkIntersectionOrder(plane(-1), plane(-2), plane(0), plane(+1));
                if (!b1 || !b2) {
                    log("removing");
                    plane(0).debug();
                    result.planes.erase(result.planes.begin() + i);
                    i -= 1;
                }
            }

            if (result.planes.size() > 3)
                return true;

            // triangle base case
            EXPECT(Math, result.planes.size(), (size_t)3);
            Vec2 v0 = intersection(result.planes[2], result.planes[0]);
            Vec2 v1 = intersection(result.planes[0], result.planes[1]);
            Vec2 v2 = intersection(result.planes[1], result.planes[2]);
            int_t count = compareAngles(v0, v1) + compareAngles(v1, v2) + compareAngles(v2, v0);
            if (count == 2)
                return true;
            EXPECT(Math, count, (int_t)1);
            return false;
        };
        for (auto plane : poly2.planes) {
            log("----------------");
            result.planes.push_back(plane);
            std::sort(result.planes.begin(), result.planes.end(), compareNormalAngles);
            if (!removePlains())
                return {};
            for (auto p : result.planes)
                p.debug();
        }

        return result;
    }

    static ConvexPolygon fromOrderedVertices(ArrayView<const Vec2> vertices) {
        ConvexPolygon result;
        result.planes.reserve(vertices.size());
        auto makePlane = [&](Vec2 v1, Vec2 v2) -> Plane {
            Vec2 normal = Vec2::findOrthogonal(v2 - v1); // rotates 90 degress clockwise
            float w = -Vec2::dot(v1, normal);
            return { normal, w };
        };
        Plane p0 = makePlane(vertices[vertices.size() - 1], vertices[0]);
        if (p0.normal.y >= 0.0f)
            result.planes.push_back(p0);
        for (int_t i = 1; i < vertices.size(); i++)
            result.planes.push_back(makePlane(vertices[i - 1], vertices[i]));
        if (result.planes.size() != vertices.size())
            result.planes.push_back(p0);

        std::sort(result.planes.begin(), result.planes.end(), compareNormalAngles);
        return result;
    }

    bool empty() const { return planes.empty(); }
    void computeVertices(Vec2* out) const {
        out[0] = intersection(planes[planes.size() - 1], planes[0]);
        for (int_t i = 1; i < planes.size(); i++)
            out[i] = intersection(planes[i - 1], planes[i]);
    }
};
