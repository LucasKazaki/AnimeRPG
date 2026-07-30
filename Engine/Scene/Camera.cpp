#include "Engine/Scene/Camera.h"

#include <cmath>

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

namespace {
constexpr float kForwardY = -0.514495755f;
constexpr float kForwardZ = 0.857492926f;
constexpr float kPi = 3.14159265358979323846f;
}

float PerspectiveCamera::Depth(const Math::Vec3& worldPosition) const {
    const float relativeY = worldPosition.y - position_.y;
    const float relativeZ = worldPosition.z - position_.z;
    return relativeY * kForwardY + relativeZ * kForwardZ;
}

bool PerspectiveCamera::WorldToScreen(const Math::Vec3& worldPosition, int viewportWidth,
    int viewportHeight, Math::Vec2& screenPosition) const {
    if (viewportWidth <= 0 || viewportHeight <= 0 || nearPlane <= 0.0f
        || verticalFieldOfViewDegrees <= 0.0f || verticalFieldOfViewDegrees >= 180.0f) {
        return false;
    }

    const float depth = Depth(worldPosition);
    if (depth <= nearPlane) return false;

    const float relativeX = worldPosition.x - position_.x;
    const float relativeY = worldPosition.y - position_.y;
    const float relativeZ = worldPosition.z - position_.z;
    const float cameraY = relativeY * kForwardZ - relativeZ * kForwardY;
    const float focalLength = static_cast<float>(viewportHeight)
        / (2.0f * std::tan(verticalFieldOfViewDegrees * kPi / 360.0f));
    screenPosition = {
        static_cast<float>(viewportWidth) * 0.5f + relativeX * focalLength / depth,
        static_cast<float>(viewportHeight) * 0.5f - cameraY * focalLength / depth,
    };
    return true;
}

void PerspectiveCamera::Follow(const Transform& target) {
    const Math::Vec3 targetPosition = target.WorldPosition();
    position_ = {targetPosition.x, 6.0f, targetPosition.y - 10.0f};
}

} // namespace Astral::Scene
