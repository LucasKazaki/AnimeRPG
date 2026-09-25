#include "Engine/Scene/Camera.h"

#include <cmath>
#include <limits>

namespace Astral::Scene {
namespace {
constexpr float kForwardY = -0.514495755f;
constexpr float kForwardZ = 0.857492926f;
constexpr double kPi = 3.14159265358979323846;

bool CommitProjection(double x, double y, Math::Vec2& output) {
    // GDI callers round to signed 32-bit coordinates and add small HUD offsets.
    // Leave headroom for float rounding and those offsets, not only a finite test.
    constexpr double rasterLimit = 2147483647.0 - 4096.0;
    if (!std::isfinite(x) || !std::isfinite(y)
        || std::abs(x) > rasterLimit || std::abs(y) > rasterLimit) return false;
    output = {static_cast<float>(x), static_cast<float>(y)};
    return true;
}
}

bool OrthographicCamera::TryWorldToScreen(const Math::Vec3& worldPosition,
    int viewportWidth, int viewportHeight, Math::Vec2& screenPosition) const {
    if (viewportWidth <= 0 || viewportHeight <= 0
        || !std::isfinite(worldWidth) || !std::isfinite(worldHeight)
        || worldWidth <= 0.0f || worldHeight <= 0.0f
        || !Math::IsFinite(worldPosition) || !Math::IsFinite(focus_)) return false;
    const double relativeX = static_cast<double>(worldPosition.x) - focus_.x;
    const double relativeY = static_cast<double>(worldPosition.y) - focus_.y;
    return CommitProjection((relativeX / worldWidth + 0.5) * viewportWidth,
        (0.5 - relativeY / worldHeight) * viewportHeight, screenPosition);
}

Math::Vec2 OrthographicCamera::WorldToScreen(const Math::Vec3& worldPosition,
    int viewportWidth, int viewportHeight) const {
    Math::Vec2 result{};
    TryWorldToScreen(worldPosition, viewportWidth, viewportHeight, result);
    return result;
}

void OrthographicCamera::Follow(const Transform& target, const Math::Vec3& offset) {
    Math::Vec3 targetPosition{};
    if (!target.TryWorldPosition(targetPosition) || !Math::IsFinite(offset)) return;
    const double x = static_cast<double>(targetPosition.x) + offset.x;
    const double y = static_cast<double>(targetPosition.y) + offset.y;
    const double z = static_cast<double>(targetPosition.z) + offset.z;
    constexpr double limit = std::numeric_limits<float>::max();
    if (std::abs(x) > limit || std::abs(y) > limit || std::abs(z) > limit) return;
    focus_ = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
}

float PerspectiveCamera::Depth(const Math::Vec3& worldPosition) const {
    if (!Math::IsFinite(worldPosition) || !Math::IsFinite(position_))
        return std::numeric_limits<float>::quiet_NaN();
    const double relativeY = static_cast<double>(worldPosition.y) - position_.y;
    const double relativeZ = static_cast<double>(worldPosition.z) - position_.z;
    const double depth = relativeY * kForwardY + relativeZ * kForwardZ;
    if (std::abs(depth) > std::numeric_limits<float>::max())
        return std::numeric_limits<float>::quiet_NaN();
    return static_cast<float>(depth);
}

bool PerspectiveCamera::WorldToScreen(const Math::Vec3& worldPosition, int viewportWidth,
    int viewportHeight, Math::Vec2& screenPosition) const {
    if (viewportWidth <= 0 || viewportHeight <= 0
        || !std::isfinite(nearPlane) || !std::isfinite(verticalFieldOfViewDegrees)
        || nearPlane <= 0.0f || verticalFieldOfViewDegrees <= 0.0f
        || verticalFieldOfViewDegrees >= 180.0f
        || !Math::IsFinite(worldPosition) || !Math::IsFinite(position_)) return false;

    // Keep the near-plane test consistent with the renderer's Depth query.
    const float depth = Depth(worldPosition);
    if (!std::isfinite(depth) || depth <= nearPlane) return false;
    const double relativeX = static_cast<double>(worldPosition.x) - position_.x;
    const double relativeY = static_cast<double>(worldPosition.y) - position_.y;
    const double relativeZ = static_cast<double>(worldPosition.z) - position_.z;
    const double cameraY = relativeY * kForwardZ - relativeZ * kForwardY;
    const double focalLength = static_cast<double>(viewportHeight)
        / (2.0 * std::tan(static_cast<double>(verticalFieldOfViewDegrees) * kPi / 360.0));
    return CommitProjection(viewportWidth * 0.5 + relativeX * focalLength / depth,
        viewportHeight * 0.5 - cameraY * focalLength / depth, screenPosition);
}

void PerspectiveCamera::Follow(const Transform& target) {
    Math::Vec3 targetPosition{};
    if (!target.TryWorldPosition(targetPosition)) return;
    const double z = static_cast<double>(targetPosition.y) - 10.0;
    if (std::abs(z) > std::numeric_limits<float>::max()) return;
    // Preserve the prototype's XY gameplay plane mapped to XZ presentation.
    position_ = {targetPosition.x, 6.0f, static_cast<float>(z)};
}

} // namespace Astral::Scene
