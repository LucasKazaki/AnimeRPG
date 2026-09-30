#pragma once

// Runtime game world (the role of Unreal's UWorld and a loaded Unity scene):
// owns the entity registry and the simulation services, runs the frame in
// fixed tick phases, and keeps components and their runtime objects in sync.
//
// Tick(dt), per frame:
//   1. Start     OnStart for behaviours added since the last frame.
//   2. Fixed     0..maxFixedSteps steps of fixedDelta (accumulator):
//                OnFixedUpdate -> transforms -> push kinematic/teleported
//                bodies and move characters -> PhysicsWorld::Step -> pull
//                dynamic bodies -> OnCollision*/OnTrigger* callbacks.
//   3. Timers    TimerManager on scaled game time.
//   4. Update    OnUpdate.
//   5. Animate   animators, root motion, CPU skinning.
//   6. Late      OnLateUpdate (cameras follow here).
//   7. Present   transforms, particle emitters follow entities, audio voices
//                and listener follow, play-on-start sources and emitters.
//   8. Cleanup   deferred destroys (OnDestroy, bodies, voices, emitters,
//                timers), queued events dispatched.
// Destroy() is deferred to the end of the frame, like Unity's Destroy, so
// callbacks never observe half-removed entities. Everything is single-threaded
// on the game thread and deterministic for identical inputs.

#include "Engine/Core/Delegate.h"
#include "Engine/Framework/Behaviour.h"
#include "Engine/Framework/Components.h"
#include "Engine/Framework/Timers.h"
#include "Engine/Graphics/RenderScene.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/VFX/Particles.h"
#include "Engine/World/Components.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Astral::Assets {
class AssetManager;
}
namespace Astral::Input {
class InputSystem;
}
namespace Astral::UI {
class CanvasPanel;
}

namespace Astral::Framework {

struct GameWorldSettings {
    float fixedDelta{1.0f / 60.0f};
    int maxFixedSteps{5};       // per frame; excess time is dropped (no spiral of death)
    float maxFrameDelta{0.25f}; // longer frames (breakpoints, hitches) are clamped
    float timeScale{1.0f};
    Physics::PhysicsSettings physics;
};

struct GameWorldStats {
    std::uint64_t frame{};
    int fixedStepsLastFrame{};
    std::size_t entities{};
    std::size_t behaviours{};
    std::size_t bodies{};
    std::size_t destroyedLastFrame{};
};

// Published on Events() for every physics contact begin/end.
struct ContactNotification {
    Entity a{};
    Entity b{};
    bool begin{};
    bool trigger{};
    Math::Vec3 point{};
    Math::Vec3 normal{}; // from a toward b
};

// Published on Events() for animation notifies (the role of UE AnimNotifies).
struct AnimationNotification {
    Entity entity{};
    std::string name;
    std::string state;
};

struct EntityRaycastHit {
    Entity entity{};
    Math::Vec3 point{};
    Math::Vec3 normal{};
    float distance{};
};

class GameWorld {
public:
    // `audio` and `assets` are optional services owned by the host.
    explicit GameWorld(GameWorldSettings settings = {}, Audio::AudioMixer* audio = nullptr,
        Assets::AssetManager* assets = nullptr);
    // Destroys every entity (OnDestroy runs) before the services go away.
    ~GameWorld();
    GameWorld(const GameWorld&) = delete;
    GameWorld& operator=(const GameWorld&) = delete;

    // ---------------------------------------------------------------- entities
    Entity CreateEntity(const std::string& name = {}, const Math::TRS& local = {}, Entity parent = {});
    // Deferred to the end of the frame; destroys descendants too.
    void Destroy(Entity entity);
    // Immediate (OnDestroy runs now). Not for use inside Registry::Each.
    void DestroyImmediate(Entity entity);
    bool IsAlive(Entity entity) const { return registry_.Valid(entity) && !registry_.Has<PendingDestroy>(entity); }
    Entity Find(const std::string& name) const;
    std::vector<Entity> Roots() const;
    bool SetParent(Entity child, Entity parent);
    World::Registry& Registry() { return registry_; }
    const World::Registry& Registry() const { return registry_; }

    // Adds or replaces a component (a replaced component's runtime object is released).
    template <typename T, typename... Args>
    T& Add(Entity entity, Args&&... args) {
        if (T* existing = registry_.Get<T>(entity)) Release(entity, *existing);
        return registry_.Add<T>(entity, std::forward<Args>(args)...);
    }
    template <typename T>
    T* Get(Entity entity) { return registry_.Get<T>(entity); }
    template <typename T>
    const T* Get(Entity entity) const { return registry_.Get<T>(entity); }
    // Removes a component and releases its runtime object (body, voice, emitter...).
    template <typename T>
    void Remove(Entity entity) {
        if (T* component = registry_.Get<T>(entity)) Release(entity, *component);
        registry_.Remove<T>(entity);
    }

    // ---------------------------------------------------------------- behaviours
    template <typename T, typename... Args>
    T& AddBehaviour(Entity entity, Args&&... args) {
        auto behaviour = std::make_shared<T>(std::forward<Args>(args)...);
        T& reference = *behaviour;
        Attach(entity, std::move(behaviour), {});
        return reference;
    }
    // By registered name with reflected properties. Null with `error` on failure.
    Behaviour* AddBehaviour(Entity entity, const std::string& type, const Core::JsonValue& properties, std::string& error,
        const BehaviourRegistry& registry = BehaviourRegistry::Global());
    template <typename T>
    T* GetBehaviour(Entity entity) const {
        const Behaviours* behaviours = registry_.Get<Behaviours>(entity);
        if (!behaviours) return nullptr;
        for (const auto& behaviour : behaviours->list) {
            if (auto* typed = dynamic_cast<T*>(behaviour.get())) return typed;
        }
        return nullptr;
    }
    // While a batch is open, OnCreate is deferred until EndCreateBatch so a
    // scene's behaviours can find every entity of the scene (Unity's Awake).
    void BeginCreateBatch() { ++batchDepth_; }
    void EndCreateBatch();

    // ---------------------------------------------------------------- transforms
    Math::TRS GetLocal(Entity entity) const;
    void SetLocal(Entity entity, const Math::TRS& local);
    // World-space pose, current as of the last transform update or set.
    Math::TRS GetWorld(Entity entity) const;
    void SetWorldPosition(Entity entity, Math::Vec3 position);
    void SetWorldRotation(Entity entity, Math::Quat rotation);
    // Recomputes every WorldTransform now (Tick does this at each phase boundary).
    void UpdateTransforms();

    // ---------------------------------------------------------------- frame
    void Tick(float dt);
    float Time() const { return time_; }            // scaled game time
    float DeltaTime() const { return deltaTime_; }  // scaled, this frame
    std::uint64_t Frame() const { return stats_.frame; }
    const GameWorldStats& Stats() const { return stats_; }
    GameWorldSettings& Settings() { return settings_; }
    const GameWorldSettings& Settings() const { return settings_; }
    Environment& Env() { return environment_; }
    const Environment& Env() const { return environment_; }

    // ---------------------------------------------------------------- services
    Physics::PhysicsWorld& Physics() { return physics_; }
    const Physics::PhysicsWorld& Physics() const { return physics_; }
    VFX::ParticleWorld& Particles() { return particles_; }
    TimerManager& Timers() { return timers_; }
    Core::EventBus& Events() { return events_; }
    Audio::AudioMixer* AudioMixer() const { return audio_; }
    Assets::AssetManager* Assets() const { return assets_; }
    // Host services (optional, owned by the host, e.g. GameHost): action input
    // for behaviours, and the HUD layer their widgets go in (cleared with the scene).
    void SetInput(Input::InputSystem* input) { input_ = input; }
    Input::InputSystem* Input() const { return input_; }
    void SetHud(UI::CanvasPanel* hud) { hud_ = hud; }
    UI::CanvasPanel* Hud() const { return hud_; }

    Entity EntityFromBody(Physics::BodyId body) const;
    bool Raycast(const Math::Ray& ray, float maxDistance, EntityRaycastHit& hit, std::uint32_t mask = 0xFFFFFFFFu,
        Entity ignore = {}) const;
    // Plays a one-shot clip at a world position (fire and forget).
    Audio::VoiceId PlaySoundAt(const Audio::AudioClip* clip, Math::Vec3 position, float volume = 1.0f);
    // AudioSource / ParticleSystem control (restarts when already playing).
    bool PlayAudio(Entity entity);
    void StopAudio(Entity entity);
    bool PlayParticles(Entity entity);
    void StopParticles(Entity entity);

    // ---------------------------------------------------------------- rendering
    Entity ActiveCamera() const;
    // Fills `scene` (draws, lights, particles, environment) and `view` from the
    // active camera. Returns false (view untouched) when there is no camera.
    bool BuildRenderScene(Graphics::RenderScene& scene, Graphics::RenderView& view, int width, int height) const;

private:
    void Attach(Entity entity, std::shared_ptr<Behaviour> behaviour, const std::string& typeName);
    void Create(Behaviour& behaviour);
    std::vector<std::pair<Entity, std::shared_ptr<Behaviour>>> SnapshotBehaviours();
    void StartPending();
    void FixedStep(float dt);
    void SyncToPhysics(float dt);
    void SyncFromPhysics();
    void DispatchContacts();
    void UpdateAnimation(float dt);
    void UpdatePresentation();
    void FlushDestroyed();
    void DestroyNow(Entity entity);
    void EnsureBody(Entity entity, Collider& collider);
    void EnsureCharacter(Entity entity, CharacterMover& mover);
    void PushBodies(CharacterMover& mover, float dt);
    void WriteWorldPose(Entity entity, Math::Vec3 position, Math::Quat rotation);
    void RefreshWorld(Entity entity);
    void CollectSubtree(Entity root, std::vector<Entity>& out) const;
    template <typename T>
    std::vector<Entity> SortedEntities() {
        std::vector<Entity> entities = registry_.Pool<T>().Entities();
        std::sort(entities.begin(), entities.end());
        return entities;
    }

    void Release(Entity entity, Collider& collider);
    void Release(Entity entity, CharacterMover& mover);
    void Release(Entity entity, AudioSource& source);
    void Release(Entity entity, ParticleSystem& particles);
    template <typename T>
    void Release(Entity, T&) {}

    static std::uint64_t Pack(Entity entity) {
        return (static_cast<std::uint64_t>(entity.generation) << 32) | entity.index;
    }
    static Entity Unpack(std::uint64_t value) {
        return {static_cast<std::uint32_t>(value & 0xFFFFFFFFu), static_cast<std::uint32_t>(value >> 32)};
    }

    GameWorldSettings settings_;
    Environment environment_;
    World::Registry registry_;
    Physics::PhysicsWorld physics_;
    VFX::ParticleWorld particles_;
    TimerManager timers_;
    Core::EventBus events_;
    Audio::AudioMixer* audio_{};
    Assets::AssetManager* assets_{};
    Input::InputSystem* input_{};
    UI::CanvasPanel* hud_{};
    std::vector<std::weak_ptr<Behaviour>> pendingCreate_;
    std::vector<Entity> pendingDestroy_;
    int batchDepth_{};
    float accumulator_{};
    float time_{};
    float deltaTime_{};
    GameWorldStats stats_;
};

} // namespace Astral::Framework
