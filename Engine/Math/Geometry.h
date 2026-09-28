#pragma once

// Bounding volumes, rays, planes, frusta and closest-point queries shared by the
// renderer (culling), physics (queries/narrowphase) and AI (line of sight).

#include "Engine/Math/VectorMath.h"

#include <array>
#include <limits>
#include <utility>

namespace Astral::Math {

struct AABB {
    Vec3 min{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()};
    Vec3 max{-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()};

    static AABB FromCenterExtents(Vec3 center, Vec3 extents) {
        return {center - extents, center + extents};
    }
    bool IsValid() const { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }
    Vec3 Center() const { return (min + max) * 0.5f; }
    Vec3 Extents() const { return (max - min) * 0.5f; }
    Vec3 Size() const { return max - min; }
    void Expand(Vec3 point) { min = Min(min, point); max = Max(max, point); }
    void Expand(const AABB& other) { min = Min(min, other.min); max = Max(max, other.max); }
    AABB Inflated(float amount) const {
        const Vec3 pad{amount, amount, amount};
        return {min - pad, max + pad};
    }
    float SurfaceArea() const {
        const Vec3 d = Size();
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }
    bool Contains(Vec3 p) const {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y
            && p.z >= min.z && p.z <= max.z;
    }
    bool Contains(const AABB& other) const {
        return other.min.x >= min.x && other.min.y >= min.y && other.min.z >= min.z
            && other.max.x <= max.x && other.max.y <= max.y && other.max.z <= max.z;
    }
    bool Overlaps(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x && min.y <= other.max.y
            && max.y >= other.min.y && min.z <= other.max.z && max.z >= other.min.z;
    }
};

inline AABB Merge(const AABB& a, const AABB& b) { return {Min(a.min, b.min), Max(a.max, b.max)}; }

// Arvo's method: tight AABB of a transformed AABB.
inline AABB TransformAABB(const Mat4& m, const AABB& box) {
    if (!box.IsValid()) return box;
    const Vec3 center = TransformPoint(m, box.Center());
    const Vec3 e = box.Extents();
    const Vec3 extents{
        std::fabs(m.m[0][0]) * e.x + std::fabs(m.m[0][1]) * e.y + std::fabs(m.m[0][2]) * e.z,
        std::fabs(m.m[1][0]) * e.x + std::fabs(m.m[1][1]) * e.y + std::fabs(m.m[1][2]) * e.z,
        std::fabs(m.m[2][0]) * e.x + std::fabs(m.m[2][1]) * e.y + std::fabs(m.m[2][2]) * e.z,
    };
    return {center - extents, center + extents};
}

struct Sphere {
    Vec3 center{};
    float radius{};
};

struct Ray {
    Vec3 origin{};
    Vec3 direction{0.0f, 0.0f, 1.0f}; // unit length
    Vec3 At(float t) const { return origin + direction * t; }
};

struct Plane {
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float d{}; // dot(normal, p) + d = 0

    static Plane FromPointNormal(Vec3 point, Vec3 unitNormal) {
        return {unitNormal, -Dot(unitNormal, point)};
    }
    float SignedDistance(Vec3 p) const { return Dot(normal, p) + d; }
    Plane Normalized() const {
        const float length = Length(normal);
        if (!(length > kEpsilon)) return *this;
        return {normal / length, d / length};
    }
};

// Six inward-facing planes: left, right, bottom, top, near, far.
struct Frustum {
    std::array<Plane, 6> planes{};

    // Gribb/Hartmann extraction for column-vector matrices with [0,1] clip depth.
    static Frustum FromViewProjection(const Mat4& vp) {
        auto row = [&vp](int r) { return Vec4{vp.m[r][0], vp.m[r][1], vp.m[r][2], vp.m[r][3]}; };
        const Vec4 r0 = row(0), r1 = row(1), r2 = row(2), r3 = row(3);
        auto make = [](Vec4 v) { return Plane{{v.x, v.y, v.z}, v.w}.Normalized(); };
        Frustum f;
        f.planes[0] = make(r3 + r0);
        f.planes[1] = make(r3 - r0);
        f.planes[2] = make(r3 + r1);
        f.planes[3] = make(r3 - r1);
        f.planes[4] = make(r2);
        f.planes[5] = make(r3 - r2);
        return f;
    }
    bool Intersects(const AABB& box) const {
        if (!box.IsValid()) return false;
        const Vec3 center = box.Center();
        const Vec3 extents = box.Extents();
        for (const Plane& plane : planes) {
            const float radius = extents.x * std::fabs(plane.normal.x)
                + extents.y * std::fabs(plane.normal.y) + extents.z * std::fabs(plane.normal.z);
            if (plane.SignedDistance(center) < -radius) return false;
        }
        return true;
    }
    bool Intersects(const Sphere& sphere) const {
        for (const Plane& plane : planes)
            if (plane.SignedDistance(sphere.center) < -sphere.radius) return false;
        return true;
    }
};

// Slab test. On hit returns entry distance (0 when the origin is inside).
inline bool IntersectRayAABB(const Ray& ray, const AABB& box, float maxDistance, float& tHit,
    Vec3* hitNormal = nullptr) {
    float tMin = 0.0f;
    float tMax = maxDistance;
    int hitAxis = -1;
    float hitSign = 0.0f;
    for (int axis = 0; axis < 3; ++axis) {
        const float origin = Component(ray.origin, axis);
        const float direction = Component(ray.direction, axis);
        const float low = Component(box.min, axis);
        const float high = Component(box.max, axis);
        if (std::fabs(direction) < 1.0e-12f) {
            if (origin < low || origin > high) return false;
            continue;
        }
        const float inverse = 1.0f / direction;
        float t0 = (low - origin) * inverse;
        float t1 = (high - origin) * inverse;
        float sign = -1.0f;
        if (t0 > t1) { std::swap(t0, t1); sign = 1.0f; }
        if (t0 > tMin) { tMin = t0; hitAxis = axis; hitSign = sign; }
        tMax = std::min(tMax, t1);
        if (tMin > tMax) return false;
    }
    tHit = tMin;
    if (hitNormal) {
        *hitNormal = {};
        if (hitAxis >= 0) SetComponent(*hitNormal, hitAxis, hitSign);
    }
    return true;
}

inline bool IntersectRaySphere(const Ray& ray, Vec3 center, float radius, float maxDistance, float& tHit) {
    const Vec3 m = ray.origin - center;
    const float b = Dot(m, ray.direction);
    const float c = LengthSquared(m) - radius * radius;
    if (c > 0.0f && b > 0.0f) return false;
    const float discriminant = b * b - c;
    if (discriminant < 0.0f) return false;
    float t = -b - std::sqrt(discriminant);
    if (t < 0.0f) t = 0.0f;
    if (t > maxDistance) return false;
    tHit = t;
    return true;
}

inline bool IntersectRayPlane(const Ray& ray, const Plane& plane, float maxDistance, float& tHit) {
    const float denominator = Dot(plane.normal, ray.direction);
    if (std::fabs(denominator) < 1.0e-8f) return false;
    const float t = -plane.SignedDistance(ray.origin) / denominator;
    if (t < 0.0f || t > maxDistance) return false;
    tHit = t;
    return true;
}

// Moller-Trumbore, double-sided. Returns barycentrics (u, v) for vertices b and c.
inline bool IntersectRayTriangle(const Ray& ray, Vec3 a, Vec3 b, Vec3 c, float maxDistance,
    float& tHit, float& u, float& v) {
    const Vec3 edge1 = b - a;
    const Vec3 edge2 = c - a;
    const Vec3 p = Cross(ray.direction, edge2);
    const float determinant = Dot(edge1, p);
    if (std::fabs(determinant) < 1.0e-10f) return false;
    const float inverse = 1.0f / determinant;
    const Vec3 s = ray.origin - a;
    u = Dot(s, p) * inverse;
    if (u < 0.0f || u > 1.0f) return false;
    const Vec3 q = Cross(s, edge1);
    v = Dot(ray.direction, q) * inverse;
    if (v < 0.0f || u + v > 1.0f) return false;
    const float t = Dot(edge2, q) * inverse;
    if (t < 0.0f || t > maxDistance) return false;
    tHit = t;
    return true;
}

inline Vec3 ClosestPointOnSegment(Vec3 p, Vec3 a, Vec3 b, float* tOut = nullptr) {
    const Vec3 ab = b - a;
    const float lengthSquared = LengthSquared(ab);
    float t = lengthSquared > kEpsilon ? Dot(p - a, ab) / lengthSquared : 0.0f;
    t = Saturate(t);
    if (tOut) *tOut = t;
    return a + ab * t;
}

inline Vec3 ClosestPointOnAABB(Vec3 p, const AABB& box) { return Clamp(p, box.min, box.max); }

// Ericson, Real-Time Collision Detection 5.1.9. Returns squared distance.
inline float ClosestPointsSegmentSegment(Vec3 p1, Vec3 q1, Vec3 p2, Vec3 q2,
    float& s, float& t, Vec3& c1, Vec3& c2) {
    const Vec3 d1 = q1 - p1;
    const Vec3 d2 = q2 - p2;
    const Vec3 r = p1 - p2;
    const float a = LengthSquared(d1);
    const float e = LengthSquared(d2);
    const float f = Dot(d2, r);
    if (a <= kEpsilon && e <= kEpsilon) {
        s = t = 0.0f;
        c1 = p1;
        c2 = p2;
        return LengthSquared(c1 - c2);
    }
    if (a <= kEpsilon) {
        s = 0.0f;
        t = Saturate(f / e);
    } else {
        const float c = Dot(d1, r);
        if (e <= kEpsilon) {
            t = 0.0f;
            s = Saturate(-c / a);
        } else {
            const float b = Dot(d1, d2);
            const float denominator = a * e - b * b;
            s = denominator > kEpsilon ? Saturate((b * f - c * e) / denominator) : 0.0f;
            t = (b * s + f) / e;
            if (t < 0.0f) {
                t = 0.0f;
                s = Saturate(-c / a);
            } else if (t > 1.0f) {
                t = 1.0f;
                s = Saturate((b - c) / a);
            }
        }
    }
    c1 = p1 + d1 * s;
    c2 = p2 + d2 * t;
    return LengthSquared(c1 - c2);
}

// Ericson 5.1.5: closest point on triangle abc to p.
inline Vec3 ClosestPointOnTriangle(Vec3 p, Vec3 a, Vec3 b, Vec3 c) {
    const Vec3 ab = b - a, ac = c - a, ap = p - a;
    const float d1 = Dot(ab, ap), d2 = Dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;
    const Vec3 bp = p - b;
    const float d3 = Dot(ab, bp), d4 = Dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b;
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) return a + ab * (d1 / (d1 - d3));
    const Vec3 cp = p - c;
    const float d5 = Dot(ab, cp), d6 = Dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c;
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) return a + ac * (d2 / (d2 - d6));
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
    const float denominator = 1.0f / (va + vb + vc);
    const float v = vb * denominator;
    const float w = vc * denominator;
    return a + ab * v + ac * w;
}

} // namespace Astral::Math
