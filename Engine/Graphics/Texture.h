#pragma once

// Linear-float RGBA textures with a full mip chain and trilinear sampling.

#include "Engine/Graphics/Image.h"
#include "Engine/Math/VectorMath.h"

#include <string>
#include <vector>

namespace Astral::Graphics {

enum class TextureWrap : std::uint8_t { Repeat, Clamp };

class Texture2D {
public:
    struct Level {
        int width{};
        int height{};
        std::vector<Math::Vec4> texels; // linear RGBA
    };

    // `srgb` decodes colour channels (alpha is always linear).
    void FromImage(const ImageRgba8& image, bool srgb = true, bool generateMips = true);
    void FromTexels(int width, int height, std::vector<Math::Vec4> texels, bool generateMips = true);
    void GenerateMips();

    bool Valid() const { return !levels_.empty() && levels_[0].width > 0; }
    int Width() const { return Valid() ? levels_[0].width : 0; }
    int Height() const { return Valid() ? levels_[0].height : 0; }
    int LevelCount() const { return static_cast<int>(levels_.size()); }
    const Level& GetLevel(int index) const { return levels_[static_cast<std::size_t>(index)]; }
    std::size_t MemoryBytes() const;

    Math::Vec4 Fetch(int level, int x, int y) const;
    Math::Vec4 SampleBilinear(Math::Vec2 uv, int level) const;
    // lod = log2(texels per pixel); negative values magnify.
    Math::Vec4 Sample(Math::Vec2 uv, float lod) const;

    TextureWrap wrap{TextureWrap::Repeat};
    std::string name;

private:
    std::vector<Level> levels_;
};

} // namespace Astral::Graphics
