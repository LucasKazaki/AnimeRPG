#include "Engine/Scene/Camera.h"

namespace Astral::Scene {

Math::Vec2 OrthographicCamera::WorldToScreen(const Math::Vec3& worldPosition, int viewportWidth,
    int viewportHeight) const {
    const float halfWidth = static_cast<float>(viewportWidth) * 0.5f;
    const float halfHeight = static_cast<float>(viewportHeight) * 0.5f;
    return {
        halfWidth + (worldPosition.x / worldWidth) * static_cast<float>(viewportWidth),
        halfHeight - (worldPosition.y / worldHeight) * static_cast<float>(viewportHeight),
    };
}

} // namespace Astral::Scene
