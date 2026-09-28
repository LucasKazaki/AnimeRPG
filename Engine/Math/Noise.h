#pragma once

// Deterministic hash-based value noise and fBm for procedural textures, sky
// clouds and VFX. Identical results on every compiler/platform.

#include "Engine/Core/Random.h"
#include "Engine/Math/VectorMath.h"

#include <cmath>
#include <cstdint>

namespace Astral::Math {

inline float HashToUnit(int x, int y, std::uint32_t seed = 0) {
    const std::uint32_t h = Core::Hash32(static_cast<std::uint32_t>(x) * 73856093u
        ^ static_cast<std::uint32_t>(y) * 19349663u ^ seed * 83492791u);
    return static_cast<float>(h & 0xFFFFFFu) / 16777215.0f;
}

inline float ValueNoise2D(float x, float y, std::uint32_t seed = 0) {
    const float fx = std::floor(x), fy = std::floor(y);
    const int ix = static_cast<int>(fx), iy = static_cast<int>(fy);
    const float tx = x - fx, ty = y - fy;
    const float sx = tx * tx * (3.0f - 2.0f * tx);
    const float sy = ty * ty * (3.0f - 2.0f * ty);
    const float a = HashToUnit(ix, iy, seed), b = HashToUnit(ix + 1, iy, seed);
    const float c = HashToUnit(ix, iy + 1, seed), d = HashToUnit(ix + 1, iy + 1, seed);
    return Lerp(Lerp(a, b, sx), Lerp(c, d, sx), sy);
}

// Tileable variant: lattice coordinates wrap with the given period.
inline float TileableValueNoise2D(float x, float y, int period, std::uint32_t seed = 0) {
    const float fx = std::floor(x), fy = std::floor(y);
    auto wrap = [period](int v) { return ((v % period) + period) % period; };
    const int ix = static_cast<int>(fx), iy = static_cast<int>(fy);
    const float tx = x - fx, ty = y - fy;
    const float sx = tx * tx * (3.0f - 2.0f * tx);
    const float sy = ty * ty * (3.0f - 2.0f * ty);
    const float a = HashToUnit(wrap(ix), wrap(iy), seed), b = HashToUnit(wrap(ix + 1), wrap(iy), seed);
    const float c = HashToUnit(wrap(ix), wrap(iy + 1), seed), d = HashToUnit(wrap(ix + 1), wrap(iy + 1), seed);
    return Lerp(Lerp(a, b, sx), Lerp(c, d, sx), sy);
}

inline float Fbm2D(float x, float y, int octaves, std::uint32_t seed = 0) {
    float sum = 0.0f, amplitude = 0.5f, frequency = 1.0f, norm = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum += amplitude * ValueNoise2D(x * frequency, y * frequency, seed + static_cast<std::uint32_t>(i));
        norm += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.03f;
    }
    return norm > 0.0f ? sum / norm : 0.0f;
}

// fBm that tiles over [0, period) when the base frequency is an integer.
inline float TileableFbm2D(float x, float y, int period, int octaves, std::uint32_t seed = 0) {
    float sum = 0.0f, amplitude = 0.5f, norm = 0.0f;
    int frequency = 1;
    for (int i = 0; i < octaves; ++i) {
        sum += amplitude * TileableValueNoise2D(x * static_cast<float>(frequency), y * static_cast<float>(frequency),
            period * frequency, seed + static_cast<std::uint32_t>(i));
        norm += amplitude;
        amplitude *= 0.5f;
        frequency *= 2;
    }
    return norm > 0.0f ? sum / norm : 0.0f;
}

} // namespace Astral::Math
