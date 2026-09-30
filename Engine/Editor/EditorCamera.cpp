#include "Engine/Editor/EditorCamera.h"

#include <algorithm>
#include <cmath>

namespace Astral::Editor {

using Math::Quat;
using Math::Vec2;
using Math::Vec3;

namespace {

float WrapDegrees(float degrees) {
    degrees = std::fmod(degrees, 360.0f);
    if (degrees > 180.0f) degrees -= 360.0f;
    if (degrees <= -180.0f) degrees += 360.0f;
    return degrees;
}

} // namespace

Quat QuatFromEulerDegrees(Vec3 pitchYawRoll) {
    return Math::QuatFromEuler(Math::Radians(pitchYawRoll.y), Math::Radians(pitchYawRoll.x),
        Math::Radians(pitchYawRoll.z));
}

Vec3 EulerDegreesFromQuat(Quat rotation) {
    const Quat q = Math::Normalize(rotation);
    const Vec3 forward = Math::Rotate(q, {0.0f, 0.0f, 1.0f});
    const float pitch = std::asin(std::clamp(-forward.y, -1.0f, 1.0f));
    const bool looksStraightUpOrDown = std::fabs(forward.x) < 1.0e-6f && std::fabs(forward.z) < 1.0e-6f;
    const float yaw = looksStraightUpOrDown ? 0.0f : std::atan2(forward.x, forward.z);
    // What remains after yaw and pitch is a pure roll about +Z.
    const Quat roll = Math::Inverse(Math::QuatFromEuler(yaw, pitch, 0.0f)) * q;
    const float rollAngle = 2.0f * std::atan2(roll.z, roll.w);
    return {WrapDegrees(Math::Degrees(pitch)), WrapDegrees(Math::Degrees(yaw)), WrapDegrees(Math::Degrees(rollAngle))};
}

Quat EditorCamera::Rotation() const {
    return QuatFromEulerDegrees({pitchDegrees, yawDegrees, 0.0f});
}

Vec3 EditorCamera::Forward() const { return Math::Rotate(Rotation(), {0.0f, 0.0f, 1.0f}); }
Vec3 EditorCamera::Right() const { return Math::Rotate(Rotation(), {1.0f, 0.0f, 0.0f}); }
Vec3 EditorCamera::Up() const { return Math::Rotate(Rotation(), {0.0f, 1.0f, 0.0f}); }
Vec3 EditorCamera::Eye() const { return target - Forward() * distance; }

void EditorCamera::Orbit(float deltaX, float deltaY) {
    yawDegrees = WrapDegrees(yawDegrees + deltaX * 0.3f);
    pitchDegrees = std::clamp(pitchDegrees + deltaY * 0.3f, -89.0f, 89.0f);
}

void EditorCamera::Pan(float deltaX, float deltaY, int viewportHeight) {
    const float perPixel = WorldPerPixel(target, viewportHeight);
    target = target - Right() * (deltaX * perPixel) + Up() * (deltaY * perPixel);
}

void EditorCamera::Dolly(float wheelSteps) {
    distance = std::clamp(distance * std::pow(0.85f, wheelSteps), 0.5f, 2000.0f);
}

void EditorCamera::Frame(const Math::AABB& bounds) {
    if (!bounds.IsValid()) return;
    target = bounds.Center();
    const float radius = std::max(0.5f, Math::Length(bounds.Extents()));
    distance = std::clamp(radius / std::sin(Math::Radians(fieldOfView * 0.5f)) * 0.9f, 2.0f, 2000.0f);
}

Graphics::RenderView EditorCamera::View(int width, int height) const {
    return Graphics::RenderView::Perspective(Eye(), target, fieldOfView, width, height, nearPlane, farPlane);
}

Math::Ray EditorCamera::RayThrough(float x, float y, int width, int height) const {
    width = std::max(1, width);
    height = std::max(1, height);
    const float tanHalf = std::tan(Math::Radians(fieldOfView * 0.5f));
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    const float ndcX = 2.0f * x / static_cast<float>(width) - 1.0f;
    const float ndcY = 1.0f - 2.0f * y / static_cast<float>(height);
    Math::Ray ray;
    ray.origin = Eye();
    ray.direction = Math::Normalize(Forward() + Right() * (ndcX * tanHalf * aspect) + Up() * (ndcY * tanHalf),
        Forward());
    return ray;
}

bool EditorCamera::Project(Vec3 point, int width, int height, Vec2& pixel) const {
    width = std::max(1, width);
    height = std::max(1, height);
    const Vec3 offset = point - Eye();
    const float depth = Math::Dot(offset, Forward());
    if (depth < nearPlane) return false;
    const float tanHalf = std::tan(Math::Radians(fieldOfView * 0.5f));
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    const float ndcX = Math::Dot(offset, Right()) / (depth * tanHalf * aspect);
    const float ndcY = Math::Dot(offset, Up()) / (depth * tanHalf);
    pixel = {(ndcX * 0.5f + 0.5f) * static_cast<float>(width), (0.5f - ndcY * 0.5f) * static_cast<float>(height)};
    return true;
}

float EditorCamera::WorldPerPixel(Vec3 point, int viewportHeight) const {
    const float depth = std::max(nearPlane, Math::Dot(point - Eye(), Forward()));
    return 2.0f * depth * std::tan(Math::Radians(fieldOfView * 0.5f)) / static_cast<float>(std::max(1, viewportHeight));
}

} // namespace Astral::Editor
