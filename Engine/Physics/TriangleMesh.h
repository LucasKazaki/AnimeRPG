#pragma once

// Static triangle-mesh collision geometry (the role of Unity's non-convex
// MeshCollider and Unreal's complex collision): an immutable, shareable triangle
// soup with welded vertices, per-edge adjacency and a bounding volume hierarchy.
// Edges shared by coplanar or concave neighbours are flagged "internal" so
// contacts against them use the face normal, which stops rolling and sliding
// bodies from catching on the seams between triangles.

#include "Engine/Math/Geometry.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Astral::Physics {

class TriangleMesh {
public:
    struct Triangle {
        Math::Vec3 a, b, c;
        Math::Vec3 normal;   // Normalize(Cross(b - a, c - a)): the front face
        std::uint8_t activeEdges{}; // bit i: edge i (ab, bc, ca) may produce edge normals
    };
    struct Node {
        Math::AABB bounds;
        std::uint32_t first{}; // leaf: first triangle; inner: right child (left is index + 1)
        std::uint32_t count{}; // triangles in a leaf, 0 for inner nodes
    };

    // Builds collision geometry from indexed triangles. Degenerate triangles are
    // dropped; the result is null (with `error`) when nothing usable remains,
    // an index is out of range, or a vertex is not finite.
    static std::shared_ptr<const TriangleMesh> Create(const std::vector<Math::Vec3>& vertices,
        const std::vector<std::uint32_t>& indices, std::string& error);

    const Math::AABB& Bounds() const { return nodes_.front().bounds; }
    std::size_t TriangleCount() const { return triangles_.size(); }
    const Triangle& GetTriangle(std::size_t index) const { return triangles_[index]; }
    const std::vector<Node>& Nodes() const { return nodes_; }
    int Depth() const { return depth_; }

    // Double-sided ray cast in mesh space; `normal` faces the ray origin.
    bool Raycast(const Math::Ray& ray, float maxDistance, float& t, Math::Vec3& normal, std::size_t* triangle = nullptr) const;

    // Calls fn(triangleIndex) for triangles whose bounds overlap `box`; fn
    // returns false to stop early.
    template <typename Fn>
    void Query(const Math::AABB& box, Fn&& fn) const {
        std::uint32_t stack[64];
        int top = 0;
        stack[top++] = 0;
        while (top > 0) {
            const Node& node = nodes_[stack[--top]];
            if (!node.bounds.Overlaps(box)) continue;
            if (node.count > 0) {
                for (std::uint32_t i = node.first; i < node.first + node.count; ++i) {
                    if (triangleBounds_[i].Overlaps(box) && !fn(static_cast<std::size_t>(i))) return;
                }
            } else {
                const std::uint32_t index = static_cast<std::uint32_t>(&node - nodes_.data());
                stack[top++] = node.first;
                stack[top++] = index + 1;
            }
        }
    }

private:
    TriangleMesh() = default;
    std::uint32_t Build(std::uint32_t first, std::uint32_t count, int depth);

    std::vector<Triangle> triangles_;
    std::vector<Math::AABB> triangleBounds_;
    std::vector<Node> nodes_;
    int depth_{};
};

} // namespace Astral::Physics
