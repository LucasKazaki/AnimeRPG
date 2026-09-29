#pragma once

// 8-bit RGBA images and a dependency-free PNG codec (zlib deflate encoder with
// LZ77 + fixed Huffman, full inflate decoder). The decoder accepts the common
// non-interlaced 8-bit gray/gray-alpha/RGB/RGBA/palette formats used by the
// starter content and rejects everything else with an error, never UB.

#include "Engine/Graphics/Color.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Astral::Graphics {

struct ImageRgba8 {
    int width{};
    int height{};
    std::vector<std::uint8_t> pixels; // row-major RGBA, top row first

    void Resize(int newWidth, int newHeight, Rgba8 fill = {0, 0, 0, 255});
    bool Contains(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
    Rgba8 Get(int x, int y) const;
    void Set(int x, int y, Rgba8 value);
    // Source-over blend in sRGB space (adequate for UI overlays).
    void Blend(int x, int y, Rgba8 value, float alpha);
};

struct PngLimits {
    std::size_t maxFileBytes{64u * 1024u * 1024u};
    int maxDimension{16384};
    std::size_t maxPixels{64u * 1024u * 1024u};
};

std::vector<std::uint8_t> EncodePng(const ImageRgba8& image, bool includeAlpha = false);
bool WritePng(const std::string& path, const ImageRgba8& image, std::string& error, bool includeAlpha = false);
bool DecodePng(const std::uint8_t* data, std::size_t size, ImageRgba8& out, std::string& error,
    const PngLimits& limits = {});
bool ReadPng(const std::string& path, ImageRgba8& out, std::string& error, const PngLimits& limits = {});

// zlib (RFC 1950/1951) helpers, exposed for tests and future asset formats.
std::vector<std::uint8_t> ZlibCompress(const std::uint8_t* data, std::size_t size);
bool ZlibDecompress(const std::uint8_t* data, std::size_t size, std::vector<std::uint8_t>& out,
    std::size_t maxOutputBytes, std::string& error);
std::uint32_t Crc32(const std::uint8_t* data, std::size_t size, std::uint32_t crc = 0);

// FNV-1a over dimensions and pixels, for capture receipts.
std::uint64_t HashImage(const ImageRgba8& image);

} // namespace Astral::Graphics
