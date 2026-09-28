#pragma once

// Rigid-body world: static/kinematic/dynamic bodies, collision layers, triggers,
// a sequential-impulse solver (Coulomb friction with two tangents, restitution,
// Baumgarte stabilisation, warm starting from a persistent contact cache),
// per-body sleeping, contact/trigger events and scene queries (ray casts,
// sphere/capsule casts, overlaps). Deterministic for identical inputs: pairs are
// processed in sorted body-index order on one thread.

#include "Engine/Core/SlotMap.h"
#include "Engine/Physics/BroadPhase.h"
#include "Engine/Physics/Collision.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace Astral::Physics {

using BodyId = Core::Handle;

enum class BodyType : std::uint8_t { Static, Kinematic, Dynamic };

struct BodyDesc {
    BodyType type{BodyType::Dynamic};
    Shape shape;
    Vec3 position{};
    Quat rotation{};
    Vec3 linearVelocity{};
    Vec3 angularVelocity{};
    float mass{1.0f};
    float friction{0.6f};
    float restitution{0.05f};
    float linearDamping{0.02f};
    float angularDamping{0.05f};
    float gravityScale{1.0f};
    std::uint32_t layer{1u};
    std::uint32_t mask{0xFFFFFFFFu};
    bool isTrigger{};
    bool lockRotation{};
    std::uint64_t userData{};
};

struct Body {
    BodyId id{};
    BodyType type{BodyType::Dynamic};
    Shape shape;
    Pose pose;
    Vec3 linearVelocity{};
    Vec3 angularVelocity{};
    float inverseMass{};
    Vec3 inverseInertiaLocal{};
    Mat3 inverseInertiaWorld;
    float friction{};
    float restitution{};
    float linearDamping{};
    float angularDamping{};
    float gravityScale{1.0f};
    std::uint32_t layer{1u};
    std::uint32_t mask{0xFFFFFFFFu};
    bool isTrigger{};
    bool lockRotation{};
    bool awake{true};
    float sleepTimer{};
    Vec3 force{};
    Vec3 torque{};
    int proxy{-1};
    std::uint64_t userData{};
};

struct PhysicsSettings {
    Vec3 gravity{0.0f, -9.81f, 0.0f};
    int velocityIterations{10};
    float baumgarte{0.2f};
    float penetrationSlop{0.01f};
    float restitutionThreshold{1.0f};
    bool allowSleep{true};
    float sleepLinearSpeed{0.05f};
    float sleepAngularSpeed{0.08f};
    float sleepTime{0.5f};
    float maxLinearSpeed{120.0f};
    float maxAngularSpeed{60.0f};
};

struct RaycastHit {
    BodyId body{};
    Vec3 point{};
    Vec3 normal{};
    float distance{};
    std::uint64_t userData{};
};

struct ShapeCastHit {
    BodyId body{};
    float distance{};
    Vec3 point{};
    Vec3 normal{}; // from the hit surface toward the moving shape
    bool startPenetrating{};
    std::uint64_t userData{};
};

enum class ContactEventType : std::uint8_t { Begin, End };

struct ContactEvent {
    ContactEventType type{};
    BodyId a{};
    BodyId b{};
    bool trigger{};
    Vec3 point{};
    Vec3 normal{};
};

struct PhysicsStats {
    std::size_t bodies{};
    std::size_t awakeBodies{};
    std::size_t broadphasePairs{};
    std::size_t contactPairs{};
    std::size_t contactPoints{};
    int treeHeight{};
};

class PhysicsWorld {
public:
    explicit PhysicsWorld(PhysicsSettings settings = {}) : settings_(settings) {}

    // Returns a null handle for an invalid description (bad shape, non-finite pose, mass <= 0 for dynamics).
    BodyId CreateBody(const BodyDesc& desc);
    bool DestroyBody(BodyId id);
    Body* GetBody(BodyId id) { return bodies_.Get(id); }
    const Body* GetBody(BodyId id) const { return bodies_.Get(id); }
    std::size_t BodyCount() const { return bodies_.Size(); }

    void SetPose(BodyId id, const Pose& pose);
    void SetLinearVelocity(BodyId id, Vec3 velocity);
    void ApplyImpulse(BodyId id, Vec3 impulse, Vec3 worldPoint);
    void ApplyForce(BodyId id, Vec3 force);
    // Kinematic bodies: velocity that reaches `target` after dt (pushes dynamics properly).
    void MoveKinematic(BodyId id, const Pose& target, float dt);
    void WakeUp(BodyId id);

    void Step(float dt);

    bool Raycast(const Math::Ray& ray, float maxDistance, RaycastHit& hit, std::uint32_t mask = 0xFFFFFFFFu,
        BodyId ignore = {}, bool includeTriggers = false) const;
    // Sphere/capsule sweep; returns the closest hit along the motion.
    bool ShapeCast(const Shape& shape, const Pose& start, Vec3 motion, ShapeCastHit& hit,
        std::uint32_t mask = 0xFFFFFFFFu, BodyId ignore = {}, bool includeTriggers = false) const;
    void Overlap(const Shape& shape, const Pose& pose, std::vector<BodyId>& out, std::uint32_t mask = 0xFFFFFFFFu,
        bool includeTriggers = true) const;
    // Contact manifold between a query shape and one body (for depenetration).
    bool ContactWith(const Shape& shape, const Pose& pose, BodyId body, Manifold& manifold) const;

    const std::vector<ContactEvent>& Events() const { return events_; }
    const PhysicsStats& Stats() const { return stats_; }
    PhysicsSettings& Settings() { return settings_; }

    template <typename Fn>
    void ForEachBody(Fn&& fn) { bodies_.ForEach([&](Core::Handle, Body& body) { fn(body); }); }
    template <typename Fn>
    void ForEachBody(Fn&& fn) const { bodies_.ForEach([&](Core::Handle, const Body& body) { fn(body); }); }

private:
    struct CachedPoint {
        Vec3 position{};
        float normalImpulse{};
        float tangentImpulse[2]{};
    };
    struct ContactConstraint {
        Body* a{};
        Body* b{};
        Vec3 normal{};
        Vec3 tangent[2]{};
        float friction{};
        struct Point {
            Vec3 position{};
            Vec3 ra{}, rb{};
            float normalMass{}, tangentMass[2]{};
            float normalImpulse{}, tangentImpulse[2]{};
            float bias{};
            float penetration{};
        } points[4];
        int count{};
        std::uint64_t key{};
    };

    static std::uint64_t PairKey(BodyId a, BodyId b);
    bool ShouldCollide(const Body& a, const Body& b) const;
    void UpdateInertia(Body& body) const;
    void SolveContact(ContactConstraint& c) const;

    PhysicsSettings settings_;
    Core::SlotMap<Body> bodies_;
    DynamicAabbTree tree_;
    std::vector<BodyId> proxyToBody_;
    std::map<std::uint64_t, std::vector<CachedPoint>> contactCache_;
    std::set<std::uint64_t> touching_;
    std::map<std::uint64_t, std::pair<BodyId, BodyId>> touchingBodies_;
    std::vector<ContactEvent> events_;
    std::vector<ContactConstraint> constraints_;
    PhysicsStats stats_;
};

} // namespace Astral::Physics
