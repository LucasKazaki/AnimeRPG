#pragma once

// Sequencer integration: drives Animation::TimelinePlayer tracks on a
// GameWorld (the role of Unreal's Level Sequence actor and Unity's
// PlayableDirector). Bindings are entity names.
//
//   vector tracks   : position, worldPosition, scale, light.color, renderer.tint
//   rotation tracks : rotation
//   float tracks    : light.intensity, light.radius, renderer.opacity, camera.fieldOfView
//   activation      : MeshRenderer visibility, Camera activity, particle emission
//                     and the entity's other behaviours
//   events          : published on GameWorld::Events() as TimelineNotification

#include "Engine/Animation/Timeline.h"
#include "Engine/Framework/GameWorld.h"

#include <cstddef>
#include <map>
#include <memory>
#include <string>

namespace Astral::Framework {

struct TimelineNotification {
    std::string binding;
    std::string name;
    std::string payload;
};

class WorldTimelineBinder : public Animation::TimelineBinder {
public:
    explicit WorldTimelineBinder(GameWorld& world) : world_(world) {}

    void SetFloat(const std::string& binding, const std::string& property, float value) override;
    void SetVector(const std::string& binding, const std::string& property, Math::Vec3 value) override;
    void SetRotation(const std::string& binding, const std::string& property, Math::Quat value) override;
    void SetActive(const std::string& binding, bool active) override;
    void OnEvent(const std::string& binding, const Animation::TimelineEvent& event) override;

    // Entity for a binding name (cached, re-resolved when the entity dies).
    Entity Resolve(const std::string& binding);
    // Lookups that found no entity or no matching property (for diagnostics).
    std::size_t misses{};

private:
    GameWorld& world_;
    std::map<std::string, Entity> cache_;
};

// Plays a timeline on the world: set `timeline` from code, or `asset` to a
// JSON timeline loaded through the world's asset manager.
class TimelineBehaviour : public Behaviour {
public:
    std::string asset;
    std::string wrap{"Once"}; // Once, Hold, Loop, PingPong
    float speed{1.0f};
    bool playOnStart{true};
    std::shared_ptr<const Animation::TimelineAsset> timeline;
    std::string error; // why the asset could not be loaded

    Animation::TimelinePlayer* Player() { return player_.get(); }
    void OnStart() override;
    void OnUpdate(float dt) override;

private:
    std::unique_ptr<WorldTimelineBinder> binder_;
    std::unique_ptr<Animation::TimelinePlayer> player_;
};

// Registers the framework's built-in behaviours ("Timeline").
void RegisterFrameworkBehaviours(BehaviourRegistry& registry = BehaviourRegistry::Global());

} // namespace Astral::Framework
