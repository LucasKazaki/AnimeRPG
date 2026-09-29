#pragma once

// Genuine 2D rendering: an orthographic 2D camera, a layered sprite batch with
// atlas sub-rectangles, rotation, pivots, tint and point/bilinear filtering,
// and tilemaps with solid-tile collision (axis-separated AABB sweeps for
// platformer/top-down movement). Blending happens in linear light and strips
// of rows are drawn in parallel while preserving layer/submission order.

#include "Engine/Core/JobSystem.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Texture.h"

#include <vector>

namespace Astral::Graphics {

struct Camera2D {
    Math::Vec2 position{};   // world point at the viewport centre
    float zoom{32.0f};       // pixels per world unit
    float rotation{};        // radians
    int viewportWidth{1280};
    int viewportHeight{720};

    Math::Vec2 WorldToScreen(Math::Vec2 world) const;
    Math::Vec2 ScreenToWorld(Math::Vec2 screen) const;
    // World-space AABB of the visible area (accounts for rotation).
    void VisibleBounds(Math::Vec2& minimum, Math::Vec2& maximum) const;
};

struct Sprite {
    const Texture2D* texture{};  // null = solid tint
    Math::Vec2 uvMin{0.0f, 0.0f};
    Math::Vec2 uvMax{1.0f, 1.0f};
    Math::Vec2 position{};       // world units, +y up
    Math::Vec2 size{1.0f, 1.0f};
    Math::Vec2 pivot{0.5f, 0.5f};
    float rotation{};
    Math::Vec4 tint{1, 1, 1, 1}; // linear rgb, alpha
    int layer{};
    bool flipX{};
    bool pointSampling{true};    // crisp pixel art
};

class SpriteBatch {
public:
    void Begin(const Camera2D& camera);
    void Draw(const Sprite& sprite);
    // Rasterises all queued sprites into `target` (sRGB) in layer order.
    void End(ImageRgba8& target, Core::JobSystem* jobs = nullptr);
    std::size_t QueuedCount() const { return sprites_.size(); }
    std::size_t LastDrawnCount() const { return lastDrawn_; }

private:
    Camera2D camera_;
    std::vector<Sprite> sprites_;
    std::size_t lastDrawn_{};
};

struct TileSet {
    const Texture2D* texture{};
    int columns{1};
    int rows{1};
    std::vector<bool> solid; // per tile index
    void TileUv(int tile, Math::Vec2& uvMin, Math::Vec2& uvMax) const;
};

class Tilemap {
public:
    Tilemap(int width, int height, float tileSize);
    int Width() const { return width_; }
    int Height() const { return height_; }
    float TileSize() const { return tileSize_; }
    void Set(int x, int y, int tile);
    int Get(int x, int y) const; // -1 = empty or out of range
    bool IsSolid(int x, int y, const TileSet& tiles) const;

    // Emits sprites only for tiles overlapping the camera's view.
    std::size_t Emit(SpriteBatch& batch, const Camera2D& camera, const TileSet& tiles, int layer) const;

    struct MoveResult {
        Math::Vec2 position{};
        bool hitX{}, hitY{}, grounded{};
    };
    // Moves an axis-aligned box (centre, half extents) by delta, resolving X
    // then Y against solid tiles; never tunnels through a tile.
    MoveResult MoveBox(Math::Vec2 center, Math::Vec2 halfExtents, Math::Vec2 delta, const TileSet& tiles) const;

private:
    bool BoxOverlapsSolid(Math::Vec2 center, Math::Vec2 half, const TileSet& tiles) const;
    int width_{};
    int height_{};
    float tileSize_{1.0f};
    std::vector<int> tiles_;
};

} // namespace Astral::Graphics
