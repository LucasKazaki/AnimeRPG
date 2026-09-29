#include "Engine/Physics/CharacterController.h"

#include <algorithm>
#include <cmath>

namespace Astral::Physics {

using namespace Math;

CharacterController::CharacterController(PhysicsWorld& world, const CharacterSettings& settings, Vec3 footPosition)
    : world_(world), settings_(settings) {
    settings_.radius = std::max(0.05f, settings_.radius);
    settings_.height = std::max(settings_.radius * 2.0f + 0.01f, settings_.height);
    settings_.skin = Clamp(settings_.skin, 0.001f, settings_.radius * 0.5f);
    capsule_ = Shape::CapsuleFromHeight(settings_.radius, settings_.height);
    minWalkableY_ = std::cos(Radians(Clamp(settings_.maxSlopeDegrees, 0.0f, 89.0f)));
    foot_ = IsFinite(footPosition) ? footPosition : Vec3{};
    Depenetrate();
}

Pose CharacterController::CapsulePose(Vec3 foot) const {
    return {foot + Vec3{0.0f, settings_.height * 0.5f, 0.0f}, Quat{}};
}

Vec3 CharacterController::Center() const { return CapsulePose(foot_).position; }

void CharacterController::Teleport(Vec3 footPosition) {
    if (!IsFinite(footPosition)) return;
    foot_ = footPosition;
    velocity_ = {};
    Depenetrate();
}

void CharacterController::Depenetrate() {
    // Restores at least half a skin of clearance from every nearby solid, so
    // resting contacts never read as blocking hits on the next sweep.
    const float clearance = settings_.skin * 0.5f;
    Shape probe = capsule_;
    probe.radius += settings_.skin;
    std::vector<BodyId> nearby;
    for (int iteration = 0; iteration < 4; ++iteration) {
        const Pose pose = CapsulePose(foot_);
        world_.Overlap(probe, pose, nearby, settings_.collisionMask, false);
        Vec3 push{};
        for (BodyId id : nearby) {
            if (id == ignore_) continue;
            const Body* body = world_.GetBody(id);
            if (!body) continue;
            // Every nearby feature: a mesh crease needs one push per face.
            Proximity found[16];
            const int count = RoundedProximities(capsule_, pose, body->shape, body->pose, clearance, found, 16);
            for (int i = 0; i < count; ++i) {
                if (found[i].distance >= clearance) continue;
                const Vec3 correction = found[i].normalFromB * (clearance - found[i].distance);
                // Combine pushes per axis so parallel contacts do not double up.
                push.x = std::fabs(correction.x) > std::fabs(push.x) ? correction.x : push.x;
                push.y = std::fabs(correction.y) > std::fabs(push.y) ? correction.y : push.y;
                push.z = std::fabs(correction.z) > std::fabs(push.z) ? correction.z : push.z;
            }
        }
        if (LengthSquared(push) < 1.0e-12f) return;
        foot_ += push;
    }
}

bool CharacterController::IsWalkable(const ShapeCastHit& hit, Vec3& floorNormal) const {
    floorNormal = hit.normal;
    if (hit.normal.y >= minWalkableY_) return true;
    if (hit.normal.y <= 0.05f) return false; // walls and ceilings
    const Vec3 inward = Normalize(-Horizontal(hit.normal), {});
    const Vec3 origin = hit.point + inward * 0.03f + Vec3{0.0f, 0.06f, 0.0f};
    RaycastHit ray;
    if (world_.Raycast({origin, {0.0f, -1.0f, 0.0f}}, 0.12f, ray, settings_.collisionMask, ignore_)
        && ray.distance > 1.0e-4f && ray.normal.y >= minWalkableY_) {
        floorNormal = ray.normal;
        return true;
    }
    return false;
}

bool CharacterController::ProbeGround(float distance, float& hitDistance, Vec3& normal) const {
    ShapeCastHit hit;
    if (!world_.ShapeCast(capsule_, CapsulePose(foot_), {0.0f, -(distance + settings_.skin), 0.0f}, hit,
            settings_.collisionMask, ignore_)) {
        return false;
    }
    if (!IsWalkable(hit, normal)) return false;
    hitDistance = std::max(0.0f, hit.distance - settings_.skin);
    return true;
}

Vec3 CharacterController::SlideMove(Vec3 motion, bool& hitWalkable, Vec3& walkableNormal) {
    hitWalkable = false;
    const Vec3 start = foot_;
    const bool vertical = std::fabs(motion.y) > 0.0f && std::fabs(motion.x) + std::fabs(motion.z) < 1.0e-7f;
    Vec3 remaining = motion;
    Vec3 planes[4];
    int planeCount = 0;
    for (int iteration = 0; iteration < 4; ++iteration) {
        const float length = Length(remaining);
        if (length < 1.0e-6f) break;
        ShapeCastHit hit;
        if (!world_.ShapeCast(capsule_, CapsulePose(foot_), remaining, hit, settings_.collisionMask, ignore_)) {
            foot_ += remaining;
            break;
        }
        const Vec3 direction = remaining / length;
        const float travel = hit.startPenetrating ? 0.0f : std::max(0.0f, hit.distance - settings_.skin);
        foot_ += direction * travel;
        Vec3 n = hit.normal;
        Vec3 floorNormal;
        const bool walkable = IsWalkable(hit, floorNormal);
        if (walkable) {
            hitWalkable = true;
            walkableNormal = floorNormal;
            // Falling onto walkable ground ends the vertical move (no sliding down slopes).
            if (vertical && motion.y < 0.0f) break;
        } else if (!vertical && motion.y <= 0.0f) {
            // Steep surfaces act as vertical walls for horizontal movement.
            const Vec3 flat = Horizontal(n);
            if (LengthSquared(flat) > 1.0e-8f) n = Normalize(flat);
        }
        remaining = direction * (length - travel);
        remaining = remaining - n * Dot(remaining, n);
        // Moving into a crease between two planes: slide along their intersection.
        for (int p = 0; p < planeCount; ++p) {
            if (Dot(remaining, planes[p]) < -1.0e-5f) {
                const Vec3 crease = Normalize(Cross(planes[p], n), {});
                remaining = crease * Dot(remaining, crease);
            }
        }
        if (planeCount < 4) planes[planeCount++] = n;
        if (hit.startPenetrating) {
            foot_ += n * settings_.skin;
        }
    }
    return foot_ - start;
}

Vec3 CharacterController::Move(Vec3 desiredVelocity, bool jump, float dt) {
    if (!(dt > 0.0f) || !std::isfinite(dt)) return {};
    if (!IsFinite(desiredVelocity)) desiredVelocity = {};
    const Vec3 startFoot = foot_;
    Depenetrate();
    const bool wasGrounded = grounded_;
    timeSinceGrounded_ = grounded_ ? 0.0f : timeSinceGrounded_ + dt;

    Vec3 horizontal = Horizontal(velocity_);
    const float acceleration = grounded_ ? settings_.groundAcceleration : settings_.airAcceleration;
    horizontal = MoveTowards(horizontal, Horizontal(desiredVelocity), acceleration * dt);
    float vertical = velocity_.y;
    bool jumped = false;
    if (jump && (grounded_ || timeSinceGrounded_ <= settings_.coyoteTime) && vertical <= 0.1f) {
        vertical = settings_.jumpSpeed;
        grounded_ = false;
        timeSinceGrounded_ = settings_.coyoteTime + 1.0f;
        jumped = true;
    } else if (grounded_) {
        vertical = 0.0f;
    } else {
        vertical += settings_.gravity * dt;
    }

    // Horizontal pass with automatic step-up.
    bool walkable = false;
    Vec3 walkableNormal{0, 1, 0};
    const Vec3 horizontalMotion = horizontal * dt;
    // On slopes, move along the ground plane so walking uphill does not lose speed.
    Vec3 slideMotion = horizontalMotion;
    if (grounded_ && groundNormal_.y < 0.999f) slideMotion = Normalize(ProjectOnPlane(horizontalMotion, groundNormal_), {}) * Length(horizontalMotion);
    const Vec3 beforeHorizontal = foot_;
    const Vec3 plain = SlideMove(slideMotion, walkable, walkableNormal);
    const float wanted = Length(horizontalMotion);
    if (wasGrounded && !jumped && wanted > 1.0e-5f && Length(Horizontal(plain)) < wanted * 0.6f && settings_.stepHeight > 0.0f) {
        const Vec3 plainFoot = foot_;
        foot_ = beforeHorizontal;
        bool w1 = false, w2 = false, w3 = false;
        Vec3 n1, n2, n3;
        const Vec3 up = SlideMove({0.0f, settings_.stepHeight, 0.0f}, w1, n1);
        SlideMove(horizontalMotion, w2, n2);
        SlideMove({0.0f, -(up.y + settings_.skin * 2.0f), 0.0f}, w3, n3);
        const float progress = Length(Horizontal(foot_ - beforeHorizontal));
        if (w3 && progress > Length(Horizontal(plain)) + 1.0e-4f && foot_.y > beforeHorizontal.y - 1.0e-3f) {
            groundNormal_ = n3;
        } else {
            foot_ = plainFoot;
        }
    }

    // Vertical pass.
    bool landed = false;
    Vec3 landedNormal{0, 1, 0};
    if (vertical != 0.0f) {
        const Vec3 applied = SlideMove({0.0f, vertical * dt, 0.0f}, landed, landedNormal);
        if (vertical > 0.0f && applied.y < vertical * dt - 1.0e-5f) vertical = 0.0f; // bumped head
    }
    if (landed && vertical <= 0.0f) {
        grounded_ = true;
        groundNormal_ = landedNormal;
        vertical = 0.0f;
    } else {
        float distance = 0.0f;
        Vec3 normal;
        // Snap down stairs/slopes while walking; otherwise just check contact.
        const float probe = (wasGrounded && !jumped) ? settings_.stepHeight : settings_.skin * 2.0f;
        if (vertical <= 0.0f && ProbeGround(probe, distance, normal)) {
            foot_.y -= distance;
            grounded_ = true;
            groundNormal_ = normal;
            vertical = 0.0f;
        } else {
            grounded_ = false;
        }
    }
    const Vec3 displacement = foot_ - startFoot;
    velocity_ = {displacement.x / dt, vertical, displacement.z / dt};
    return displacement;
}

Vec3 CharacterController::SweepTo(Vec3 target) {
    if (!IsFinite(target)) return foot_;
    const Vec3 motion = target - foot_;
    const float length = Length(motion);
    if (length < 1.0e-6f) return foot_;
    ShapeCastHit hit;
    // Lift slightly so ground contact does not block a horizontal dash.
    const Vec3 lift{0.0f, settings_.skin * 2.0f, 0.0f};
    const Pose start = CapsulePose(foot_ + lift);
    if (world_.ShapeCast(capsule_, start, motion, hit, settings_.collisionMask, ignore_) && !hit.startPenetrating) {
        foot_ += motion * (std::max(0.0f, hit.distance - settings_.skin) / length);
    } else if (!hit.startPenetrating) {
        foot_ = target;
    }
    float distance = 0.0f;
    Vec3 normal;
    if (ProbeGround(settings_.stepHeight * 2.0f, distance, normal)) {
        foot_.y -= distance;
        grounded_ = true;
        groundNormal_ = normal;
    }
    return foot_;
}

} // namespace Astral::Physics
