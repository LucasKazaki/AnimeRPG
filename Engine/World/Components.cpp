#include "Engine/World/Components.h"

#include <algorithm>
#include <cstddef>

namespace Astral::World {

namespace {

void Detach(Registry& registry, Entity child) {
    Hierarchy* node = registry.Get<Hierarchy>(child);
    if (!node || node->parent.IsNull()) return;
    const Entity parent = node->parent;
    const Entity previous = node->previousSibling;
    const Entity next = node->nextSibling;
    if (Hierarchy* p = registry.Get<Hierarchy>(previous)) p->nextSibling = next;
    if (Hierarchy* n = registry.Get<Hierarchy>(next)) n->previousSibling = previous;
    if (Hierarchy* parentNode = registry.Get<Hierarchy>(parent)) {
        if (parentNode->firstChild == child) parentNode->firstChild = next;
        if (parentNode->childCount > 0) --parentNode->childCount;
    }
    node = registry.Get<Hierarchy>(child);
    node->parent = {};
    node->previousSibling = {};
    node->nextSibling = {};
}

} // namespace

bool IsAncestor(const Registry& registry, Entity ancestor, Entity entity) {
    if (!registry.Valid(ancestor) || !registry.Valid(entity)) return false;
    std::size_t guard = registry.AliveCount() + 1;
    const Hierarchy* node = registry.Get<Hierarchy>(entity);
    while (node && registry.Valid(node->parent) && guard-- > 0) {
        if (node->parent == ancestor) return true;
        node = registry.Get<Hierarchy>(node->parent);
    }
    return false;
}

bool SetParent(Registry& registry, Entity child, Entity parent) {
    if (!registry.Valid(child)) return false;
    if (!parent.IsNull()) {
        if (!registry.Valid(parent) || parent == child || IsAncestor(registry, child, parent)) return false;
    }
    if (!registry.Has<Hierarchy>(child)) registry.Add<Hierarchy>(child);
    if (!parent.IsNull() && !registry.Has<Hierarchy>(parent)) registry.Add<Hierarchy>(parent);
    Detach(registry, child);
    if (parent.IsNull()) return true;
    // Append to the tail so child order is insertion order (stable serialisation).
    Hierarchy* parentNode = registry.Get<Hierarchy>(parent);
    Entity tail = parentNode->firstChild;
    if (tail.IsNull()) {
        parentNode->firstChild = child;
    } else {
        std::size_t guard = registry.AliveCount() + 1;
        while (guard-- > 0) {
            const Hierarchy* tailNode = registry.Get<Hierarchy>(tail);
            if (!tailNode || !registry.Valid(tailNode->nextSibling)) break;
            tail = tailNode->nextSibling;
        }
        registry.Get<Hierarchy>(tail)->nextSibling = child;
    }
    ++parentNode->childCount;
    Hierarchy* node = registry.Get<Hierarchy>(child);
    node->parent = parent;
    node->previousSibling = parentNode->firstChild == child ? Entity{} : tail;
    node->nextSibling = {};
    return true;
}

Entity GetParent(const Registry& registry, Entity entity) {
    const Hierarchy* node = registry.Get<Hierarchy>(entity);
    return node && registry.Valid(node->parent) ? node->parent : Entity{};
}

std::vector<Entity> GetChildren(const Registry& registry, Entity entity) {
    std::vector<Entity> children;
    const Hierarchy* node = registry.Get<Hierarchy>(entity);
    if (!node) return children;
    Entity current = node->firstChild;
    std::size_t guard = registry.AliveCount() + 1;
    while (registry.Valid(current) && guard-- > 0) {
        children.push_back(current);
        const Hierarchy* childNode = registry.Get<Hierarchy>(current);
        if (!childNode) break;
        current = childNode->nextSibling;
    }
    return children;
}

void DestroyRecursive(Registry& registry, Entity entity) {
    if (!registry.Valid(entity)) return;
    Detach(registry, entity);
    std::vector<Entity> stack{entity};
    std::vector<Entity> doomed;
    while (!stack.empty()) {
        const Entity current = stack.back();
        stack.pop_back();
        doomed.push_back(current);
        for (Entity child : GetChildren(registry, current)) stack.push_back(child);
    }
    for (Entity e : doomed) registry.Destroy(e);
}

TransformUpdateStats UpdateTransforms(Registry& registry) {
    TransformUpdateStats stats;
    std::vector<Entity> roots;
    auto considerRoot = [&](Entity e) {
        if (!registry.Valid(GetParent(registry, e))) roots.push_back(e);
    };
    for (Entity e : registry.Pool<LocalTransform>().Entities()) considerRoot(e);
    for (Entity e : registry.Pool<Hierarchy>().Entities()) {
        if (!registry.Has<LocalTransform>(e)) considerRoot(e);
    }
    std::sort(roots.begin(), roots.end());
    struct Item {
        Entity entity;
        Math::TRS parentTrs;
        Math::Mat4 parentMatrix;
        std::size_t depth;
    };
    std::vector<Item> stack;
    for (Entity root : roots) {
        stack.push_back({root, Math::TRS{}, Math::Mat4::Identity(), 0});
        while (!stack.empty()) {
            const Item item = stack.back();
            stack.pop_back();
            const LocalTransform* local = registry.Get<LocalTransform>(item.entity);
            const Math::TRS localTrs = local ? local->trs : Math::TRS{};
            WorldTransform& world = registry.Has<WorldTransform>(item.entity)
                ? *registry.Get<WorldTransform>(item.entity) : registry.Add<WorldTransform>(item.entity);
            world.trs = Math::Combine(item.parentTrs, localTrs);
            world.matrix = item.parentMatrix * Math::ToMat4(localTrs);
            ++stats.updated;
            stats.maxDepth = std::max(stats.maxDepth, item.depth);
            const Math::TRS trs = world.trs;
            const Math::Mat4 matrix = world.matrix;
            const std::vector<Entity> children = GetChildren(registry, item.entity);
            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                stack.push_back({*it, trs, matrix, item.depth + 1});
            }
        }
    }
    return stats;
}

} // namespace Astral::World
