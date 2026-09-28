#pragma once

// Collision shapes, rigid poses and the small 3x3 matrix helpers the solver
// needs. Capsules are Y-aligned in their local frame (core segment +/- halfHeight).

#include "Engine/Math/Geometry.h"

namespace Astral::Physics {

using Math::Quat;
using Math::Vec3;

struct Mat3 {
    float m[3][3]{};

    static Mat3 Identity() {
        Mat3 r;
        r.m[0][0] = r.m[1][1] = r.m[2][2] = 1.0f;
        return r;
    }
    static Mat3 Diagonal(Vec3 d) {
        Mat3 r;
        r.m[0][0] = d.x;
        r.m[1][1] = d.y;
        r.m[2][2] = d.z;
        return r;
    }
    static Mat3 FromQuat(Quat q) {
        const Vec3 x = Math::Rotate(q, {1, 0, 0}), y = Math::Rotate(q, {0, 1, 0}), z = Math::Rotate(q, {0, 0, 1});
        Mat3 r;
        r.m[0][0] = x.x; r.m[0][1] = y.x; r.m[0][2] = z.x;
        r.m[1][0] = x.y; r.m[1][1] = y.y; r.m[1][2] = z.y;
        r.m[2][0] = x.z; r.m[2][1] = y.z; r.m[2][2] = z.z;
        return r;
    }
    Vec3 Column(int c) const { return {m[0][c], m[1][c], m[2][c]}; }
    Mat3 Transposed() const {
        Mat3 r;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) r.m[i][j] = m[j][i];
        return r;
    }
};

inline Vec3 operator*(const Mat3& a, Vec3 v) {
    return {a.m[0][0] * v.x + a.m[0][1] * v.y + a.m[0][2] * v.z, a.m[1][0] * v.x + a.m[1][1] * v.y + a.m[1][2] * v.z,
        a.m[2][0] * v.x + a.m[2][1] * v.y + a.m[2][2] * v.z};
}
inline Mat3 operator*(const Mat3& a, const Mat3& b) {
    Mat3 r;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j];
    return r;
}

struct Pose {
    Vec3 position{};
    Quat rotation{};

    Vec3 TransformPoint(Vec3 local) const { return position + Math::Rotate(rotation, local); }
    Vec3 InverseTransformPoint(Vec3 world) const { return Math::Rotate(Math::Conjugate(rotation), world - position); }
    Vec3 TransformVector(Vec3 local) const { return Math::Rotate(rotation, local); }
    Vec3 InverseTransformVector(Vec3 world) const { return Math::Rotate(Math::Conjugate(rotation), world); }
};

enum class ShapeType : std::uint8_t { Sphere, Capsule, Box };

struct Shape {
    ShapeType type{ShapeType::Sphere};
    float radius{0.5f};      // sphere and capsule
    float halfHeight{0.0f};  // capsule core half length
    Vec3 halfExtents{0.5f, 0.5f, 0.5f}; // box

    static Shape Sphere(float r) {
        Shape s;
        s.type = ShapeType::Sphere;
        s.radius = r;
        return s;
    }
    // Total height includes both hemispherical caps.
    static Shape CapsuleFromHeight(float r, float totalHeight) {
        Shape s;
        s.type = ShapeType::Capsule;
        s.radius = r;
        s.halfHeight = Math::Clamp(totalHeight * 0.5f - r, 0.0f, 1.0e6f);
        return s;
    }
    static Shape Box(Vec3 half) {
        Shape s;
        s.type = ShapeType::Box;
        s.halfExtents = half;
        return s;
    }

    bool IsValid() const {
        switch (type) {
        case ShapeType::Sphere: return Math::IsFinite(radius) && radius > 0.0f;
        case ShapeType::Capsule: return Math::IsFinite(radius) && radius > 0.0f && Math::IsFinite(halfHeight) && halfHeight >= 0.0f;
        case ShapeType::Box: return Math::IsFinite(halfExtents) && halfExtents.x > 0.0f && halfExtents.y > 0.0f && halfExtents.z > 0.0f;
        }
        return false;
    }
    float Volume() const {
        switch (type) {
        case ShapeType::Sphere: return 4.0f / 3.0f * Math::kPi * radius * radius * radius;
        case ShapeType::Capsule:
            return Math::kPi * radius * radius * (2.0f * halfHeight) + 4.0f / 3.0f * Math::kPi * radius * radius * radius;
        case ShapeType::Box: return 8.0f * halfExtents.x * halfExtents.y * halfExtents.z;
        }
        return 0.0f;
    }
    // Principal moments of inertia for unit density scaled to `mass`.
    Vec3 InertiaDiagonal(float mass) const {
        switch (type) {
        case ShapeType::Sphere: {
            const float i = 0.4f * mass * radius * radius;
            return {i, i, i};
        }
        case ShapeType::Capsule: {
            // Cylinder plus two hemispheres (standard approximation).
            const float h = 2.0f * halfHeight;
            const float r2 = radius * radius;
            const float axial = 0.5f * mass * r2;
            const float lateral = mass * (3.0f * r2 + h * h) / 12.0f + mass * r2 * 0.25f;
            return {lateral, axial, lateral};
        }
        case ShapeType::Box: {
            const Vec3 d = halfExtents * 2.0f;
            return {mass * (d.y * d.y + d.z * d.z) / 12.0f, mass * (d.x * d.x + d.z * d.z) / 12.0f,
                mass * (d.x * d.x + d.y * d.y) / 12.0f};
        }
        }
        return {1, 1, 1};
    }
    Math::AABB WorldBounds(const Pose& pose) const {
        switch (type) {
        case ShapeType::Sphere: return Math::AABB::FromCenterExtents(pose.position, {radius, radius, radius});
        case ShapeType::Capsule: {
            const Vec3 axis = pose.TransformVector({0.0f, halfHeight, 0.0f});
            Math::AABB box = Math::AABB::FromCenterExtents(pose.position + axis, {radius, radius, radius});
            box.Expand(Math::AABB::FromCenterExtents(pose.position - axis, {radius, radius, radius}));
            return box;
        }
        case ShapeType::Box: {
            const Mat3 r = Mat3::FromQuat(pose.rotation);
            const Vec3 e{
                std::fabs(r.m[0][0]) * halfExtents.x + std::fabs(r.m[0][1]) * halfExtents.y + std::fabs(r.m[0][2]) * halfExtents.z,
                std::fabs(r.m[1][0]) * halfExtents.x + std::fabs(r.m[1][1]) * halfExtents.y + std::fabs(r.m[1][2]) * halfExtents.z,
                std::fabs(r.m[2][0]) * halfExtents.x + std::fabs(r.m[2][1]) * halfExtents.y + std::fabs(r.m[2][2]) * halfExtents.z};
            return Math::AABB::FromCenterExtents(pose.position, e);
        }
        }
        return {};
    }
};

} // namespace Astral::Physics
