#pragma once

// Core scene components and the cycle-safe transform hierarchy.

#include "Engine/Math/VectorMath.h"
#include "Engine/World/Registry.h"

#include <cstddef>
#include <string>
#include <vector>

namespace Astral::World {

struct Name {
    std::string value;
};

struct LocalTransform {
    Math::TRS trs;
};

// Written by UpdateTransforms(); read by rendering, physics sync, audio.
struct WorldTransform {
    Math::TRS trs;
    Math::Mat4 matrix = Math::Mat4::Identity();
};

// Intrusive child list: O(1) attach/detach, no per-node allocation.
struct Hierarchy {
    Entity parent{};
    Entity firstChild{};
    Entity nextSibling{};
    Entity previousSibling{};
    std::uint32_t childCount{};
};

// Attaches child under parent (null parent detaches). Rejects invalid entities,
// self-parenting and any assignment that would create a cycle.
bool SetParent(Registry& registry, Entity child, Entity parent);
Entity GetParent(const Registry& registry, Entity entity);
std::vector<Entity> GetChildren(const Registry& registry, Entity entity);
// True when `ancestor` is on entity's parent chain.
bool IsAncestor(const Registry& registry, Entity ancestor, Entity entity);
// Destroys an entity and all descendants.
void DestroyRecursive(Registry& registry, Entity entity);

struct TransformUpdateStats {
    std::size_t updated{};
    std::size_t maxDepth{};
};

// Propagates LocalTransform to WorldTransform for every entity, parents before
// children, iteratively (no recursion limit). Entities without LocalTransform
// act as identity.
TransformUpdateStats UpdateTransforms(Registry& registry);

} // namespace Astral::World
