#include "Engine/Graphics/RenderTarget.h"

#include <algorithm>

namespace Astral::Graphics {

void RenderTarget::Resize(int width, int height) {
    width = std::max(1, width);
    height = std::max(1, height);
    if (width == width_ && height == height_) return;
    width_ = width;
    height_ = height;
    const std::size_t count = PixelCount();
    depth.assign(count, 1.0f);
    visibility.assign(count, 0);
    barycentric.assign(count, {});
    hdr.assign(count, {});
    normal.assign(count, {});
    linearDepth.assign(count, 0.0f);
    objectId.assign(count, 0);
    flags.assign(count, 0);
    reflectivity.assign(count, 0.0f);
    output.Resize(width, height);
}

std::size_t RenderTarget::MemoryBytes() const {
    return depth.size() * sizeof(float) + visibility.size() * sizeof(std::uint32_t)
        + barycentric.size() * sizeof(Math::Vec2) + hdr.size() * sizeof(Color)
        + normal.size() * sizeof(Math::Vec3) + linearDepth.size() * sizeof(float)
        + objectId.size() * sizeof(std::uint32_t) + flags.size() + reflectivity.size() * sizeof(float)
        + output.pixels.size();
}

} // namespace Astral::Graphics
