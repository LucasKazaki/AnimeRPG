#pragma once

#include "Engine/Math/Math.h"
#include "Engine/Scene/Transform.h"

namespace Astral::Scene {

class OrthographicCamera {
public:
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
