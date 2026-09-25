#pragma once

#include <cmath>

namespace Astral::Math {

struct Vec2 {
    float x{};
    float y{};
};

struct Vec3 {
    float x{};
    float y{};
    float z{};

    // hypot avoids intermediate square overflow/underflow for representable norms.
    float Length() const { return std::hypot(x, y, z); }
};

inline bool IsFinite(const Vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

struct Vec4 {
    float x{};
    float y{};
    float z{};
    float w{1.0f};
};

struct Mat4 {
    float m[4][4]{};

    static Mat4 Identity() {
        Mat4 result{};
        for (int i = 0; i < 4; ++i) {
            result.m[i][i] = 1.0f;
        }
        return result;
    }
};

struct Quat {
    float x{};
    float y{};
    float z{};
    float w{1.0f};
};

} // namespace Astral::Math
