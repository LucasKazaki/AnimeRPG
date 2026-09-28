#pragma once

// Dynamic AABB tree (surface-area-heuristic insertion with AVL-style
// rotations), the broadphase used by Box2D/Bullet-class engines. Leaves hold
// "fat" boxes so small motions do not restructure the tree; queries and ray
// casts are O(log n) on average.

#include "Engine/Math/Geometry.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace Astral::Physics {

class DynamicAabbTree {
public:
    static constexpr int kNull = -1;

    int CreateProxy(const Math::AABB& box, std::uint32_t userData, float margin = 0.1f);
    void DestroyProxy(int proxy);
    // Re-inserts only when the tight box escapes the fat box. Returns true if re-inserted.
    bool MoveProxy(int proxy, const Math::AABB& box, Math::Vec3 displacement, float margin = 0.1f);

    const Math::AABB& FatBounds(int proxy) const { return nodes_[static_cast<std::size_t>(proxy)].box; }
    std::uint32_t UserData(int proxy) const { return nodes_[static_cast<std::size_t>(proxy)].userData; }

    // Callback returns false to stop early.
    void Query(const Math::AABB& box, const std::function<bool(int)>& callback) const;
    // Callback returns the new clip distance (0 terminates, <0 ignores the proxy).
    void RayCast(const Math::Ray& ray, float maxDistance, const std::function<float(int, float)>& callback) const;

    int Height() const;
    int ProxyCount() const { return proxyCount_; }
    // Structural invariants (parents, heights, containment); for tests.
    bool Validate() const;

private:
    struct Node {
        Math::AABB box;
        std::uint32_t userData{};
        int parent{kNull};
        int child1{kNull};
        int child2{kNull};
        int height{-1}; // -1 = free
        bool IsLeaf() const { return child1 == kNull; }
    };
    int Allocate();
    void Free(int node);
    void InsertLeaf(int leaf);
    void RemoveLeaf(int leaf);
    int Balance(int node);
    int ComputeHeight(int node) const;

    std::vector<Node> nodes_;
    int root_{kNull};
    int freeList_{kNull};
    int proxyCount_{};
};

} // namespace Astral::Physics
