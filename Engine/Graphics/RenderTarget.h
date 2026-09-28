#pragma once

// Frame buffers of the software renderer. The visibility buffer (triangle id +
// perspective-correct barycentrics per pixel) is resolved by the shading pass,
// so every visible pixel is shaded exactly once regardless of overdraw.

#include "Engine/Graphics/Color.h"
#include "Engine/Graphics/Image.h"

#include <cstdint>
#include <vector>

namespace Astral::Graphics {

enum PixelFlags : std::uint8_t {
    kPixelOutline = 1u << 0,
    kPixelWater = 1u << 1,
    kPixelSky = 1u << 2,
};

class RenderTarget {
public:
    void Resize(int width, int height);
    int Width() const { return width_; }
    int Height() const { return height_; }
    std::size_t PixelCount() const { return static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_); }
    std::size_t Index(int x, int y) const { return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x); }
    std::size_t MemoryBytes() const;

    std::vector<float> depth;               // NDC depth, 1 = far/empty
    std::vector<std::uint32_t> visibility;  // setup-triangle index + 1, 0 = empty
    std::vector<Math::Vec2> barycentric;    // perspective-correct (b1, b2)
    std::vector<Color> hdr;                 // linear scene radiance
    std::vector<Math::Vec3> normal;         // world normal of the shaded surface
    std::vector<float> linearDepth;         // view-space z, very large for sky
    std::vector<std::uint32_t> objectId;
    std::vector<std::uint8_t> flags;        // PixelFlags
    std::vector<float> reflectivity;        // water Fresnel weight
    ImageRgba8 output;                      // final sRGB image

private:
    int width_{};
    int height_{};
};

} // namespace Astral::Graphics
