#include "Engine/Graphics/Texture.h"

#include <algorithm>
#include <cmath>

namespace Astral::Graphics {

using Math::Vec2;
using Math::Vec4;

void Texture2D::FromImage(const ImageRgba8& image, bool srgb, bool generateMips) {
    std::vector<Vec4> texels(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height));
    const auto& decode = SrgbDecodeTable();
    for (std::size_t i = 0; i < texels.size(); ++i) {
        const std::uint8_t* p = &image.pixels[i * 4];
        if (srgb) texels[i] = {decode[p[0]], decode[p[1]], decode[p[2]], p[3] / 255.0f};
        else texels[i] = {p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f, p[3] / 255.0f};
    }
    FromTexels(image.width, image.height, std::move(texels), generateMips);
}

void Texture2D::FromTexels(int width, int height, std::vector<Vec4> texels, bool generateMips) {
    levels_.clear();
    if (width <= 0 || height <= 0
        || texels.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {
        return;
    }
    levels_.push_back({width, height, std::move(texels)});
    if (generateMips) GenerateMips();
}

void Texture2D::GenerateMips() {
    if (levels_.empty()) return;
    levels_.resize(1);
    while (levels_.back().width > 1 || levels_.back().height > 1) {
        const Level& source = levels_.back();
        Level next;
        next.width = std::max(1, source.width / 2);
        next.height = std::max(1, source.height / 2);
        next.texels.resize(static_cast<std::size_t>(next.width) * static_cast<std::size_t>(next.height));
        for (int y = 0; y < next.height; ++y) {
            for (int x = 0; x < next.width; ++x) {
                Vec4 sum{0, 0, 0, 0};
                int samples = 0;
                for (int dy = 0; dy < 2; ++dy) {
                    for (int dx = 0; dx < 2; ++dx) {
                        const int sx = std::min(source.width - 1, x * 2 + dx);
                        const int sy = std::min(source.height - 1, y * 2 + dy);
                        sum = sum + source.texels[static_cast<std::size_t>(sy) * static_cast<std::size_t>(source.width) + static_cast<std::size_t>(sx)];
                        ++samples;
                    }
                }
                next.texels[static_cast<std::size_t>(y) * static_cast<std::size_t>(next.width) + static_cast<std::size_t>(x)] = sum * (1.0f / samples);
            }
        }
        levels_.push_back(std::move(next));
    }
}

std::size_t Texture2D::MemoryBytes() const {
    std::size_t bytes = 0;
    for (const Level& level : levels_) bytes += level.texels.size() * sizeof(Vec4);
    return bytes;
}

Vec4 Texture2D::Fetch(int level, int x, int y) const {
    const Level& l = levels_[static_cast<std::size_t>(level)];
    if (wrap == TextureWrap::Repeat) {
        x %= l.width;
        y %= l.height;
        if (x < 0) x += l.width;
        if (y < 0) y += l.height;
    } else {
        x = std::clamp(x, 0, l.width - 1);
        y = std::clamp(y, 0, l.height - 1);
    }
    return l.texels[static_cast<std::size_t>(y) * static_cast<std::size_t>(l.width) + static_cast<std::size_t>(x)];
}

Vec4 Texture2D::SampleBilinear(Vec2 uv, int level) const {
    if (!Valid()) return {1, 0, 1, 1};
    level = std::clamp(level, 0, LevelCount() - 1);
    const Level& l = levels_[static_cast<std::size_t>(level)];
    if (!std::isfinite(uv.x) || !std::isfinite(uv.y)) uv = {0, 0};
    // Keep coordinates bounded so float->int conversion cannot overflow.
    if (wrap == TextureWrap::Repeat) {
        uv.x -= std::floor(uv.x);
        uv.y -= std::floor(uv.y);
    }
    const float fx = uv.x * static_cast<float>(l.width) - 0.5f;
    const float fy = uv.y * static_cast<float>(l.height) - 0.5f;
    const float bx = std::floor(fx);
    const float by = std::floor(fy);
    const int x0 = static_cast<int>(std::clamp(bx, -2.0f, static_cast<float>(l.width + 1)));
    const int y0 = static_cast<int>(std::clamp(by, -2.0f, static_cast<float>(l.height + 1)));
    const float tx = fx - bx;
    const float ty = fy - by;
    const Vec4 a = Fetch(level, x0, y0);
    const Vec4 b = Fetch(level, x0 + 1, y0);
    const Vec4 c = Fetch(level, x0, y0 + 1);
    const Vec4 d = Fetch(level, x0 + 1, y0 + 1);
    return Math::Lerp(Math::Lerp(a, b, tx), Math::Lerp(c, d, tx), ty);
}

Vec4 Texture2D::Sample(Vec2 uv, float lod) const {
    if (!Valid()) return {1, 0, 1, 1};
    if (!std::isfinite(lod) || lod <= 0.0f) return SampleBilinear(uv, 0);
    const float maxLevel = static_cast<float>(LevelCount() - 1);
    lod = std::min(lod, maxLevel);
    const int level = static_cast<int>(lod);
    const float fraction = lod - static_cast<float>(level);
    const Vec4 fine = SampleBilinear(uv, level);
    if (fraction < 0.01f || level + 1 >= LevelCount()) return fine;
    return Math::Lerp(fine, SampleBilinear(uv, level + 1), fraction);
}

} // namespace Astral::Graphics
