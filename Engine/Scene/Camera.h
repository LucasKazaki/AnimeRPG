#pragma once

#include "Engine/Math/Math.h"
#include "Engine/Scene/Transform.h"

namespace Astral::Scene {

class OrthographicCamera {
public:
    // Checked projection leaves output unchanged on failure. The compatibility
    // wrapper returns {0,0}; use this query to distinguish failure from origin.
    bool TryWorldToScreen(const Math::Vec3& worldPosition, int viewportWidth,
        int viewportHeight, Math::Vec2& screenPosition) const;
    Math::Vec2 WorldToScreen(const Math::Vec3& worldPosition, int viewportWidth,
        int viewportHeight) const;
    void Follow(const Transform& target, const Math::Vec3& offset = {});

    float worldWidth{20.0f};
    float worldHeight{12.0f};

private:
    Math::Vec3 focus_{};
};

class PerspectiveCamera {
public:
    // Reject invalid inputs/unsafe raster coordinates without changing output.
    // Finite offscreen points remain valid; this does not clip to the viewport.
    bool WorldToScreen(const Math::Vec3& worldPosition, int viewportWidth,
        int viewportHeight, Math::Vec2& screenPosition) const;
    void Follow(const Transform& target);

    // Returns NaN for invalid input or a depth outside float range.
    float Depth(const Math::Vec3& worldPosition) const;
    const Math::Vec3& Position() const { return position_; }
    float NearPlane() const { return nearPlane; }

    float verticalFieldOfViewDegrees{60.0f};
    float nearPlane{0.5f};

private:
    Math::Vec3 position_{0.0f, 6.0f, -10.0f};
};

} // namespace Astral::Scene
