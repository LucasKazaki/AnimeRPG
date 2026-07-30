#include "Engine/Scene/Camera.h"

namespace Astral::Scene {

Math::Vec2 OrthographicCamera::WorldToScreen(const Math::Vec3& worldPosition,
    int viewportWidth, int viewportHeight) const {
    const float relativeX = worldPosition.x - focus_.x;
    const float relativeY = worldPosition.y - focus_.y;
    const float normalizedX = (relativeX / worldWidth) + 0.5f;
    const float normalizedY = 0.5f - (relativeY / worldHeight);
    return {normalizedX * static_cast<float>(viewportWidth),
        normalizedY * static_cast<float>(viewportHeight)};
}

void OrthographicCamera::Follow(const Transform& target, const Math::Vec3& offset) {
    const Math::Vec3 targetPosition = target.WorldPosition();
    focus_ = {targetPosition.x + offset.x, targetPosition.y + offset.y,
        targetPosition.z + offset.z};
}

} // namespace Astral::Scene
