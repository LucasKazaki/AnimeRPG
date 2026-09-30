#pragma once

// Orbit camera for the editor viewport (Unity's Scene view camera, UE's
// perspective viewport): orbits a focus point, pans in the view plane, dollies
// toward the focus and frames bounds. It also turns viewport pixels into picking
// rays and world points into pixels, using the same left-handed perspective as
// Graphics::RenderView::Perspective, so overlays line up with the rendered image.

#include "Engine/Graphics/RenderScene.h"
#include "Engine/Math/Geometry.h"
#include "Engine/Math/VectorMath.h"

namespace Astral::Editor {

// Euler angles in the scene format's convention ({"euler": [pitch, yaw, roll]}
// degrees; yaw about +Y, then pitch about +X, then roll about +Z).
Math::Quat QuatFromEulerDegrees(Math::Vec3 pitchYawRoll);
// Inverse of QuatFromEulerDegrees (pitch in [-90, 90], yaw and roll in (-180, 180]).
Math::Vec3 EulerDegreesFromQuat(Math::Quat rotation);

class EditorCamera {
public:
    Math::Vec3 target{0.0f, 1.0f, 0.0f};
    float yawDegrees{30.0f};
    float pitchDegrees{25.0f}; // positive looks down
    float distance{16.0f};
    float fieldOfView{60.0f}; // vertical, degrees
    float nearPlane{0.1f};
    float farPlane{800.0f};

    Math::Quat Rotation() const;
    Math::Vec3 Forward() const;
    Math::Vec3 Right() const;
    Math::Vec3 Up() const;
    Math::Vec3 Eye() const;

    // Mouse-driven navigation, in pixels.
    void Orbit(float deltaX, float deltaY);
    void Pan(float deltaX, float deltaY, int viewportHeight);
    void Dolly(float wheelSteps); // positive moves toward the focus
    // Centres `bounds` and backs off until it fits the view.
    void Frame(const Math::AABB& bounds);

    Graphics::RenderView View(int width, int height) const;
    // Ray from the eye through pixel (x, y) of a width x height viewport.
    Math::Ray RayThrough(float x, float y, int width, int height) const;
    // Pixel position of `point`; false when it is behind the near plane.
    bool Project(Math::Vec3 point, int width, int height, Math::Vec2& pixel) const;
    // World-space size of one pixel at `point`'s depth.
    float WorldPerPixel(Math::Vec3 point, int viewportHeight) const;
};

} // namespace Astral::Editor
