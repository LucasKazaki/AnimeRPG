#include "Engine/Framework/Behaviour.h"

#include "Engine/Framework/GameWorld.h"

#include <utility>

namespace Astral::Framework {

TimerHandle Behaviour::Invoke(float delay, std::function<void()> callback) {
    return world_ ? world_->Timers().Set(delay, std::move(callback), 0.0f, entity_) : TimerHandle{};
}

TimerHandle Behaviour::InvokeRepeating(float delay, float interval, std::function<void()> callback) {
    if (!world_ || !(interval > 0.0f)) return {};
    return world_->Timers().Set(delay, std::move(callback), interval, entity_);
}

BehaviourRegistry& BehaviourRegistry::Global() {
    static BehaviourRegistry registry;
    return registry;
}

std::vector<std::string> BehaviourRegistry::Names() const {
    std::vector<std::string> names;
    names.reserve(entries_.size());
    for (const auto& entry : entries_) names.push_back(entry.first);
    return names;
}

std::shared_ptr<Behaviour> BehaviourRegistry::Create(const std::string& name, const Core::JsonValue& properties,
    std::string& error) const {
    const auto found = entries_.find(name);
    const Core::TypeInfo* info = types_.Find(name);
    if (found == entries_.end() || !info) {
        error = "unknown behaviour type '" + name + "'";
        return nullptr;
    }
    if (!properties.IsNull() && !properties.IsObject()) {
        error = "properties of behaviour '" + name + "' must be an object";
        return nullptr;
    }
    std::shared_ptr<Behaviour> behaviour = found->second.create();
    if (properties.IsObject()) {
        std::string fieldError;
        if (!info->FromJson(found->second.object(behaviour.get()), properties, fieldError, true)) {
            error = "behaviour " + fieldError; // "behaviour Spinner: unknown field(s) ..."
            return nullptr;
        }
    }
    return behaviour;
}

bool BehaviourRegistry::Validate(const std::string& name, const Core::JsonValue& properties, std::string& error) const {
    return Create(name, properties, error) != nullptr;
}

Core::JsonValue BehaviourRegistry::Properties(const Behaviour& behaviour) const {
    const auto found = entries_.find(behaviour.TypeName());
    const Core::TypeInfo* info = types_.Find(behaviour.TypeName());
    if (found == entries_.end() || !info) return {};
    return info->ToJson(found->second.constObject(&behaviour));
}

} // namespace Astral::Framework
