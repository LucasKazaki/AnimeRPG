#pragma once

// Native gameplay scripting (the role of Unity's MonoBehaviour and Unreal's
// actor components with Blueprint events). Subclass Behaviour, override the
// lifecycle and collision callbacks, and attach instances to entities with
// GameWorld::AddBehaviour. Registering a type with BehaviourRegistry (plus its
// reflected properties) lets scenes and prefabs create it by name:
//
//   struct Spinner : Framework::Behaviour {
//       float degreesPerSecond{90.0f};
//       void OnUpdate(float dt) override { ... }
//   };
//   BehaviourRegistry::Global().Register<Spinner>("Spinner")
//       .Field("degreesPerSecond", &Spinner::degreesPerSecond);
//
// Order per frame (see GameWorld::Tick): OnStart for new behaviours, then per
// fixed step OnFixedUpdate -> physics -> OnCollision*/OnTrigger*, then timers,
// OnUpdate, animation, OnLateUpdate. OnCreate runs when a behaviour is added
// (after the whole scene or prefab it belongs to exists); OnDestroy runs when
// its entity is destroyed at the end of the frame.

#include "Engine/Core/Json.h"
#include "Engine/Core/Reflection.h"
#include "Engine/Framework/Timers.h"
#include "Engine/Math/VectorMath.h"
#include "Engine/World/Registry.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace Astral::Framework {

class GameWorld;
using World::Entity;

struct CollisionInfo {
    Entity other{};
    Math::Vec3 point{};
    Math::Vec3 normal{}; // points from this entity toward the other
    bool trigger{};
};

class Behaviour {
public:
    virtual ~Behaviour() = default;

    virtual void OnCreate() {}
    virtual void OnStart() {}
    virtual void OnFixedUpdate(float) {}
    virtual void OnUpdate(float) {}
    virtual void OnLateUpdate(float) {}
    virtual void OnDestroy() {}
    virtual void OnCollisionEnter(const CollisionInfo&) {}
    virtual void OnCollisionExit(const CollisionInfo&) {}
    virtual void OnTriggerEnter(const CollisionInfo&) {}
    virtual void OnTriggerExit(const CollisionInfo&) {}

    // Disabled behaviours receive no update or collision callbacks.
    bool enabled{true};

    GameWorld& World() const { return *world_; }
    Entity Self() const { return entity_; }
    // Registered type name ("" for behaviours added from code by type).
    const std::string& TypeName() const { return typeName_; }
    bool Started() const { return started_; }

    // Timers owned by this entity: cancelled automatically when it is destroyed.
    TimerHandle Invoke(float delay, std::function<void()> callback);
    TimerHandle InvokeRepeating(float delay, float interval, std::function<void()> callback);

private:
    friend class GameWorld;
    GameWorld* world_{};
    Entity entity_{};
    std::string typeName_;
    bool created_{};
    bool started_{};
    bool destroyed_{};
};

// Named behaviour types with reflected properties (scene/prefab authoring).
class BehaviourRegistry {
public:
    static BehaviourRegistry& Global();

    template <typename T>
    Core::TypeBuilder<T> Register(const std::string& name) {
        static_assert(std::is_base_of_v<Behaviour, T>, "registered behaviours derive from Behaviour");
        Entry entry;
        entry.create = [] { return std::static_pointer_cast<Behaviour>(std::make_shared<T>()); };
        entry.object = [](Behaviour* behaviour) { return static_cast<void*>(static_cast<T*>(behaviour)); };
        entry.constObject = [](const Behaviour* behaviour) { return static_cast<const void*>(static_cast<const T*>(behaviour)); };
        entries_[name] = std::move(entry);
        return types_.Register<T>(name);
    }

    bool Has(const std::string& name) const { return entries_.count(name) > 0; }
    std::vector<std::string> Names() const;
    // Creates an instance and applies `properties` (a JSON object; unknown or
    // mistyped properties fail). Returns null with `error` on failure.
    std::shared_ptr<Behaviour> Create(const std::string& name, const Core::JsonValue& properties, std::string& error) const;
    // Checks `properties` without creating anything.
    bool Validate(const std::string& name, const Core::JsonValue& properties, std::string& error) const;
    // Reflected properties of a registered behaviour instance (null object when unregistered).
    Core::JsonValue Properties(const Behaviour& behaviour) const;
    const Core::TypeInfo* Info(const std::string& name) const { return types_.Find(name); }

private:
    struct Entry {
        std::function<std::shared_ptr<Behaviour>()> create;
        std::function<void*(Behaviour*)> object;
        std::function<const void*(const Behaviour*)> constObject;
    };
    std::map<std::string, Entry> entries_;
    Core::TypeRegistry types_;
};

} // namespace Astral::Framework
