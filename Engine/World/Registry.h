#pragma once

// Entity-component registry (sparse-set ECS). Entities are generational ids, so
// a destroyed entity's handle never aliases a new one. Each component type has a
// packed pool; iteration walks the smallest pool involved and checks the rest,
// giving cache-friendly systems with O(1) add/remove/lookup.
//
// Structural rule: do not add or remove components of the iterated types, or
// destroy entities, inside Each(); record them with a CommandBuffer instead.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace Astral::World {

struct Entity {
    std::uint32_t index{std::numeric_limits<std::uint32_t>::max()};
    std::uint32_t generation{0};

    bool IsNull() const { return index == std::numeric_limits<std::uint32_t>::max(); }
    friend bool operator==(Entity a, Entity b) { return a.index == b.index && a.generation == b.generation; }
    friend bool operator!=(Entity a, Entity b) { return !(a == b); }
    friend bool operator<(Entity a, Entity b) {
        return a.index != b.index ? a.index < b.index : a.generation < b.generation;
    }
};

inline constexpr Entity kNullEntity{};

namespace Detail {
inline std::uint32_t NextComponentTypeId() {
    static std::atomic<std::uint32_t> next{0};
    return next.fetch_add(1, std::memory_order_relaxed);
}
} // namespace Detail

template <typename T>
std::uint32_t ComponentTypeId() {
    static const std::uint32_t id = Detail::NextComponentTypeId();
    return id;
}

class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual bool Has(Entity entity) const = 0;
    virtual void Remove(Entity entity) = 0;
    virtual std::size_t Size() const = 0;
    virtual const std::vector<Entity>& Entities() const = 0;
};

template <typename T>
class ComponentPool final : public IComponentPool {
public:
    static constexpr std::uint32_t kAbsent = std::numeric_limits<std::uint32_t>::max();

    template <typename... Args>
    T& Emplace(Entity entity, Args&&... args) {
        if (entity.index >= sparse_.size()) sparse_.resize(static_cast<std::size_t>(entity.index) + 1, kAbsent);
        std::uint32_t& slot = sparse_[entity.index];
        if (slot != kAbsent) {
            dense_[slot] = T{std::forward<Args>(args)...};
            entities_[slot] = entity;
            return dense_[slot];
        }
        slot = static_cast<std::uint32_t>(dense_.size());
        entities_.push_back(entity);
        dense_.push_back(T{std::forward<Args>(args)...});
        return dense_.back();
    }

    bool Has(Entity entity) const override {
        return entity.index < sparse_.size() && sparse_[entity.index] != kAbsent
            && entities_[sparse_[entity.index]] == entity;
    }

    T* Get(Entity entity) { return Has(entity) ? &dense_[sparse_[entity.index]] : nullptr; }
    const T* Get(Entity entity) const { return Has(entity) ? &dense_[sparse_[entity.index]] : nullptr; }

    void Remove(Entity entity) override {
        if (!Has(entity)) return;
        const std::uint32_t slot = sparse_[entity.index];
        const std::uint32_t last = static_cast<std::uint32_t>(dense_.size() - 1);
        if (slot != last) {
            dense_[slot] = std::move(dense_[last]);
            entities_[slot] = entities_[last];
            sparse_[entities_[slot].index] = slot;
        }
        dense_.pop_back();
        entities_.pop_back();
        sparse_[entity.index] = kAbsent;
    }

    std::size_t Size() const override { return dense_.size(); }
    const std::vector<Entity>& Entities() const override { return entities_; }
    std::vector<T>& Components() { return dense_; }

private:
    std::vector<std::uint32_t> sparse_;
    std::vector<Entity> entities_;
    std::vector<T> dense_;
};

class Registry {
public:
    Entity Create();
    // Removes every component, then invalidates the handle.
    void Destroy(Entity entity);
    bool Valid(Entity entity) const {
        return !entity.IsNull() && entity.index < generations_.size() && alive_[entity.index]
            && generations_[entity.index] == entity.generation;
    }
    std::size_t AliveCount() const { return aliveCount_; }
    // Visits every live entity in index order.
    void ForEachEntity(const std::function<void(Entity)>& fn) const;
    void Clear();

    // Adds or replaces a component. Throws std::invalid_argument for a dead entity.
    template <typename T, typename... Args>
    T& Add(Entity entity, Args&&... args) {
        if (!Valid(entity)) throw std::invalid_argument("Registry::Add on an invalid entity");
        return Pool<T>().Emplace(entity, std::forward<Args>(args)...);
    }
    template <typename T>
    void Remove(Entity entity) {
        if (auto* pool = FindPool<T>()) pool->Remove(entity);
    }
    template <typename T>
    T* Get(Entity entity) {
        if (!Valid(entity)) return nullptr;
        auto* pool = FindPool<T>();
        return pool ? pool->Get(entity) : nullptr;
    }
    template <typename T>
    const T* Get(Entity entity) const {
        if (!Valid(entity)) return nullptr;
        const auto* pool = FindPool<T>();
        return pool ? pool->Get(entity) : nullptr;
    }
    template <typename T>
    bool Has(Entity entity) const { return Get<T>(entity) != nullptr; }

    template <typename T>
    ComponentPool<T>& Pool() {
        const std::uint32_t id = ComponentTypeId<T>();
        if (id >= pools_.size()) pools_.resize(static_cast<std::size_t>(id) + 1);
        if (!pools_[id]) pools_[id] = std::make_unique<ComponentPool<T>>();
        return static_cast<ComponentPool<T>&>(*pools_[id]);
    }

    template <typename T>
    std::size_t Count() const {
        const auto* pool = FindPool<T>();
        return pool ? pool->Size() : 0;
    }

    // fn(Entity, First&, Rest&...) for every entity holding all listed components.
    template <typename First, typename... Rest, typename Fn>
    void Each(Fn&& fn) {
        ComponentPool<First>* first = FindPool<First>();
        if (!first) return;
        const IComponentPool* smallest = first;
        bool missing = false;
        (void)std::initializer_list<int>{(SelectSmallest<Rest>(smallest, missing), 0)...};
        if (missing) return;
        // Copy: the callback may modify component values (not structure).
        const std::vector<Entity> entities = smallest->Entities();
        for (Entity entity : entities) {
            First* a = first->Get(entity);
            if (!a) continue;
            std::tuple<Rest*...> rest{FindPool<Rest>()->Get(entity)...};
            if (!AllPresent(rest, std::index_sequence_for<Rest...>{})) continue;
            Invoke(fn, entity, *a, rest, std::index_sequence_for<Rest...>{});
        }
    }

private:
    template <typename T>
    ComponentPool<T>* FindPool() {
        const std::uint32_t id = ComponentTypeId<T>();
        return id < pools_.size() ? static_cast<ComponentPool<T>*>(pools_[id].get()) : nullptr;
    }
    template <typename T>
    const ComponentPool<T>* FindPool() const {
        const std::uint32_t id = ComponentTypeId<T>();
        return id < pools_.size() ? static_cast<const ComponentPool<T>*>(pools_[id].get()) : nullptr;
    }
    template <typename T>
    void SelectSmallest(const IComponentPool*& smallest, bool& missing) {
        const ComponentPool<T>* pool = FindPool<T>();
        if (!pool) {
            missing = true;
            return;
        }
        if (pool->Size() < smallest->Size()) smallest = pool;
    }
    template <typename Tuple, std::size_t... I>
    static bool AllPresent(const Tuple& tuple, std::index_sequence<I...>) {
        bool present = true;
        (void)std::initializer_list<int>{(present = present && std::get<I>(tuple) != nullptr, 0)...};
        return present;
    }
    template <typename Fn, typename A, typename Tuple, std::size_t... I>
    static void Invoke(Fn& fn, Entity entity, A& a, Tuple& tuple, std::index_sequence<I...>) {
        fn(entity, a, *std::get<I>(tuple)...);
    }

    std::vector<std::uint32_t> generations_;
    std::vector<bool> alive_;
    std::vector<std::uint32_t> freeList_;
    std::size_t aliveCount_{};
    std::vector<std::unique_ptr<IComponentPool>> pools_;
};

// Deferred structural changes, applied in recording order by Flush().
class CommandBuffer {
public:
    void Destroy(Entity entity) { commands_.push_back([entity](Registry& r) { r.Destroy(entity); }); }
    template <typename T>
    void Add(Entity entity, T component) {
        commands_.push_back([entity, component](Registry& r) {
            if (r.Valid(entity)) r.Add<T>(entity, component);
        });
    }
    template <typename T>
    void Remove(Entity entity) { commands_.push_back([entity](Registry& r) { r.Remove<T>(entity); }); }
    std::size_t Size() const { return commands_.size(); }
    void Flush(Registry& registry) {
        for (auto& command : commands_) command(registry);
        commands_.clear();
    }

private:
    std::vector<std::function<void(Registry&)>> commands_;
};

} // namespace Astral::World
