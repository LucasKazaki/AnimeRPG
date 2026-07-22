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

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
};

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
