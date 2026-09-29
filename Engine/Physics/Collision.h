#pragma once

// Narrowphase contact generation, ray casts, distances and shape casts.
//
// Pairs: sphere/capsule combinations use closest points between core segments;
// sphere/capsule vs box uses convex segment-box distance with a SAT fallback
// for deep overlap; box vs box uses the 15-axis separating axis test with
// reference-face clipping (up to 4 contact points) so stacks rest stably.
// Triangle meshes collide per triangle (sphere/capsule by segment-triangle
// closest points, boxes by 13-axis SAT and clipping), fix up contacts on
// internal edges with the face normal, and group the result into one manifold
// per distinct normal.

#include "Engine/Physics/Shapes.h"

namespace Astral::Physics {

struct ContactPoint {
    Vec3 position{};
    float penetration{};
};

struct Manifold {
    Vec3 normal{0.0f, 1.0f, 0.0f}; // unit, points from shape A toward shape B
    ContactPoint points[4]{};
    int count{};
};

// Single manifold: for mesh pairs, the deepest of CollideAll's manifolds.
bool Collide(const Shape& a, const Pose& poseA, const Shape& b, const Pose& poseB, Manifold& manifold);

// Mesh pairs can touch several faces at once (a crease, a step): up to
// `capacity` manifolds, one per contact normal. Convex pairs give at most one.
constexpr int kMaxManifolds = 6;
int CollideAll(const Shape& a, const Pose& poseA, const Shape& b, const Pose& poseB, Manifold* manifolds, int capacity);

bool RaycastShape(const Shape& shape, const Pose& pose, const Math::Ray& ray, float maxDistance, float& t, Vec3& normal);

// Signed distance from a sphere/capsule `a` to any shape `b` (negative when
// overlapping). normalFromB points from b toward a at the closest features.
float RoundedDistance(const Shape& a, const Pose& poseA, const Shape& b, const Pose& poseB, Vec3& normalFromB, Vec3& pointOnB);

struct Proximity {
    float distance{};  // signed, negative when overlapping
    Vec3 normalFromB{};
    Vec3 pointOnB{};
};

// Every separate feature of `b` within maxDistance of a sphere/capsule `a`
// (one per nearby mesh triangle; convex shapes give one), nearest first.
// Depenetration needs all of them: one normal cannot resolve a crease.
int RoundedProximities(const Shape& a, const Pose& poseA, const Shape& b, const Pose& poseB, float maxDistance,
    Proximity* out, int capacity);

struct ShapeCastResult {
    float fraction{1.0f}; // of the motion at first contact
    Vec3 normal{};        // from the hit surface toward the moving shape
    Vec3 point{};         // on the hit surface
    bool startPenetrating{};
};

// Sweeps a sphere or capsule along `motion` against one shape (conservative
// advancement; meshes are swept triangle by triangle and the earliest hit wins).
bool ShapeCast(const Shape& moving, const Pose& start, Vec3 motion, const Shape& target, const Pose& targetPose,
    ShapeCastResult& result, float tolerance = 1.0e-3f);

// Closest point of a (local-space) segment to an origin-centred box of half extents e.
void ClosestSegmentBox(Vec3 a, Vec3 b, Vec3 e, float& t, Vec3& pointOnSegment, Vec3& pointOnBox);

} // namespace Astral::Physics
