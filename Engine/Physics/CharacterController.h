#pragma once

// Kinematic capsule character controller (the role of UE's CharacterMovement
// component): collide-and-slide against the physics world, slope limits,
// automatic step-up onto stairs and kerbs, ground snapping on descents,
// jumping with coyote time, depenetration, and a swept blink/dash that stops
// at obstacles instead of passing through walls.

#include "Engine/Physics/PhysicsWorld.h"

namespace Astral::Physics {

struct CharacterSettings {
    float radius{0.35f};
    float height{1.8f};
    float stepHeight{0.4f};
    float maxSlopeDegrees{50.0f};
    float skin{0.02f};
    float gravity{-24.0f};
    float jumpSpeed{8.5f};
    float coyoteTime{0.1f};
    float groundAcceleration{60.0f};
    float airAcceleration{18.0f};
    std::uint32_t collisionMask{0xFFFFFFFFu};
};

class CharacterController {
public:
    CharacterController(PhysicsWorld& world, const CharacterSettings& settings, Vec3 footPosition);

    // desiredVelocity is horizontal (y ignored). Returns the actual displacement.
    Vec3 Move(Vec3 desiredVelocity, bool jump, float dt);
    // Sweeps from the current position toward target (horizontal dash/blink);
    // stops `skin` short of the first obstacle. Returns the reached foot position.
    Vec3 SweepTo(Vec3 targetFoot);
    void Teleport(Vec3 footPosition);

    Vec3 FootPosition() const { return foot_; }
    Vec3 Center() const;
    Vec3 Velocity() const { return velocity_; }
    bool Grounded() const { return grounded_; }
    Vec3 GroundNormal() const { return groundNormal_; }
    const CharacterSettings& Settings() const { return settings_; }
    // Bodies to ignore (e.g. the character's own hitbox trigger).
    void SetIgnoredBody(BodyId body) { ignore_ = body; }

private:
    Pose CapsulePose(Vec3 foot) const;
    // Collide-and-slide; returns the applied displacement.
    Vec3 SlideMove(Vec3 motion, bool& hitWalkable, Vec3& walkableNormal);
    bool ProbeGround(float distance, float& hitDistance, Vec3& normal) const;
    // Rounded capsules touch step edges with steep normals; confirm the actual
    // floor under the contact with a short ray before calling it a wall.
    bool IsWalkable(const ShapeCastHit& hit, Vec3& floorNormal) const;
    void Depenetrate();

    PhysicsWorld& world_;
    CharacterSettings settings_;
    Shape capsule_;
    Vec3 foot_{};
    Vec3 velocity_{};
    Vec3 groundNormal_{0, 1, 0};
    bool grounded_{};
    float timeSinceGrounded_{};
    float minWalkableY_{};
    BodyId ignore_{};
};

} // namespace Astral::Physics
