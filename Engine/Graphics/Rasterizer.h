#pragma once

// Tile-binned triangle rasterizer: homogeneous clipping against all six frustum
// planes, 28.4 fixed-point edge functions with the Direct3D top-left fill rule
// (so shared edges are covered exactly once), and perspective-correct
// barycentrics. Tiles are independent, so they are shaded in parallel without
// locks, and each tile preserves submission order for deterministic blending.

#include "Engine/Math/VectorMath.h"

#include <cstdint>
#include <vector>

namespace Astral::Graphics {

struct ClipVertex {
    Math::Vec4 clip{};   // clip-space position
    Math::Vec3 world{};  // world-space position
    Math::Vec3 normal{}; // world-space normal
    Math::Vec2 uv{};
    Math::Vec4 color{1, 1, 1, 1};
};

enum class CullMode : std::uint8_t { Back, Front, None };

struct SetupTriangle {
    ClipVertex v[3];
    std::int32_t x[3]{}; // screen position, 28.4 fixed point
    std::int32_t y[3]{};
    float z[3]{};        // NDC depth in [0, 1]
    float invW[3]{};
    std::int64_t area2{}; // twice the fixed-point area, always > 0 after setup
    int minX{}, minY{}, maxX{}, maxY{}; // inclusive pixel bounds, clamped to the viewport
    std::uint32_t drawIndex{};
    bool backFacing{}; // true when a double-sided back face was re-wound
    // Screen-space gradients of u/w, v/w and 1/w (for texture LOD).
    float uOverWdx{}, uOverWdy{}, vOverWdx{}, vOverWdy{}, invWdx{}, invWdy{};
};

inline constexpr int kSubpixelBits = 4;
inline constexpr int kSubpixelScale = 1 << kSubpixelBits;

// Clips one triangle, culls it and appends 0..7 set-up triangles.
void ClipAndSetupTriangle(const ClipVertex& a, const ClipVertex& b, const ClipVertex& c, CullMode cull,
    int width, int height, std::uint32_t drawIndex, std::vector<SetupTriangle>& out);

// Clips a clip-space segment to the near/far planes; false when fully outside.
bool ClipSegment(Math::Vec4& a, Math::Vec4& b);

struct TileBins {
    int tileSize{64};
    int tilesX{};
    int tilesY{};
    std::vector<std::vector<std::uint32_t>> bins; // triangle indices per tile, in submission order

    void Build(const std::vector<SetupTriangle>& triangles, int width, int height, int tile);
    int TileCount() const { return tilesX * tilesY; }
};

// Calls fn(x, y, z, l0, l1, l2) for every covered pixel centre of `t` inside the
// half-open rectangle [x0, x1) x [y0, y1). l* are screen-linear barycentrics.
template <typename Fn>
inline void RasterizeTriangle(const SetupTriangle& t, int x0, int y0, int x1, int y1, Fn&& fn) {
    const int minX = t.minX > x0 ? t.minX : x0;
    const int minY = t.minY > y0 ? t.minY : y0;
    const int maxX = t.maxX < x1 - 1 ? t.maxX : x1 - 1;
    const int maxY = t.maxY < y1 - 1 ? t.maxY : y1 - 1;
    if (minX > maxX || minY > maxY) return;

    // Edge i is opposite vertex i: e0 = v1->v2, e1 = v2->v0, e2 = v0->v1.
    std::int64_t dx[3], dy[3], bias[3], rowStart[3];
    const int from[3] = {1, 2, 0};
    const int to[3] = {2, 0, 1};
    const std::int64_t px = static_cast<std::int64_t>(minX) * kSubpixelScale + kSubpixelScale / 2;
    const std::int64_t py = static_cast<std::int64_t>(minY) * kSubpixelScale + kSubpixelScale / 2;
    for (int e = 0; e < 3; ++e) {
        const std::int64_t ax = t.x[from[e]], ay = t.y[from[e]];
        dx[e] = static_cast<std::int64_t>(t.x[to[e]]) - ax;
        dy[e] = static_cast<std::int64_t>(t.y[to[e]]) - ay;
        const bool topLeft = dy[e] < 0 || (dy[e] == 0 && dx[e] > 0);
        bias[e] = topLeft ? 0 : -1;
        rowStart[e] = dx[e] * (py - ay) - dy[e] * (px - ax);
    }
    const float inverseArea = 1.0f / static_cast<float>(t.area2);
    const std::int64_t stepX[3] = {-dy[0] * kSubpixelScale, -dy[1] * kSubpixelScale, -dy[2] * kSubpixelScale};
    const std::int64_t stepY[3] = {dx[0] * kSubpixelScale, dx[1] * kSubpixelScale, dx[2] * kSubpixelScale};
    for (int y = minY; y <= maxY; ++y) {
        std::int64_t e0 = rowStart[0], e1 = rowStart[1], e2 = rowStart[2];
        for (int x = minX; x <= maxX; ++x) {
            if ((e0 + bias[0]) >= 0 && (e1 + bias[1]) >= 0 && (e2 + bias[2]) >= 0) {
                const float l0 = static_cast<float>(e0) * inverseArea;
                const float l1 = static_cast<float>(e1) * inverseArea;
                const float l2 = 1.0f - l0 - l1;
                const float z = l0 * t.z[0] + l1 * t.z[1] + l2 * t.z[2];
                fn(x, y, z, l0, l1, l2);
            }
            e0 += stepX[0];
            e1 += stepX[1];
            e2 += stepX[2];
        }
        rowStart[0] += stepY[0];
        rowStart[1] += stepY[1];
        rowStart[2] += stepY[2];
    }
}

// Perspective-correct barycentrics from screen-linear ones.
inline void PerspectiveCorrect(const SetupTriangle& t, float l0, float l1, float l2, float& b0, float& b1, float& b2) {
    const float p0 = l0 * t.invW[0], p1 = l1 * t.invW[1], p2 = l2 * t.invW[2];
    const float sum = p0 + p1 + p2;
    const float inverse = sum > 0.0f ? 1.0f / sum : 0.0f;
    b0 = p0 * inverse;
    b1 = p1 * inverse;
    b2 = p2 * inverse;
}

inline ClipVertex InterpolateVertex(const SetupTriangle& t, float b0, float b1, float b2) {
    ClipVertex r;
    const ClipVertex& a = t.v[0];
    const ClipVertex& b = t.v[1];
    const ClipVertex& c = t.v[2];
    r.world = a.world * b0 + b.world * b1 + c.world * b2;
    r.normal = a.normal * b0 + b.normal * b1 + c.normal * b2;
    r.uv = a.uv * b0 + b.uv * b1 + c.uv * b2;
    r.color = a.color * b0 + b.color * b1 + c.color * b2;
    return r;
}

} // namespace Astral::Graphics
