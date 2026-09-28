#include "Engine/Graphics/Rasterizer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace Astral::Graphics {

using Math::Vec4;

namespace {

float PlaneDistance(const Vec4& p, int plane) {
    switch (plane) {
    case 0: return p.w + p.x;
    case 1: return p.w - p.x;
    case 2: return p.w + p.y;
    case 3: return p.w - p.y;
    case 4: return p.z;
    default: return p.w - p.z;
    }
}

ClipVertex LerpVertex(const ClipVertex& a, const ClipVertex& b, float t) {
    ClipVertex r;
    r.clip = Math::Lerp(a.clip, b.clip, t);
    r.world = Math::Lerp(a.world, b.world, t);
    r.normal = Math::Lerp(a.normal, b.normal, t);
    r.uv = Math::Lerp(a.uv, b.uv, t);
    r.color = Math::Lerp(a.color, b.color, t);
    return r;
}

constexpr int kMaxPolygon = 9; // 3 + one per clip plane

void SetupScreenTriangle(const ClipVertex& a, const ClipVertex& b, const ClipVertex& c, CullMode cull,
    int width, int height, std::uint32_t drawIndex, std::vector<SetupTriangle>& out) {
    SetupTriangle t;
    t.v[0] = a;
    t.v[1] = b;
    t.v[2] = c;
    float sx[3], sy[3];
    for (int i = 0; i < 3; ++i) {
        const Vec4& p = t.v[i].clip;
        if (!(p.w > 0.0f) || !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) return;
        const float invW = 1.0f / p.w;
        sx[i] = (p.x * invW * 0.5f + 0.5f) * static_cast<float>(width);
        sy[i] = (0.5f - p.y * invW * 0.5f) * static_cast<float>(height);
        t.z[i] = Math::Saturate(p.z * invW);
        t.invW[i] = invW;
        t.x[i] = static_cast<std::int32_t>(std::lround(sx[i] * kSubpixelScale));
        t.y[i] = static_cast<std::int32_t>(std::lround(sy[i] * kSubpixelScale));
    }
    std::int64_t area2 = (static_cast<std::int64_t>(t.x[1]) - t.x[0]) * (static_cast<std::int64_t>(t.y[2]) - t.y[0])
        - (static_cast<std::int64_t>(t.y[1]) - t.y[0]) * (static_cast<std::int64_t>(t.x[2]) - t.x[0]);
    if (area2 == 0) return;
    const bool front = area2 > 0;
    if ((cull == CullMode::Back && !front) || (cull == CullMode::Front && front)) return;
    if (!front) {
        std::swap(t.v[1], t.v[2]);
        std::swap(t.x[1], t.x[2]);
        std::swap(t.y[1], t.y[2]);
        std::swap(t.z[1], t.z[2]);
        std::swap(t.invW[1], t.invW[2]);
        std::swap(sx[1], sx[2]);
        std::swap(sy[1], sy[2]);
        area2 = -area2;
        t.backFacing = true;
    }
    t.area2 = area2;
    t.drawIndex = drawIndex;

    const int minFixedX = std::min({t.x[0], t.x[1], t.x[2]});
    const int maxFixedX = std::max({t.x[0], t.x[1], t.x[2]});
    const int minFixedY = std::min({t.y[0], t.y[1], t.y[2]});
    const int maxFixedY = std::max({t.y[0], t.y[1], t.y[2]});
    const float half = static_cast<float>(kSubpixelScale / 2);
    t.minX = std::max(0, static_cast<int>(std::ceil((static_cast<float>(minFixedX) - half) / kSubpixelScale)));
    t.maxX = std::min(width - 1, static_cast<int>(std::floor((static_cast<float>(maxFixedX) - half) / kSubpixelScale)));
    t.minY = std::max(0, static_cast<int>(std::ceil((static_cast<float>(minFixedY) - half) / kSubpixelScale)));
    t.maxY = std::min(height - 1, static_cast<int>(std::floor((static_cast<float>(maxFixedY) - half) / kSubpixelScale)));
    if (t.minX > t.maxX || t.minY > t.maxY) return; // covers no pixel centre

    // Plane gradients of attribute/w in screen space for texture LOD selection.
    const float d = (sx[1] - sx[0]) * (sy[2] - sy[0]) - (sx[2] - sx[0]) * (sy[1] - sy[0]);
    if (std::fabs(d) > 1.0e-12f) {
        const float inverse = 1.0f / d;
        auto gradient = [&](float a0, float a1, float a2, float& ddx, float& ddy) {
            ddx = ((a1 - a0) * (sy[2] - sy[0]) - (a2 - a0) * (sy[1] - sy[0])) * inverse;
            ddy = ((a2 - a0) * (sx[1] - sx[0]) - (a1 - a0) * (sx[2] - sx[0])) * inverse;
        };
        gradient(t.v[0].uv.x * t.invW[0], t.v[1].uv.x * t.invW[1], t.v[2].uv.x * t.invW[2], t.uOverWdx, t.uOverWdy);
        gradient(t.v[0].uv.y * t.invW[0], t.v[1].uv.y * t.invW[1], t.v[2].uv.y * t.invW[2], t.vOverWdx, t.vOverWdy);
        gradient(t.invW[0], t.invW[1], t.invW[2], t.invWdx, t.invWdy);
    }
    out.push_back(t);
}

} // namespace

void ClipAndSetupTriangle(const ClipVertex& a, const ClipVertex& b, const ClipVertex& c, CullMode cull,
    int width, int height, std::uint32_t drawIndex, std::vector<SetupTriangle>& out) {
    if (width <= 0 || height <= 0) return;
    // Trivial accept/reject using outcodes.
    unsigned outside[3] = {0, 0, 0};
    const ClipVertex* input[3] = {&a, &b, &c};
    for (int i = 0; i < 3; ++i) {
        for (int plane = 0; plane < 6; ++plane) {
            if (PlaneDistance(input[i]->clip, plane) < 0.0f) outside[i] |= 1u << plane;
        }
    }
    if (outside[0] & outside[1] & outside[2]) return;
    if ((outside[0] | outside[1] | outside[2]) == 0) {
        SetupScreenTriangle(a, b, c, cull, width, height, drawIndex, out);
        return;
    }

    ClipVertex polygon[2][kMaxPolygon];
    int count = 3;
    polygon[0][0] = a;
    polygon[0][1] = b;
    polygon[0][2] = c;
    int current = 0;
    const unsigned combined = outside[0] | outside[1] | outside[2];
    for (int plane = 0; plane < 6 && count > 0; ++plane) {
        if (!(combined & (1u << plane))) continue;
        const ClipVertex* source = polygon[current];
        ClipVertex* destination = polygon[1 - current];
        int produced = 0;
        for (int i = 0; i < count; ++i) {
            const ClipVertex& p = source[i];
            const ClipVertex& q = source[(i + 1) % count];
            const float dp = PlaneDistance(p.clip, plane);
            const float dq = PlaneDistance(q.clip, plane);
            if (dp >= 0.0f && produced < kMaxPolygon) destination[produced++] = p;
            if ((dp >= 0.0f) != (dq >= 0.0f) && produced < kMaxPolygon) {
                destination[produced++] = LerpVertex(p, q, dp / (dp - dq));
            }
        }
        count = produced;
        current = 1 - current;
    }
    for (int i = 1; i + 1 < count; ++i) {
        SetupScreenTriangle(polygon[current][0], polygon[current][i], polygon[current][i + 1], cull, width,
            height, drawIndex, out);
    }
}

bool ClipSegment(Vec4& a, Vec4& b) {
    for (int plane = 4; plane < 6; ++plane) {
        const float da = PlaneDistance(a, plane);
        const float db = PlaneDistance(b, plane);
        if (da < 0.0f && db < 0.0f) return false;
        if (da < 0.0f) a = Math::Lerp(a, b, da / (da - db));
        else if (db < 0.0f) b = Math::Lerp(b, a, db / (db - da));
    }
    return a.w > 0.0f && b.w > 0.0f;
}

void TileBins::Build(const std::vector<SetupTriangle>& triangles, int width, int height, int tile) {
    tileSize = std::max(8, tile);
    tilesX = (std::max(0, width) + tileSize - 1) / tileSize;
    tilesY = (std::max(0, height) + tileSize - 1) / tileSize;
    bins.resize(static_cast<std::size_t>(tilesX) * static_cast<std::size_t>(tilesY));
    for (auto& bin : bins) bin.clear();
    for (std::size_t index = 0; index < triangles.size(); ++index) {
        const SetupTriangle& t = triangles[index];
        const int tx0 = t.minX / tileSize, tx1 = t.maxX / tileSize;
        const int ty0 = t.minY / tileSize, ty1 = t.maxY / tileSize;
        for (int ty = ty0; ty <= ty1; ++ty) {
            for (int tx = tx0; tx <= tx1; ++tx) {
                bins[static_cast<std::size_t>(ty) * static_cast<std::size_t>(tilesX) + static_cast<std::size_t>(tx)]
                    .push_back(static_cast<std::uint32_t>(index));
            }
        }
    }
}

} // namespace Astral::Graphics
