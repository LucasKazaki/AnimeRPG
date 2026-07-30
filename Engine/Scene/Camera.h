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

} // namespace Astral::Scene
