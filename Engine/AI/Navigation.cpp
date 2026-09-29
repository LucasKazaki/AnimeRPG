#include "Engine/AI/Navigation.h"

#include "Engine/Physics/PhysicsWorld.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <queue>

namespace Astral::AI {

using namespace Math;

NavGrid::NavGrid(const NavGridDesc& desc) : desc_(desc) {
    desc_.width = std::clamp(desc_.width, 1, 4096);
    desc_.depth = std::clamp(desc_.depth, 1, 4096);
    if (!(desc_.cellSize > 0.0f) || !std::isfinite(desc_.cellSize)) desc_.cellSize = 0.5f;
    walkable_.assign(static_cast<std::size_t>(desc_.width) * static_cast<std::size_t>(desc_.depth), 1);
}

bool NavGrid::Walkable(int x, int z) const {
    return InBounds(x, z) && walkable_[static_cast<std::size_t>(z) * static_cast<std::size_t>(desc_.width) + static_cast<std::size_t>(x)] != 0;
}

void NavGrid::SetWalkable(int x, int z, bool walkable) {
    if (InBounds(x, z)) walkable_[static_cast<std::size_t>(z) * static_cast<std::size_t>(desc_.width) + static_cast<std::size_t>(x)] = walkable ? 1 : 0;
}

bool NavGrid::WorldToCell(Vec3 p, int& x, int& z) const {
    x = z = -1;
    if (!IsFinite(p)) return false;
    x = static_cast<int>(std::floor((p.x - desc_.origin.x) / desc_.cellSize));
    z = static_cast<int>(std::floor((p.z - desc_.origin.z) / desc_.cellSize));
    return InBounds(x, z);
}

Vec3 NavGrid::CellCenter(int x, int z) const {
    return {desc_.origin.x + (static_cast<float>(x) + 0.5f) * desc_.cellSize, desc_.origin.y,
        desc_.origin.z + (static_cast<float>(z) + 0.5f) * desc_.cellSize};
}

void NavGrid::BlockBox(const AABB& box, float agentRadius) {
    if (!box.IsValid()) return;
    const float r = std::max(0.0f, agentRadius);
    int x0 = 0, z0 = 0, x1 = 0, z1 = 0;
    WorldToCell({box.min.x - r, 0, box.min.z - r}, x0, z0);
    WorldToCell({box.max.x + r, 0, box.max.z + r}, x1, z1);
    x0 = std::clamp(x0, 0, desc_.width - 1);
    z0 = std::clamp(z0, 0, desc_.depth - 1);
    x1 = std::clamp(x1, 0, desc_.width - 1);
    z1 = std::clamp(z1, 0, desc_.depth - 1);
    for (int z = z0; z <= z1; ++z) {
        for (int x = x0; x <= x1; ++x) {
            const Vec3 c = CellCenter(x, z);
            // Distance from the cell centre to the box footprint (rounded clearance).
            const float dx = std::max({box.min.x - c.x, 0.0f, c.x - box.max.x});
            const float dz = std::max({box.min.z - c.z, 0.0f, c.z - box.max.z});
            if (dx * dx + dz * dz <= r * r) SetWalkable(x, z, false);
        }
    }
}

void NavGrid::BakeFromPhysics(const Physics::PhysicsWorld& world, float agentRadius, float minHeight, float maxHeight,
    std::uint32_t mask) {
    const float low = desc_.origin.y + minHeight, high = desc_.origin.y + maxHeight;
    world.ForEachBody([&](const Physics::Body& body) {
        if (body.type != Physics::BodyType::Static || body.isTrigger || !(body.layer & mask)) return;
        const AABB bounds = body.shape.WorldBounds(body.pose);
        if (bounds.max.y < low || bounds.min.y > high) return;
        BlockBox(bounds, agentRadius);
    });
}

bool NavGrid::NearestWalkable(Vec3 p, int& x, int& z, int maxRadius) const {
    if (!IsFinite(p)) return false;
    int cx = 0, cz = 0;
    WorldToCell(p, cx, cz);
    cx = std::clamp(cx, 0, desc_.width - 1);
    cz = std::clamp(cz, 0, desc_.depth - 1);
    if (Walkable(cx, cz)) {
        x = cx;
        z = cz;
        return true;
    }
    for (int radius = 1; radius <= maxRadius; ++radius) {
        float best = 1.0e30f;
        bool found = false;
        for (int dz = -radius; dz <= radius; ++dz) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (std::max(std::abs(dx), std::abs(dz)) != radius || !Walkable(cx + dx, cz + dz)) continue;
                const float d = DistanceSquared(CellCenter(cx + dx, cz + dz), Vec3{p.x, desc_.origin.y, p.z});
                if (d < best) {
                    best = d;
                    x = cx + dx;
                    z = cz + dz;
                    found = true;
                }
            }
        }
        if (found) return true;
    }
    return false;
}

bool NavGrid::LineWalkable(Vec3 a, Vec3 b) const {
    // Amanatides-Woo traversal over the cells the segment crosses.
    int x = 0, z = 0, endX = 0, endZ = 0;
    if (!WorldToCell(a, x, z) || !WorldToCell(b, endX, endZ)) return false;
    const float dx = b.x - a.x, dz = b.z - a.z;
    const int stepX = dx > 0 ? 1 : -1, stepZ = dz > 0 ? 1 : -1;
    const float fx = (a.x - desc_.origin.x) / desc_.cellSize, fz = (a.z - desc_.origin.z) / desc_.cellSize;
    const float tDeltaX = dx != 0.0f ? std::fabs(desc_.cellSize / dx) : 1.0e30f;
    const float tDeltaZ = dz != 0.0f ? std::fabs(desc_.cellSize / dz) : 1.0e30f;
    float tMaxX = dx != 0.0f ? ((stepX > 0 ? std::floor(fx) + 1.0f - fx : fx - std::floor(fx)) * tDeltaX) : 1.0e30f;
    float tMaxZ = dz != 0.0f ? ((stepZ > 0 ? std::floor(fz) + 1.0f - fz : fz - std::floor(fz)) * tDeltaZ) : 1.0e30f;
    for (int guard = 0; guard < desc_.width + desc_.depth + 4; ++guard) {
        if (!Walkable(x, z)) return false;
        if (x == endX && z == endZ) return true;
        if (tMaxX < tMaxZ) {
            tMaxX += tDeltaX;
            x += stepX;
        } else if (tMaxZ < tMaxX) {
            tMaxZ += tDeltaZ;
            z += stepZ;
        } else {
            // Exactly through a corner: both neighbours must be open (no corner cutting).
            if (!Walkable(x + stepX, z) || !Walkable(x, z + stepZ)) return false;
            tMaxX += tDeltaX;
            tMaxZ += tDeltaZ;
            x += stepX;
            z += stepZ;
        }
    }
    return false;
}

PathResult NavGrid::FindPath(Vec3 start, Vec3 goal, int maxExpanded, bool smooth) const {
    PathResult result;
    int sx = 0, sz = 0, gx = 0, gz = 0;
    if (!NearestWalkable(start, sx, sz) || !NearestWalkable(goal, gx, gz)) return result;
    const int width = desc_.width;
    const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(desc_.depth);
    std::vector<float> cost(count, 1.0e30f);
    std::vector<int> parent(count, -1);
    std::vector<std::uint8_t> closed(count, 0);
    auto index = [width](int x, int z) { return z * width + x; };
    auto heuristic = [&](int x, int z) {
        const float dx = static_cast<float>(std::abs(x - gx)), dz = static_cast<float>(std::abs(z - gz));
        return (dx + dz) + (1.41421356f - 2.0f) * std::min(dx, dz); // octile
    };
    using Item = std::pair<float, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> open;
    cost[static_cast<std::size_t>(index(sx, sz))] = 0.0f;
    open.push({heuristic(sx, sz), index(sx, sz)});
    const int dirs[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    int goalIndex = -1;
    while (!open.empty()) {
        const int current = open.top().second;
        open.pop();
        if (closed[static_cast<std::size_t>(current)]) continue;
        closed[static_cast<std::size_t>(current)] = 1;
        if (++result.expanded > maxExpanded) break;
        const int cx = current % width, cz = current / width;
        if (cx == gx && cz == gz) {
            goalIndex = current;
            break;
        }
        for (const auto& d : dirs) {
            const int nx = cx + d[0], nz = cz + d[1];
            if (!Walkable(nx, nz)) continue;
            const bool diagonal = d[0] != 0 && d[1] != 0;
            if (diagonal && (!Walkable(cx + d[0], cz) || !Walkable(cx, cz + d[1]))) continue;
            const int next = index(nx, nz);
            const float candidate = cost[static_cast<std::size_t>(current)] + (diagonal ? 1.41421356f : 1.0f);
            if (candidate < cost[static_cast<std::size_t>(next)]) {
                cost[static_cast<std::size_t>(next)] = candidate;
                parent[static_cast<std::size_t>(next)] = current;
                open.push({candidate + heuristic(nx, nz), next});
            }
        }
    }
    if (goalIndex < 0) return result;
    std::vector<Vec3> cells;
    for (int at = goalIndex; at >= 0; at = parent[static_cast<std::size_t>(at)]) cells.push_back(CellCenter(at % width, at / width));
    std::reverse(cells.begin(), cells.end());
    const Vec3 startPoint{start.x, desc_.origin.y, start.z};
    const Vec3 goalPoint{goal.x, desc_.origin.y, goal.z};
    // Use the exact endpoints when they lie on walkable cells (otherwise the projected cells).
    int cx, cz;
    if (WorldToCell(start, cx, cz) && Walkable(cx, cz)) cells.front() = startPoint;
    if (WorldToCell(goal, cx, cz) && Walkable(cx, cz)) cells.back() = goalPoint;
    if (!smooth || cells.size() <= 2) {
        result.points = cells;
    } else {
        // String pulling: skip every waypoint that has direct line of sight.
        result.points.push_back(cells.front());
        std::size_t anchor = 0;
        while (anchor + 1 < cells.size()) {
            std::size_t furthest = anchor + 1;
            for (std::size_t probe = cells.size() - 1; probe > anchor + 1; --probe) {
                if (LineWalkable(cells[anchor], cells[probe])) {
                    furthest = probe;
                    break;
                }
            }
            result.points.push_back(cells[furthest]);
            anchor = furthest;
        }
    }
    result.found = true;
    return result;
}

int NavGrid::WalkableCount() const {
    int count = 0;
    for (std::uint8_t w : walkable_) count += w != 0;
    return count;
}

} // namespace Astral::AI
