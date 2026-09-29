#include "Engine/Graphics/ProceduralTextures.h"

#include "Engine/Math/Noise.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

namespace Astral::Graphics::Procedural {

using namespace Math;

namespace {
Texture2D Build(int width, int height, const char* name, const std::function<Vec4(int, int)>& texel) {
    std::vector<Vec4> texels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) texels[static_cast<std::size_t>(y * width + x)] = texel(x, y);
    Texture2D texture;
    texture.name = name;
    texture.FromTexels(width, height, std::move(texels));
    return texture;
}
Vec4 Opaque(Color c) { return {c.x, c.y, c.z, 1.0f}; }
} // namespace

Texture2D Checker(int size, int cells, Color a, Color b) {
    cells = std::max(1, cells);
    return Build(size, size, "checker", [&](int x, int y) {
        const int cx = x * cells / size, cy = y * cells / size;
        return Opaque(((cx + cy) & 1) ? b : a);
    });
}

Texture2D Marble(int size, Color base, Color vein, std::uint32_t seed) {
    const int period = 4;
    return Build(size, size, "marble", [&](int x, int y) {
        const float u = static_cast<float>(x) / static_cast<float>(size) * period;
        const float v = static_cast<float>(y) / static_cast<float>(size) * period;
        const float warp = TileableFbm2D(u, v, period, 5, seed);
        // Veins: thin bands where a warped sine crosses zero.
        const float bands = std::fabs(std::sin((u + v * 0.6f + warp * 3.2f) * kPi));
        const float veins = std::pow(1.0f - bands, 10.0f) * 0.55f;
        const float mottling = (TileableFbm2D(u * 2.0f, v * 2.0f, period * 2, 3, seed + 9) - 0.5f) * 0.08f;
        return Opaque(Lerp(base, vein, Saturate(veins)) * (1.0f + mottling));
    });
}

Texture2D Grass(int size, Color light, Color dark, std::uint32_t seed) {
    const int period = 8;
    return Build(size, size, "grass", [&](int x, int y) {
        const float u = static_cast<float>(x) / static_cast<float>(size) * period;
        const float v = static_cast<float>(y) / static_cast<float>(size) * period;
        const float clumps = TileableFbm2D(u, v, period, 4, seed);
        const float blades = TileableValueNoise2D(u * 8.0f, v * 8.0f, period * 8, seed + 5);
        const float stripe = (y * 2 / size) & 1 ? 0.04f : -0.04f; // mowing stripes
        const float t = Saturate(clumps * 0.8f + blades * 0.3f + stripe);
        return Opaque(Lerp(dark, light, t));
    });
}

Texture2D Gravel(int size, Color base, std::uint32_t seed) {
    const int period = 16;
    return Build(size, size, "gravel", [&](int x, int y) {
        const float u = static_cast<float>(x) / static_cast<float>(size) * period;
        const float v = static_cast<float>(y) / static_cast<float>(size) * period;
        const float pebbles = TileableValueNoise2D(u * 4.0f, v * 4.0f, period * 4, seed);
        const float large = TileableFbm2D(u * 0.5f, v * 0.5f, period / 2, 3, seed + 3);
        return Opaque(base * (0.82f + pebbles * 0.28f + (large - 0.5f) * 0.12f));
    });
}

Texture2D Paving(int size, int tilesX, int tilesY, Color stone, Color grout, std::uint32_t seed) {
    tilesX = std::max(1, tilesX);
    tilesY = std::max(1, tilesY);
    return Build(size, size, "paving", [&](int x, int y) {
        const float fx = static_cast<float>(x) / static_cast<float>(size) * static_cast<float>(tilesX);
        const float fy = static_cast<float>(y) / static_cast<float>(size) * static_cast<float>(tilesY);
        const int row = static_cast<int>(fy);
        const float offsetX = (row & 1) ? 0.5f : 0.0f; // running bond
        const float gx = fx + offsetX;
        const float cellX = gx - std::floor(gx), cellY = fy - std::floor(fy);
        const float edge = std::min(std::min(cellX, 1.0f - cellX) * 3.0f, std::min(cellY, 1.0f - cellY) * 3.0f);
        const int tileId = static_cast<int>(std::floor(gx)) + row * 131;
        const float tone = 0.9f + HashToUnit(tileId, row, seed) * 0.18f;
        const float grain = (TileableValueNoise2D(fx * 6.0f, fy * 6.0f, tilesX * 6, seed + 1) - 0.5f) * 0.1f;
        const float mortar = SmoothStep(0.04f, 0.09f, edge);
        return Opaque(Lerp(grout, stone * (tone + grain), mortar));
    });
}

Texture2D SoftDisc(int size) {
    return Build(size, size, "soft_disc", [&](int x, int y) {
        const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(size) * 2.0f - 1.0f;
        const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(size) * 2.0f - 1.0f;
        const float falloff = Saturate(1.0f - (u * u + v * v));
        return Vec4{1.0f, 1.0f, 1.0f, falloff * falloff};
    });
}

Texture2D Gradient(int width, Color left, Color right) {
    return Build(width, 1, "gradient", [&](int x, int) {
        return Opaque(Lerp(left, right, static_cast<float>(x) / static_cast<float>(std::max(1, width - 1))));
    });
}

} // namespace Astral::Graphics::Procedural
