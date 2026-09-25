#pragma once

#include "Engine/Math/Math.h"
#include "Engine/Scene/Transform.h"

namespace Astral::Scene {

class OrthographicCamera {
public:
    // Checked projection leaves output unchanged on failure. The legacy wrapper
    // returns {0,0}; use TryWorldToScreen when failure must be distinguished.
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
    // Rejects invalid parameters/non-finite or unsafe raster coordinates without
    // modifying output. Ordinary off-screen points remain valid (no viewport clip).
    bool WorldToScreen(const Math::Vec3& worldPosition, int viewportWidth,
        int viewportHeight, Math::Vec2& screenPosition) const;
    void Follow(const Transform& target);

    float Depth(const Math::Vec3& worldPosition) const;
    const Math::Vec3& Position() const { return position_; }
    float NearPlane() const { return nearPlane; }

    float verticalFieldOfViewDegrees{60.0f};
    float nearPlane{0.5f};

private:
    Math::Vec3 position_{0.0f, 6.0f, -10.0f};
};

} // namespace Astral::Scene
