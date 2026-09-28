#include "Engine/World/Registry.h"

namespace Astral::World {

Entity Registry::Create() {
    std::uint32_t index;
    if (!freeList_.empty()) {
        index = freeList_.back();
        freeList_.pop_back();
        alive_[index] = true;
    } else {
        index = static_cast<std::uint32_t>(generations_.size());
        generations_.push_back(1);
        alive_.push_back(true);
    }
    ++aliveCount_;
    return {index, generations_[index]};
}

void Registry::Destroy(Entity entity) {
    if (!Valid(entity)) return;
    for (auto& pool : pools_) {
        if (pool) pool->Remove(entity);
    }
    alive_[entity.index] = false;
    std::uint32_t& generation = generations_[entity.index];
    generation = generation == std::numeric_limits<std::uint32_t>::max() ? 1u : generation + 1u;
    freeList_.push_back(entity.index);
    --aliveCount_;
}

void Registry::ForEachEntity(const std::function<void(Entity)>& fn) const {
    for (std::uint32_t index = 0; index < generations_.size(); ++index) {
        if (alive_[index]) fn({index, generations_[index]});
    }
}

void Registry::Clear() {
    std::vector<Entity> live;
    ForEachEntity([&live](Entity e) { live.push_back(e); });
    for (Entity e : live) Destroy(e);
}

} // namespace Astral::World
