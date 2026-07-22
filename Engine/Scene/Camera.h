#pragma once

#include "Engine/Math/Math.h"

namespace Astral::Scene {

class OrthographicCamera {
public:
    Math::Vec2 WorldToScreen(const Math::Vec3& worldPosition, int viewportWidth,
        int viewportHeight) const;

    float worldWidth{20.0f};
    float worldHeight{12.0f};
};

} // namespace Astral::Scene
