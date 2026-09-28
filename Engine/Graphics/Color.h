#pragma once

// Linear-light colour is carried in Math::Vec3 (r, g, b = x, y, z) so all the
// vector operators apply. 8-bit images are sRGB-encoded unless stated otherwise.

#include "Engine/Math/VectorMath.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace Astral::Graphics {

using Color = Math::Vec3;

struct Rgba8 {
    std::uint8_t r{}, g{}, b{}, a{255};
};

inline float SrgbToLinear(float c) {
    return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}
inline float LinearToSrgb(float c) {
    c = Math::Saturate(c);
    return c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
}

// 8-bit sRGB -> linear lookup.
inline const std::array<float, 256>& SrgbDecodeTable() {
    static const std::array<float, 256> table = [] {
        std::array<float, 256> values{};
        for (int i = 0; i < 256; ++i) values[static_cast<std::size_t>(i)] = SrgbToLinear(i / 255.0f);
        return values;
    }();
    return table;
}

// Linear [0,1] -> 8-bit sRGB via a 4096-entry table (max error < 0.5 LSB).
inline std::uint8_t EncodeSrgb8(float linear) {
    static const std::array<std::uint8_t, 4097> table = [] {
        std::array<std::uint8_t, 4097> values{};
        for (int i = 0; i <= 4096; ++i) {
            values[static_cast<std::size_t>(i)] =
                static_cast<std::uint8_t>(std::lround(LinearToSrgb(i / 4096.0f) * 255.0f));
        }
        return values;
    }();
    if (!(linear > 0.0f)) return 0;
    if (linear >= 1.0f) return 255;
    return table[static_cast<std::size_t>(linear * 4096.0f + 0.5f)];
}

// Convenience for authoring colours from familiar sRGB hex/byte values.
inline Color FromSrgb8(int r, int g, int b) {
    const auto& table = SrgbDecodeTable();
    return {table[static_cast<std::size_t>(r & 255)], table[static_cast<std::size_t>(g & 255)],
        table[static_cast<std::size_t>(b & 255)]};
}

inline float Luminance(Color c) { return 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z; }

inline Color Saturation(Color c, float amount) {
    const float luma = Luminance(c);
    return Math::Lerp(Color{luma, luma, luma}, c, amount);
}

} // namespace Astral::Graphics
