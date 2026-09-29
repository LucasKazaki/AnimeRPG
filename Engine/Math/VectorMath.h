#pragma once

// Operators and functions for the plain Math.h value types.
// Conventions: left-handed, +X right, +Y up, +Z forward (matches the existing
// PerspectiveCamera and WorldBlockout). Mat4 is row-major storage m[row][col]
// used with column vectors (p' = M * p), so translation lives in m[0..2][3].
// Clip-space depth is [0, 1] (Direct3D convention).

#include "Engine/Math/Math.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Astral::Math {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;
constexpr float kHalfPi = 0.5f * kPi;
constexpr float kEpsilon = 1.0e-6f;

inline float Radians(float degrees) { return degrees * (kPi / 180.0f); }
inline float Degrees(float radians) { return radians * (180.0f / kPi); }
inline float Clamp(float value, float low, float high) { return std::min(std::max(value, low), high); }
inline float Saturate(float value) { return Clamp(value, 0.0f, 1.0f); }
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
inline bool IsFinite(float value) { return std::isfinite(value); }
inline float SmoothStep(float edge0, float edge1, float x) {
    if (edge1 == edge0) return x < edge0 ? 0.0f : 1.0f;
    const float t = Saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}
// Frame-rate independent exponential approach: fraction of the gap closed after dt.
inline float DampFactor(float sharpness, float deltaSeconds) {
    return 1.0f - std::exp(-sharpness * deltaSeconds);
}
inline float WrapAngle(float radians) {
    radians = std::fmod(radians + kPi, kTwoPi);
    if (radians < 0.0f) radians += kTwoPi;
    return radians - kPi;
}

// ---------------------------------------------------------------- Vec2
inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator-(Vec2 a) { return {-a.x, -a.y}; }
inline Vec2 operator*(Vec2 a, float s) { return {a.x * s, a.y * s}; }
inline Vec2 operator*(float s, Vec2 a) { return {a.x * s, a.y * s}; }
inline Vec2 operator/(Vec2 a, float s) { return {a.x / s, a.y / s}; }
inline Vec2& operator+=(Vec2& a, Vec2 b) { a.x += b.x; a.y += b.y; return a; }
inline Vec2& operator-=(Vec2& a, Vec2 b) { a.x -= b.x; a.y -= b.y; return a; }
inline Vec2& operator*=(Vec2& a, float s) { a.x *= s; a.y *= s; return a; }
inline float Dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline float Cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }
inline float LengthSquared(Vec2 a) { return Dot(a, a); }
inline float Length(Vec2 a) { return std::sqrt(LengthSquared(a)); }
inline Vec2 Normalize(Vec2 a, Vec2 fallback = {0.0f, 0.0f}) {
    const float length = Length(a);
    return length > kEpsilon && IsFinite(length) ? a / length : fallback;
}
inline Vec2 Lerp(Vec2 a, Vec2 b, float t) { return a + (b - a) * t; }
inline Vec2 Perpendicular(Vec2 a) { return {-a.y, a.x}; }

// ---------------------------------------------------------------- Vec3
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator*(float s, Vec3 a) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator/(Vec3 a, float s) { return {a.x / s, a.y / s, a.z / s}; }
inline Vec3& operator+=(Vec3& a, Vec3 b) { a.x += b.x; a.y += b.y; a.z += b.z; return a; }
inline Vec3& operator-=(Vec3& a, Vec3 b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; return a; }
inline Vec3& operator*=(Vec3& a, float s) { a.x *= s; a.y *= s; a.z *= s; return a; }
inline Vec3& operator/=(Vec3& a, float s) { a.x /= s; a.y /= s; a.z /= s; return a; }
inline bool operator==(Vec3 a, Vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
inline bool operator!=(Vec3 a, Vec3 b) { return !(a == b); }
inline Vec3 Multiply(Vec3 a, Vec3 b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }
inline Vec3 Divide(Vec3 a, Vec3 b) { return {a.x / b.x, a.y / b.y, a.z / b.z}; }
inline float Dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 Cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float LengthSquared(Vec3 a) { return Dot(a, a); }
inline float Length(Vec3 a) { return std::sqrt(LengthSquared(a)); }
inline float Distance(Vec3 a, Vec3 b) { return Length(a - b); }
inline float DistanceSquared(Vec3 a, Vec3 b) { return LengthSquared(a - b); }
inline Vec3 Normalize(Vec3 a, Vec3 fallback = {0.0f, 0.0f, 0.0f}) {
    const float length = Length(a);
    return length > kEpsilon && IsFinite(length) ? a / length : fallback;
}
inline Vec3 Lerp(Vec3 a, Vec3 b, float t) { return a + (b - a) * t; }
inline Vec3 Min(Vec3 a, Vec3 b) { return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)}; }
inline Vec3 Max(Vec3 a, Vec3 b) { return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}; }
inline Vec3 Abs(Vec3 a) { return {std::fabs(a.x), std::fabs(a.y), std::fabs(a.z)}; }
inline Vec3 Clamp(Vec3 a, Vec3 low, Vec3 high) { return Min(Max(a, low), high); }
inline float MaxComponent(Vec3 a) { return std::max(a.x, std::max(a.y, a.z)); }
inline float MinComponent(Vec3 a) { return std::min(a.x, std::min(a.y, a.z)); }
inline bool IsFinite(Vec3 a) { return IsFinite(a.x) && IsFinite(a.y) && IsFinite(a.z); }
inline Vec3 Reflect(Vec3 incident, Vec3 normal) { return incident - normal * (2.0f * Dot(incident, normal)); }
inline Vec3 ProjectOnPlane(Vec3 v, Vec3 unitNormal) { return v - unitNormal * Dot(v, unitNormal); }
inline Vec3 Horizontal(Vec3 v) { return {v.x, 0.0f, v.z}; }
inline float Component(Vec3 v, int axis) { return axis == 0 ? v.x : (axis == 1 ? v.y : v.z); }
inline void SetComponent(Vec3& v, int axis, float value) {
    if (axis == 0) v.x = value; else if (axis == 1) v.y = value; else v.z = value;
}
// Two unit vectors orthogonal to n (n must be unit length).
inline void OrthonormalBasis(Vec3 n, Vec3& tangent, Vec3& bitangent) {
    // Duff et al. 2017, "Building an Orthonormal Basis, Revisited".
    const float sign = n.z >= 0.0f ? 1.0f : -1.0f;
    const float a = -1.0f / (sign + n.z);
    const float b = n.x * n.y * a;
    tangent = {1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x};
    bitangent = {b, sign + n.y * n.y * a, -n.y};
}
inline Vec3 MoveTowards(Vec3 current, Vec3 target, float maxDistance) {
    const Vec3 delta = target - current;
    const float distance = Length(delta);
    if (distance <= maxDistance || distance < kEpsilon) return target;
    return current + delta * (maxDistance / distance);
}

// ---------------------------------------------------------------- Vec4
inline Vec4 MakeVec4(Vec3 v, float w) { return {v.x, v.y, v.z, w}; }
inline Vec3 XYZ(Vec4 v) { return {v.x, v.y, v.z}; }
inline Vec4 operator+(Vec4 a, Vec4 b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
inline Vec4 operator-(Vec4 a, Vec4 b) { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
inline Vec4 operator*(Vec4 a, float s) { return {a.x * s, a.y * s, a.z * s, a.w * s}; }
inline Vec4 Lerp(Vec4 a, Vec4 b, float t) { return a + (b - a) * t; }
inline float Dot(Vec4 a, Vec4 b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

// ---------------------------------------------------------------- Mat4
inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 result{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result.m[row][col] = a.m[row][0] * b.m[0][col] + a.m[row][1] * b.m[1][col]
                + a.m[row][2] * b.m[2][col] + a.m[row][3] * b.m[3][col];
        }
    }
    return result;
}
inline Vec4 operator*(const Mat4& a, Vec4 v) {
    return {
        a.m[0][0] * v.x + a.m[0][1] * v.y + a.m[0][2] * v.z + a.m[0][3] * v.w,
        a.m[1][0] * v.x + a.m[1][1] * v.y + a.m[1][2] * v.z + a.m[1][3] * v.w,
        a.m[2][0] * v.x + a.m[2][1] * v.y + a.m[2][2] * v.z + a.m[2][3] * v.w,
        a.m[3][0] * v.x + a.m[3][1] * v.y + a.m[3][2] * v.z + a.m[3][3] * v.w,
    };
}
// Affine point transform (ignores the projective row).
inline Vec3 TransformPoint(const Mat4& a, Vec3 p) {
    return {
        a.m[0][0] * p.x + a.m[0][1] * p.y + a.m[0][2] * p.z + a.m[0][3],
        a.m[1][0] * p.x + a.m[1][1] * p.y + a.m[1][2] * p.z + a.m[1][3],
        a.m[2][0] * p.x + a.m[2][1] * p.y + a.m[2][2] * p.z + a.m[2][3],
    };
}
inline Vec3 TransformVector(const Mat4& a, Vec3 v) {
    return {
        a.m[0][0] * v.x + a.m[0][1] * v.y + a.m[0][2] * v.z,
        a.m[1][0] * v.x + a.m[1][1] * v.y + a.m[1][2] * v.z,
        a.m[2][0] * v.x + a.m[2][1] * v.y + a.m[2][2] * v.z,
    };
}
inline Vec3 GetTranslation(const Mat4& a) { return {a.m[0][3], a.m[1][3], a.m[2][3]}; }
inline Vec3 GetAxis(const Mat4& a, int column) { return {a.m[0][column], a.m[1][column], a.m[2][column]}; }
inline Mat4 Transpose(const Mat4& a) {
    Mat4 result{};
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col) result.m[row][col] = a.m[col][row];
    return result;
}
inline Mat4 Translation(Vec3 t) {
    Mat4 result = Mat4::Identity();
    result.m[0][3] = t.x; result.m[1][3] = t.y; result.m[2][3] = t.z;
    return result;
}
inline Mat4 Scaling(Vec3 s) {
    Mat4 result{};
    result.m[0][0] = s.x; result.m[1][1] = s.y; result.m[2][2] = s.z; result.m[3][3] = 1.0f;
    return result;
}
inline Mat4 RotationY(float radians) {
    Mat4 result = Mat4::Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    // Left-handed: positive yaw turns +Z toward +X.
    result.m[0][0] = c; result.m[0][2] = s;
    result.m[2][0] = -s; result.m[2][2] = c;
    return result;
}
inline Mat4 RotationX(float radians) {
    Mat4 result = Mat4::Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    // Positive pitch turns +Y toward +Z.
    result.m[1][1] = c; result.m[1][2] = -s;
    result.m[2][1] = s; result.m[2][2] = c;
    return result;
}
inline Mat4 RotationZ(float radians) {
    Mat4 result = Mat4::Identity();
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    result.m[0][0] = c; result.m[0][1] = -s;
    result.m[1][0] = s; result.m[1][1] = c;
    return result;
}
// General inverse via cofactors; returns false for singular or non-finite input.
inline bool Inverse(const Mat4& a, Mat4& out) {
    const float* m = &a.m[0][0];
    float inv[16];
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15]
        + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15]
        - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15]
        + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14]
        - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15]
        - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15]
        + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15]
        - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14]
        + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15]
        + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15]
        - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15]
        + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14]
        - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11]
        - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11]
        + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11]
        - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10]
        + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];
    const float determinant = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (!IsFinite(determinant) || std::fabs(determinant) < 1.0e-12f) return false;
    const float inverseDeterminant = 1.0f / determinant;
    float* o = &out.m[0][0];
    for (int i = 0; i < 16; ++i) o[i] = inv[i] * inverseDeterminant;
    return true;
}
// Camera at eye looking at target; view space is +X right, +Y up, +Z forward.
inline Mat4 LookAtLH(Vec3 eye, Vec3 target, Vec3 up) {
    const Vec3 forward = Normalize(target - eye, {0.0f, 0.0f, 1.0f});
    Vec3 right = Cross(up, forward);
    right = Normalize(right, std::fabs(forward.y) < 0.99f
        ? Normalize(Cross(Vec3{0.0f, 1.0f, 0.0f}, forward)) : Vec3{1.0f, 0.0f, 0.0f});
    const Vec3 trueUp = Cross(forward, right);
    Mat4 result = Mat4::Identity();
    result.m[0][0] = right.x; result.m[0][1] = right.y; result.m[0][2] = right.z;
    result.m[1][0] = trueUp.x; result.m[1][1] = trueUp.y; result.m[1][2] = trueUp.z;
    result.m[2][0] = forward.x; result.m[2][1] = forward.y; result.m[2][2] = forward.z;
    result.m[0][3] = -Dot(right, eye);
    result.m[1][3] = -Dot(trueUp, eye);
    result.m[2][3] = -Dot(forward, eye);
    return result;
}
// Left-handed perspective, depth mapped to [0, 1], w = view-space z.
inline Mat4 PerspectiveLH(float verticalFovRadians, float aspect, float nearPlane, float farPlane) {
    const float yScale = 1.0f / std::tan(verticalFovRadians * 0.5f);
    const float xScale = yScale / aspect;
    Mat4 result{};
    result.m[0][0] = xScale;
    result.m[1][1] = yScale;
    result.m[2][2] = farPlane / (farPlane - nearPlane);
    result.m[2][3] = -nearPlane * farPlane / (farPlane - nearPlane);
    result.m[3][2] = 1.0f;
    return result;
}
inline Mat4 OrthographicLH(float width, float height, float nearPlane, float farPlane) {
    Mat4 result{};
    result.m[0][0] = 2.0f / width;
    result.m[1][1] = 2.0f / height;
    result.m[2][2] = 1.0f / (farPlane - nearPlane);
    result.m[2][3] = -nearPlane / (farPlane - nearPlane);
    result.m[3][3] = 1.0f;
    return result;
}
inline Mat4 OrthographicOffCenterLH(float left, float right, float bottom, float top,
    float nearPlane, float farPlane) {
    Mat4 result = OrthographicLH(right - left, top - bottom, nearPlane, farPlane);
    result.m[0][3] = -(right + left) / (right - left);
    result.m[1][3] = -(top + bottom) / (top - bottom);
    return result;
}
inline bool NearlyEqual(const Mat4& a, const Mat4& b, float tolerance = 1.0e-4f) {
    for (int row = 0; row < 4; ++row)
        for (int col = 0; col < 4; ++col)
            if (std::fabs(a.m[row][col] - b.m[row][col]) > tolerance) return false;
    return true;
}

// ---------------------------------------------------------------- Quat
inline Quat operator*(Quat a, Quat b) {
    return {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
}
inline float Dot(Quat a, Quat b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
inline Quat Conjugate(Quat q) { return {-q.x, -q.y, -q.z, q.w}; }
inline Quat Normalize(Quat q) {
    const float length = std::sqrt(Dot(q, q));
    if (!(length > kEpsilon) || !IsFinite(length)) return Quat{};
    const float inverse = 1.0f / length;
    return {q.x * inverse, q.y * inverse, q.z * inverse, q.w * inverse};
}
inline Quat Inverse(Quat q) {
    const float lengthSquared = Dot(q, q);
    if (!(lengthSquared > kEpsilon)) return Quat{};
    const Quat c = Conjugate(q);
    return {c.x / lengthSquared, c.y / lengthSquared, c.z / lengthSquared, c.w / lengthSquared};
}
// Rotation of `radians` about `axis`, matching the RotationX/Y/Z matrix senses.
inline Quat QuatFromAxisAngle(Vec3 axis, float radians) {
    const Vec3 unit = Normalize(axis, {0.0f, 1.0f, 0.0f});
    const float half = radians * 0.5f;
    const float s = std::sin(half);
    // Same numeric sense as RotationX/Y/Z: RotationY(+a) and this both take +Z toward +X.
    return {unit.x * s, unit.y * s, unit.z * s, std::cos(half)};
}
inline Vec3 Rotate(Quat q, Vec3 v) {
    // v' = v + 2w(u x v) + 2 u x (u x v), with u the vector part.
    const Vec3 u{q.x, q.y, q.z};
    const Vec3 t = Cross(u, v) * 2.0f;
    return v + t * q.w + Cross(u, t);
}
// Yaw about +Y, then pitch about +X, then roll about +Z (all radians).
inline Quat QuatFromEuler(float yaw, float pitch, float roll) {
    return QuatFromAxisAngle({0.0f, 1.0f, 0.0f}, yaw)
        * QuatFromAxisAngle({1.0f, 0.0f, 0.0f}, pitch)
        * QuatFromAxisAngle({0.0f, 0.0f, 1.0f}, roll);
}
inline Quat Nlerp(Quat a, Quat b, float t) {
    if (Dot(a, b) < 0.0f) b = {-b.x, -b.y, -b.z, -b.w};
    return Normalize({Lerp(a.x, b.x, t), Lerp(a.y, b.y, t), Lerp(a.z, b.z, t), Lerp(a.w, b.w, t)});
}
inline Quat Slerp(Quat a, Quat b, float t) {
    float cosine = Dot(a, b);
    if (cosine < 0.0f) {
        b = {-b.x, -b.y, -b.z, -b.w};
        cosine = -cosine;
    }
    if (cosine > 0.9995f) return Nlerp(a, b, t);
    const float angle = std::acos(Clamp(cosine, -1.0f, 1.0f));
    const float sine = std::sin(angle);
    const float wa = std::sin((1.0f - t) * angle) / sine;
    const float wb = std::sin(t * angle) / sine;
    return Normalize({a.x * wa + b.x * wb, a.y * wa + b.y * wb, a.z * wa + b.z * wb,
        a.w * wa + b.w * wb});
}
inline Mat4 ToMat4(Quat q) {
    const Vec3 x = Rotate(q, {1.0f, 0.0f, 0.0f});
    const Vec3 y = Rotate(q, {0.0f, 1.0f, 0.0f});
    const Vec3 z = Rotate(q, {0.0f, 0.0f, 1.0f});
    Mat4 result = Mat4::Identity();
    result.m[0][0] = x.x; result.m[0][1] = y.x; result.m[0][2] = z.x;
    result.m[1][0] = x.y; result.m[1][1] = y.y; result.m[1][2] = z.y;
    result.m[2][0] = x.z; result.m[2][1] = y.z; result.m[2][2] = z.z;
    return result;
}
// Shortest-arc rotation taking unit vector `from` onto unit vector `to`.
inline Quat QuatFromTo(Vec3 from, Vec3 to) {
    const float cosine = Clamp(Dot(from, to), -1.0f, 1.0f);
    if (cosine > 0.999999f) return Quat{};
    if (cosine < -0.999999f) {
        Vec3 axis = Cross({1.0f, 0.0f, 0.0f}, from);
        if (LengthSquared(axis) < 1.0e-6f) axis = Cross({0.0f, 1.0f, 0.0f}, from);
        return QuatFromAxisAngle(axis, kPi);
    }
    // Rotate(q, from) must equal to; our Rotate is the standard Hamilton form.
    const Vec3 axis = Cross(from, to);
    return Normalize(Quat{axis.x, axis.y, axis.z, 1.0f + cosine});
}
// Yaw-only facing for a horizontal direction (+Z forward is yaw 0).
inline float YawFromDirection(Vec3 direction) { return std::atan2(direction.x, direction.z); }
inline Quat QuatFromYaw(float yaw) { return QuatFromAxisAngle({0.0f, 1.0f, 0.0f}, yaw); }

// ---------------------------------------------------------------- TRS transform
struct TRS {
    Vec3 translation{};
    Quat rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};
inline Mat4 ToMat4(const TRS& t) {
    Mat4 result = ToMat4(t.rotation);
    for (int row = 0; row < 3; ++row) {
        result.m[row][0] *= t.scale.x;
        result.m[row][1] *= t.scale.y;
        result.m[row][2] *= t.scale.z;
    }
    result.m[0][3] = t.translation.x;
    result.m[1][3] = t.translation.y;
    result.m[2][3] = t.translation.z;
    return result;
}
inline Vec3 TransformPoint(const TRS& t, Vec3 p) {
    return t.translation + Rotate(t.rotation, Multiply(t.scale, p));
}
// parent * child (child expressed in parent space). Exact for uniform scale.
inline TRS Combine(const TRS& parent, const TRS& child) {
    TRS result;
    result.rotation = Normalize(parent.rotation * child.rotation);
    result.scale = Multiply(parent.scale, child.scale);
    result.translation = TransformPoint(parent, child.translation);
    return result;
}
inline TRS Lerp(const TRS& a, const TRS& b, float t) {
    return {Lerp(a.translation, b.translation, t), Slerp(a.rotation, b.rotation, t),
        Lerp(a.scale, b.scale, t)};
}

} // namespace Astral::Math
