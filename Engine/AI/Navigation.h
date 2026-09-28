#pragma once

// Grid navigation (the role of UE's NavMesh for this engine's current scale):
// a walkability grid baked from the physics world with agent-radius
// clearance, 8-way A* with no corner cutting, line-of-sight path smoothing,
// nearest-walkable projection and bounded search budgets.

#include "Engine/Math/Geometry.h"

#include <cstdint>
#include <vector>

namespace Astral::Physics {
class PhysicsWorld;
}

namespace Astral::AI {

using Math::Vec3;

struct NavGridDesc {
    Vec3 origin{};          // minimum x/z corner (y is the floor height)
    float cellSize{0.5f};
    int width{64};          // cells along x
    int depth{64};          // cells along z
};

struct PathResult {
    bool found{};
    std::vector<Vec3> points; // world positions from start to goal
    int expanded{};
};

class NavGrid {
public:
    explicit NavGrid(const NavGridDesc& desc);

    // Marks cells whose centres fall inside the inflated footprint as blocked.
    void BlockBox(const Math::AABB& box, float agentRadius);
    // Blocks every static collider overlapping [floorY + minHeight, floorY + maxHeight].
    void BakeFromPhysics(const Physics::PhysicsWorld& world, float agentRadius, float minHeight = 0.1f,
        float maxHeight = 2.0f, std::uint32_t mask = 0xFFFFFFFFu);
    void SetWalkable(int x, int z, bool walkable);

    bool InBounds(int x, int z) const { return x >= 0 && z >= 0 && x < desc_.width && z < desc_.depth; }
    bool Walkable(int x, int z) const;
    bool WorldToCell(Vec3 p, int& x, int& z) const;
    Vec3 CellCenter(int x, int z) const;
    bool NearestWalkable(Vec3 p, int& x, int& z, int maxRadius = 8) const;
    // Grid DDA line-of-sight between two world points.
    bool LineWalkable(Vec3 a, Vec3 b) const;

    PathResult FindPath(Vec3 start, Vec3 goal, int maxExpanded = 20000, bool smooth = true) const;
    int WalkableCount() const;
    const NavGridDesc& Desc() const { return desc_; }

private:
    NavGridDesc desc_;
    std::vector<std::uint8_t> walkable_;
};

} // namespace Astral::AI
