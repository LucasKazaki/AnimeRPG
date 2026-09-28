#include "Engine/Graphics/Sprite2D.h"

#include "Engine/Core/Profiler.h"

#include <algorithm>
#include <cmath>

namespace Astral::Graphics {

using Math::Vec2;
using Math::Vec4;

Vec2 Camera2D::WorldToScreen(Vec2 world) const {
    const Vec2 relative = world - position;
    const float c = std::cos(-rotation), s = std::sin(-rotation);
    const Vec2 rotated{relative.x * c - relative.y * s, relative.x * s + relative.y * c};
    return {static_cast<float>(viewportWidth) * 0.5f + rotated.x * zoom,
        static_cast<float>(viewportHeight) * 0.5f - rotated.y * zoom};
}

Vec2 Camera2D::ScreenToWorld(Vec2 screen) const {
    const float safeZoom = zoom != 0.0f ? zoom : 1.0f;
    const Vec2 rotated{(screen.x - static_cast<float>(viewportWidth) * 0.5f) / safeZoom,
        (static_cast<float>(viewportHeight) * 0.5f - screen.y) / safeZoom};
    const float c = std::cos(rotation), s = std::sin(rotation);
    return position + Vec2{rotated.x * c - rotated.y * s, rotated.x * s + rotated.y * c};
}

void Camera2D::VisibleBounds(Vec2& minimum, Vec2& maximum) const {
    const Vec2 corners[4] = {ScreenToWorld({0, 0}), ScreenToWorld({static_cast<float>(viewportWidth), 0}),
        ScreenToWorld({0, static_cast<float>(viewportHeight)}),
        ScreenToWorld({static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)})};
    minimum = maximum = corners[0];
    for (const Vec2& c : corners) {
        minimum = {std::min(minimum.x, c.x), std::min(minimum.y, c.y)};
        maximum = {std::max(maximum.x, c.x), std::max(maximum.y, c.y)};
    }
}

void SpriteBatch::Begin(const Camera2D& camera) {
    camera_ = camera;
    sprites_.clear();
}

void SpriteBatch::Draw(const Sprite& sprite) { sprites_.push_back(sprite); }

void SpriteBatch::End(ImageRgba8& target, Core::JobSystem* jobs) {
    ASTRAL_PROFILE_SCOPE("Sprite2D.End");
    std::stable_sort(sprites_.begin(), sprites_.end(), [](const Sprite& a, const Sprite& b) { return a.layer < b.layer; });
    struct Prepared {
        const Sprite* sprite;
        Vec2 origin, axisU, axisV; // screen = origin + axisU * lx + axisV * ly
        float inverse[4];
        int minX, minY, maxX, maxY;
    };
    std::vector<Prepared> prepared;
    prepared.reserve(sprites_.size());
    for (const Sprite& sprite : sprites_) {
        if (sprite.size.x == 0.0f || sprite.size.y == 0.0f || sprite.tint.w <= 0.0f) continue;
        const float c = std::cos(sprite.rotation), s = std::sin(sprite.rotation);
        auto worldCorner = [&](float lx, float ly) {
            const Vec2 local{(lx - sprite.pivot.x) * sprite.size.x, (ly - sprite.pivot.y) * sprite.size.y};
            return sprite.position + Vec2{local.x * c - local.y * s, local.x * s + local.y * c};
        };
        const Vec2 o = camera_.WorldToScreen(worldCorner(0, 0));
        const Vec2 u = camera_.WorldToScreen(worldCorner(1, 0)) - o;
        const Vec2 v = camera_.WorldToScreen(worldCorner(0, 1)) - o;
        const float determinant = u.x * v.y - u.y * v.x;
        if (std::fabs(determinant) < 1.0e-8f || !std::isfinite(determinant)) continue;
        Prepared p{&sprite, o, u, v, {v.y / determinant, -v.x / determinant, -u.y / determinant, u.x / determinant}, 0, 0, 0, 0};
        const Vec2 corners[4] = {o, o + u, o + v, o + u + v};
        float minX = corners[0].x, maxX = corners[0].x, minY = corners[0].y, maxY = corners[0].y;
        for (const Vec2& corner : corners) {
            minX = std::min(minX, corner.x);
            maxX = std::max(maxX, corner.x);
            minY = std::min(minY, corner.y);
            maxY = std::max(maxY, corner.y);
        }
        p.minX = std::max(0, static_cast<int>(std::floor(minX)));
        p.minY = std::max(0, static_cast<int>(std::floor(minY)));
        p.maxX = std::min(target.width - 1, static_cast<int>(std::ceil(maxX)));
        p.maxY = std::min(target.height - 1, static_cast<int>(std::ceil(maxY)));
        if (p.minX > p.maxX || p.minY > p.maxY) continue;
        prepared.push_back(p);
    }
    lastDrawn_ = prepared.size();
    const auto& decode = SrgbDecodeTable();
    constexpr int kStrip = 16;
    const int strips = (target.height + kStrip - 1) / kStrip;
    auto drawStrips = [&](std::size_t begin, std::size_t end) {
        for (std::size_t strip = begin; strip < end; ++strip) {
            const int y0 = static_cast<int>(strip) * kStrip;
            const int y1 = std::min(target.height, y0 + kStrip);
            for (const Prepared& p : prepared) {
                const Sprite& sprite = *p.sprite;
                const int rowBegin = std::max(y0, p.minY), rowEnd = std::min(y1 - 1, p.maxY);
                for (int y = rowBegin; y <= rowEnd; ++y) {
                    for (int x = p.minX; x <= p.maxX; ++x) {
                        const float dx = static_cast<float>(x) + 0.5f - p.origin.x;
                        const float dy = static_cast<float>(y) + 0.5f - p.origin.y;
                        const float lx = p.inverse[0] * dx + p.inverse[1] * dy;
                        const float ly = p.inverse[2] * dx + p.inverse[3] * dy;
                        if (lx < 0.0f || lx >= 1.0f || ly < 0.0f || ly >= 1.0f) continue;
                        Vec4 color = sprite.tint;
                        if (sprite.texture && sprite.texture->Valid()) {
                            const float su = sprite.flipX ? 1.0f - lx : lx;
                            const Vec2 uv{Math::Lerp(sprite.uvMin.x, sprite.uvMax.x, su), Math::Lerp(sprite.uvMin.y, sprite.uvMax.y, 1.0f - ly)};
                            Vec4 texel;
                            if (sprite.pointSampling) {
                                const int tx = std::clamp(static_cast<int>(uv.x * static_cast<float>(sprite.texture->Width())), 0, sprite.texture->Width() - 1);
                                const int ty = std::clamp(static_cast<int>(uv.y * static_cast<float>(sprite.texture->Height())), 0, sprite.texture->Height() - 1);
                                texel = sprite.texture->Fetch(0, tx, ty);
                            } else {
                                texel = sprite.texture->SampleBilinear(uv, 0);
                            }
                            color = {color.x * texel.x, color.y * texel.y, color.z * texel.z, color.w * texel.w};
                        }
                        const float alpha = Math::Saturate(color.w);
                        if (alpha <= 0.0f) continue;
                        const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(target.width) + static_cast<std::size_t>(x)) * 4u;
                        std::uint8_t* dst = &target.pixels[i];
                        dst[0] = EncodeSrgb8(Math::Lerp(decode[dst[0]], color.x, alpha));
                        dst[1] = EncodeSrgb8(Math::Lerp(decode[dst[1]], color.y, alpha));
                        dst[2] = EncodeSrgb8(Math::Lerp(decode[dst[2]], color.z, alpha));
                    }
                }
            }
        }
    };
    if (jobs) jobs->ParallelFor(static_cast<std::size_t>(strips), 1, drawStrips);
    else drawStrips(0, static_cast<std::size_t>(strips));
    sprites_.clear();
}

void TileSet::TileUv(int tile, Vec2& uvMin, Vec2& uvMax) const {
    const int cols = std::max(1, columns), rowCount = std::max(1, rows);
    const int tx = tile % cols, ty = tile / cols;
    const float inset = 0.25f / static_cast<float>(std::max(1, texture ? texture->Width() : 64)); // avoid atlas bleeding
    uvMin = {static_cast<float>(tx) / static_cast<float>(cols) + inset, static_cast<float>(ty) / static_cast<float>(rowCount) + inset};
    uvMax = {static_cast<float>(tx + 1) / static_cast<float>(cols) - inset, static_cast<float>(ty + 1) / static_cast<float>(rowCount) - inset};
}

Tilemap::Tilemap(int width, int height, float tileSize)
    : width_(std::max(0, width)), height_(std::max(0, height)), tileSize_(tileSize > 0.0f ? tileSize : 1.0f),
      tiles_(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), -1) {}

void Tilemap::Set(int x, int y, int tile) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    tiles_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)] = tile;
}

int Tilemap::Get(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return -1;
    return tiles_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
}

bool Tilemap::IsSolid(int x, int y, const TileSet& tiles) const {
    const int tile = Get(x, y);
    return tile >= 0 && static_cast<std::size_t>(tile) < tiles.solid.size() && tiles.solid[static_cast<std::size_t>(tile)];
}

std::size_t Tilemap::Emit(SpriteBatch& batch, const Camera2D& camera, const TileSet& tiles, int layer) const {
    Vec2 minimum, maximum;
    camera.VisibleBounds(minimum, maximum);
    const int x0 = std::max(0, static_cast<int>(std::floor(minimum.x / tileSize_)) - 1);
    const int y0 = std::max(0, static_cast<int>(std::floor(minimum.y / tileSize_)) - 1);
    const int x1 = std::min(width_ - 1, static_cast<int>(std::ceil(maximum.x / tileSize_)) + 1);
    const int y1 = std::min(height_ - 1, static_cast<int>(std::ceil(maximum.y / tileSize_)) + 1);
    std::size_t emitted = 0;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const int tile = Get(x, y);
            if (tile < 0) continue;
            Sprite sprite;
            sprite.texture = tiles.texture;
            tiles.TileUv(tile, sprite.uvMin, sprite.uvMax);
            sprite.position = {(static_cast<float>(x) + 0.5f) * tileSize_, (static_cast<float>(y) + 0.5f) * tileSize_};
            sprite.size = {tileSize_ * 1.001f, tileSize_ * 1.001f}; // hide seams at fractional zoom
            sprite.layer = layer;
            batch.Draw(sprite);
            ++emitted;
        }
    }
    return emitted;
}

bool Tilemap::BoxOverlapsSolid(Vec2 center, Vec2 half, const TileSet& tiles) const {
    const int x0 = static_cast<int>(std::floor((center.x - half.x) / tileSize_));
    const int x1 = static_cast<int>(std::floor((center.x + half.x) / tileSize_ - 1.0e-5f));
    const int y0 = static_cast<int>(std::floor((center.y - half.y) / tileSize_));
    const int y1 = static_cast<int>(std::floor((center.y + half.y) / tileSize_ - 1.0e-5f));
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            if (IsSolid(x, y, tiles)) return true;
    return false;
}

Tilemap::MoveResult Tilemap::MoveBox(Vec2 center, Vec2 half, Vec2 delta, const TileSet& tiles) const {
    MoveResult result;
    result.position = center;
    if (!std::isfinite(delta.x) || !std::isfinite(delta.y)) return result;
    const float maxStep = tileSize_ * 0.25f;
    for (int axis = 0; axis < 2; ++axis) {
        float remaining = axis == 0 ? delta.x : delta.y;
        while (std::fabs(remaining) > 0.0f) {
            const float step = std::clamp(remaining, -maxStep, maxStep);
            Vec2 next = result.position;
            if (axis == 0) next.x += step; else next.y += step;
            if (BoxOverlapsSolid(next, half, tiles)) {
                // Snap flush against the blocking tile boundary.
                float& coordinate = axis == 0 ? result.position.x : result.position.y;
                const float extent = axis == 0 ? half.x : half.y;
                if (step > 0.0f) {
                    const float edge = std::floor((coordinate + extent + step) / tileSize_) * tileSize_;
                    coordinate = std::max(coordinate, edge - extent - 1.0e-4f);
                } else {
                    const float edge = (std::floor((coordinate - extent + step) / tileSize_) + 1.0f) * tileSize_;
                    coordinate = std::min(coordinate, edge + extent + 1.0e-4f);
                }
                if (axis == 0) result.hitX = true;
                else {
                    result.hitY = true;
                    result.grounded = step < 0.0f;
                }
                break;
            }
            result.position = next;
            remaining -= step;
        }
    }
    return result;
}

} // namespace Astral::Graphics
