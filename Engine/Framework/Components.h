#pragma once

// Gameplay components (Unity components / Unreal actor components). They are
// plain data in the ECS registry; GameWorld's systems create and sync the
// runtime objects (physics bodies, audio voices, emitters, animators) they
// describe. Fields that scenes may author are reflected (FrameworkTypes());
// runtime handles are not serialised.
//
// Every entity a GameWorld creates has World::Name, World::LocalTransform and
// World::WorldTransform, plus World::Hierarchy when parented.

#include "Engine/Animation/Animator.h"
#include "Engine/Audio/AudioMixer.h"
#include "Engine/Core/Reflection.h"
#include "Engine/Graphics/Material.h"
#include "Engine/Graphics/Mesh.h"
#include "Engine/Graphics/MeshSimplify.h"
#include "Engine/Physics/CharacterController.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/VFX/Particles.h"
#include "Engine/World/Registry.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Astral::Framework {

using World::Entity;

// Draws a mesh with a material at the entity's world transform.
struct MeshRenderer {
    std::string meshRef;     // "primitive:cube", "models/crate.glb#0.0" (mesh.primitive)
    std::string materialRef; // scene material library name or "models/crate.glb#material0"
    Graphics::Color tint{1.0f, 1.0f, 1.0f};
    float opacity{1.0f};
    bool visible{true};
    std::uint32_t objectId{}; // 0 = derived from the entity
    bool autoLod{};           // scenes: generate simplified LODs for `mesh`
    int forcedLod{-1};        // >= 0 pins a level (debug views, cinematics)
    // Resolved resources (shared, immutable).
    std::shared_ptr<const Graphics::MeshData> mesh;
    std::shared_ptr<const Graphics::Material> material;
    std::shared_ptr<const Graphics::LodGroup> lods; // optional; level 0 replaces `mesh`
    mutable int currentLod{-1};                     // runtime, for hysteresis
};

enum class LightType : std::uint8_t { Directional, Point };

// Directional lights shine along the entity's +Z; the brightest is the sun.
struct Light {
    LightType type{LightType::Point};
    Graphics::Color color{1.0f, 1.0f, 1.0f};
    float intensity{1.0f};
    float radius{5.0f};
    bool castShadows{true};
};

// Looks along the entity's +Z. The active camera with the highest priority renders.
struct Camera {
    float fieldOfView{60.0f}; // vertical, degrees
    float nearPlane{0.25f};
    float farPlane{600.0f};
    int priority{};
    bool active{true};
};

enum class ColliderShape : std::uint8_t { Sphere, Capsule, Box, Mesh };

// A collider without a RigidBody is static level geometry. Sizes are scaled by
// the entity's world scale when the body is created.
struct Collider {
    ColliderShape shape{ColliderShape::Box};
    float radius{0.5f};                  // sphere, capsule
    float height{2.0f};                  // capsule, total including caps
    Math::Vec3 halfExtents{0.5f, 0.5f, 0.5f}; // box
    std::string meshRef;                 // mesh colliders; empty = the MeshRenderer's mesh
    float friction{0.6f};
    float restitution{0.05f};
    bool isTrigger{};
    std::uint32_t layer{1u};
    std::uint32_t mask{0xFFFFFFFFu};
    // Runtime. Mesh colliders use `triangles` (unit scale) or cook them from
    // `sourceMesh` (the resolved meshRef) or the MeshRenderer's mesh.
    std::shared_ptr<const Graphics::MeshData> sourceMesh;
    std::shared_ptr<const Physics::TriangleMesh> triangles;
    Physics::BodyId body{};
    bool failed{}; // the shape was invalid; no body is created
    Math::Vec3 syncedPosition{};
    Math::Quat syncedRotation{};
};

// Makes a Collider dynamic (simulated) or kinematic (moved by its transform,
// pushing dynamic bodies).
struct RigidBody {
    Physics::BodyType type{Physics::BodyType::Dynamic};
    float mass{1.0f};
    float linearDamping{0.02f};
    float angularDamping{0.05f};
    float gravityScale{1.0f};
    bool lockRotation{};
    Math::Vec3 velocity{}; // initial linear velocity
};

// Kinematic capsule movement (UE CharacterMovement / Unity CharacterController).
// Behaviours set desiredVelocity/jump; the fixed step moves the capsule and
// writes the entity position (the foot).
struct CharacterMover {
    float radius{0.35f};
    float height{1.8f};
    float stepHeight{0.4f};
    float maxSlopeDegrees{50.0f};
    float jumpSpeed{8.5f};
    float gravity{-24.0f};
    float groundAcceleration{60.0f};
    float airAcceleration{18.0f};
    std::uint32_t collisionMask{0xFFFFFFFFu};
    // Walking into a dynamic body pushes it toward the walking speed (UE's
    // physics interaction); bodies heavier than maxPushMass stay put.
    float pushStrength{1.0f};
    float maxPushMass{200.0f};
    Math::Vec3 desiredVelocity{};
    bool jump{};
    // Set launch (with launchVelocity) to launch the character on the next
    // fixed step (jump pads, knockback); both requests clear after the step.
    bool launch{};
    Math::Vec3 launchVelocity{};
    // Runtime: the controller, plus a kinematic capsule body so dynamic bodies
    // are pushed by the character and triggers see it.
    std::shared_ptr<Physics::CharacterController> controller;
    Physics::BodyId body{};
    bool Grounded() const { return controller && controller->Grounded(); }
};

struct AudioSource {
    std::string clipRef;
    float volume{1.0f};
    float pitch{1.0f};
    bool loop{};
    bool spatial{true};
    bool playOnStart{true};
    float minDistance{1.0f};
    float maxDistance{40.0f};
    Audio::Bus bus{Audio::Bus::Sfx};
    int priority{};
    // Runtime.
    std::shared_ptr<const Audio::AudioClip> clip;
    Audio::VoiceId voice{};
    bool started{};
};

// The mixer listens from this entity (else from the active camera).
struct AudioListener {
    bool active{true};
};

// A particle emitter that follows the entity.
struct ParticleSystem {
    float rate{20.0f};
    int burst{};
    float duration{-1.0f};
    float lifetimeMin{0.5f}, lifetimeMax{1.0f};
    Math::Vec3 direction{0.0f, 1.0f, 0.0f};
    float spreadDegrees{30.0f};
    float speedMin{1.0f}, speedMax{2.0f};
    float spawnRadius{};
    Math::Vec3 gravity{};
    float drag{};
    float sizeStart{0.1f}, sizeEnd{0.0f};
    Math::Vec4 colorStart{1.0f, 1.0f, 1.0f, 1.0f};
    Math::Vec4 colorEnd{1.0f, 1.0f, 1.0f, 0.0f};
    bool additive{true};
    int maxParticles{512};
    int seed{1};
    bool playOnStart{true};
    bool followEntity{true};
    // Runtime.
    VFX::EmitterId emitter{};
    bool started{};

    VFX::EmitterSettings ToSettings() const;
};

// Runs an animation state machine and, with a bind-pose mesh, CPU-skins it into
// the entity's MeshRenderer every frame. Set up from code (skeleton and
// machine are shared assets); only playback settings are serialised.
struct AnimatorComponent {
    float speed{1.0f};
    bool applyRootMotion{};
    std::shared_ptr<const Animation::Skeleton> skeleton;
    std::shared_ptr<const Animation::AnimStateMachine> machine;
    std::shared_ptr<const Graphics::MeshData> bindMesh; // optional skinned mesh in bind pose
    // Runtime.
    std::shared_ptr<Animation::Animator> animator;
    std::shared_ptr<Graphics::MeshData> skinned;
    std::vector<Math::Mat4> skinMatrices;
};

class Behaviour;
// Native script instances attached to an entity (see Behaviour.h).
struct Behaviours {
    std::vector<std::shared_ptr<Behaviour>> list;
};

// Entity scheduled for destruction at the end of the frame.
struct PendingDestroy {};

// Collision geometry from a render mesh, with `scale` baked into the vertices.
std::shared_ptr<const Physics::TriangleMesh> CookCollisionMesh(const Graphics::MeshData& mesh, Math::Vec3 scale,
    std::string& error);

// Reflected, scene-authorable types: "Transform", "MeshRenderer", "Light",
// "Camera", "Collider", "RigidBody", "CharacterMover", "AudioSource",
// "AudioListener", "ParticleSystem", "Animator", "Material", "Environment".
const Core::TypeRegistry& FrameworkTypes();

// Authoring view of a transform ("position", "rotation" as [x,y,z,w] or
// {"euler": [pitch, yaw, roll]} degrees, "scale").
struct TransformDesc {
    Math::Vec3 position{};
    Math::Quat rotation{};
    Math::Vec3 scale{1.0f, 1.0f, 1.0f};
};

// Scene-wide rendering environment.
struct Environment {
    Graphics::Color ambientSky{0.30f, 0.34f, 0.45f};
    Graphics::Color ambientGround{0.18f, 0.16f, 0.15f};
    bool fog{true};
    Graphics::Color fogColor{0.72f, 0.80f, 0.93f};
    float fogDensity{0.006f};
    float exposure{1.0f};
    bool shadows{true};
    float shadowRadius{60.0f};
    bool outlines{true};
    bool bloom{true};
    float lodBias{1.0f}; // multiplies screen size for LOD selection (> 1 keeps detail longer)
};

} // namespace Astral::Framework
