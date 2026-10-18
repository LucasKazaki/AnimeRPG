#include "Engine/Framework/Sequencer.h"

#include "Engine/Assets/AssetManager.h"

#include <utility>

namespace Astral::Framework {

Entity WorldTimelineBinder::Resolve(const std::string& binding) {
    const auto cached = cache_.find(binding);
    if (cached != cache_.end() && world_.IsAlive(cached->second)) return cached->second;
    const Entity entity = world_.Find(binding);
    if (entity.IsNull()) {
        ++misses;
        cache_.erase(binding);
        return {};
    }
    cache_[binding] = entity;
    return entity;
}

void WorldTimelineBinder::SetFloat(const std::string& binding, const std::string& property, float value) {
    const Entity entity = Resolve(binding);
    if (entity.IsNull()) return;
    if (property == "light.intensity" || property == "light.radius") {
        if (Light* light = world_.Get<Light>(entity)) {
            (property == "light.intensity" ? light->intensity : light->radius) = value;
            return;
        }
    } else if (property == "renderer.opacity") {
        if (MeshRenderer* renderer = world_.Get<MeshRenderer>(entity)) {
            renderer->opacity = value;
            return;
        }
    } else if (property == "camera.fieldOfView") {
        if (Camera* camera = world_.Get<Camera>(entity)) {
            camera->fieldOfView = value;
            return;
        }
    }
    ++misses;
}

void WorldTimelineBinder::SetVector(const std::string& binding, const std::string& property, Math::Vec3 value) {
    const Entity entity = Resolve(binding);
    if (entity.IsNull()) return;
    if (property == "position" || property == "scale") {
        Math::TRS local = world_.GetLocal(entity);
        (property == "position" ? local.translation : local.scale) = value;
        world_.SetLocal(entity, local);
        return;
    }
    if (property == "worldPosition") {
        world_.SetWorldPosition(entity, value);
        return;
    }
    if (property == "light.color") {
        if (Light* light = world_.Get<Light>(entity)) {
            light->color = value;
            return;
        }
    } else if (property == "renderer.tint") {
        if (MeshRenderer* renderer = world_.Get<MeshRenderer>(entity)) {
            renderer->tint = value;
            return;
        }
    }
    ++misses;
}

void WorldTimelineBinder::SetRotation(const std::string& binding, const std::string& property, Math::Quat value) {
    const Entity entity = Resolve(binding);
    if (entity.IsNull()) return;
    if (property != "rotation") {
        ++misses;
        return;
    }
    Math::TRS local = world_.GetLocal(entity);
    local.rotation = value;
    world_.SetLocal(entity, local);
}

void WorldTimelineBinder::SetActive(const std::string& binding, bool active) {
    const Entity entity = Resolve(binding);
    if (entity.IsNull()) return;
    if (MeshRenderer* renderer = world_.Get<MeshRenderer>(entity)) renderer->visible = active;
    if (Camera* camera = world_.Get<Camera>(entity)) camera->active = active;
    if (world_.Get<ParticleSystem>(entity)) {
        if (active) world_.PlayParticles(entity);
        else world_.StopParticles(entity);
    }
    if (Behaviours* behaviours = world_.Get<Behaviours>(entity)) {
        for (const auto& behaviour : behaviours->list) {
            // Never switch off the player driving this timeline.
            if (!dynamic_cast<TimelineBehaviour*>(behaviour.get())) behaviour->enabled = active;
        }
    }
}

void WorldTimelineBinder::OnEvent(const std::string& binding, const Animation::TimelineEvent& event) {
    world_.Events().Publish(TimelineNotification{binding, event.name, event.payload});
}

void TimelineBehaviour::OnStart() {
    if (!timeline && !asset.empty()) {
        Assets::AssetManager* assets = World().Assets();
        if (!assets) {
            error = "timeline '" + asset + "' needs an asset manager";
        } else {
            const auto handle = assets->Load<Core::JsonValue>(asset);
            auto loaded = std::make_shared<Animation::TimelineAsset>();
            if (!handle.Ready()) {
                error = handle.Error();
            } else if (!loaded->FromJson(*handle.Get(), error)) {
                error = asset + ": " + error;
            } else {
                timeline = std::move(loaded);
            }
        }
    }
    if (!timeline) return;
    binder_ = std::make_unique<WorldTimelineBinder>(World());
    player_ = std::make_unique<Animation::TimelinePlayer>(timeline, binder_.get());
    if (wrap == "Hold") player_->wrap = Animation::TimelineWrap::Hold;
    else if (wrap == "Loop") player_->wrap = Animation::TimelineWrap::Loop;
    else if (wrap == "PingPong") player_->wrap = Animation::TimelineWrap::PingPong;
    player_->speed = speed;
    player_->Seek(speed >= 0.0f ? 0.0f : timeline->duration);
    if (playOnStart) player_->Play();
}

void TimelineBehaviour::OnUpdate(float dt) {
    if (player_) player_->Update(dt);
}

void RegisterFrameworkBehaviours(BehaviourRegistry& registry) {
    registry.Register<TimelineBehaviour>("Timeline")
        .Field("asset", &TimelineBehaviour::asset, "JSON timeline asset path")
        .Field("wrap", &TimelineBehaviour::wrap, "Once, Hold, Loop or PingPong")
        .Field("speed", &TimelineBehaviour::speed)
        .Field("playOnStart", &TimelineBehaviour::playOnStart);
}

} // namespace Astral::Framework
