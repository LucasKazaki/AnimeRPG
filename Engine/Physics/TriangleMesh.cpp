#include "Engine/Physics/TriangleMesh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <utility>

namespace Astral::Physics {

using namespace Math;

namespace {

constexpr std::uint32_t kLeafSize = 4;
// Neighbours within ~2 degrees of coplanar count as flat.
constexpr float kFlatSine = 0.035f;

std::array<std::uint32_t, 3> PositionKey(Vec3 p) {
    std::array<std::uint32_t, 3> key{};
    // Bit-exact welding; +0 and -0 are the same position.
    const float values[3] = {p.x == 0.0f ? 0.0f : p.x, p.y == 0.0f ? 0.0f : p.y, p.z == 0.0f ? 0.0f : p.z};
    std::memcpy(key.data(), values, sizeof(values));
    return key;
}

AABB TriangleBounds(const TriangleMesh::Triangle& t) {
    AABB box;
    box.Expand(t.a);
    box.Expand(t.b);
    box.Expand(t.c);
    return box;
}

} // namespace

std::shared_ptr<const TriangleMesh> TriangleMesh::Create(const std::vector<Vec3>& vertices,
    const std::vector<std::uint32_t>& indices, std::string& error) {
    if (indices.size() % 3 != 0) {
        error = "triangle mesh index count is not a multiple of 3";
        return nullptr;
    }
    for (const Vec3& v : vertices) {
        if (!IsFinite(v)) {
            error = "triangle mesh has a non-finite vertex";
            return nullptr;
        }
    }
    // Weld identical positions so adjacency works across split (UV/normal) seams.
    std::map<std::array<std::uint32_t, 3>, std::uint32_t> welded;
    std::vector<std::uint32_t> remap(vertices.size());
    std::vector<Vec3> positions;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const auto inserted = welded.emplace(PositionKey(vertices[i]), static_cast<std::uint32_t>(positions.size()));
        if (inserted.second) positions.push_back(vertices[i]);
        remap[i] = inserted.first->second;
    }

    auto mesh = std::shared_ptr<TriangleMesh>(new TriangleMesh());
    std::vector<std::array<std::uint32_t, 3>> corners;
    for (std::size_t i = 0; i < indices.size(); i += 3) {
        if (indices[i] >= vertices.size() || indices[i + 1] >= vertices.size() || indices[i + 2] >= vertices.size()) {
            error = "triangle mesh index out of range";
            return nullptr;
        }
        const std::array<std::uint32_t, 3> c{remap[indices[i]], remap[indices[i + 1]], remap[indices[i + 2]]};
        if (c[0] == c[1] || c[1] == c[2] || c[0] == c[2]) continue;
        Triangle t;
        t.a = positions[c[0]];
        t.b = positions[c[1]];
        t.c = positions[c[2]];
        const Vec3 cross = Cross(t.b - t.a, t.c - t.a);
        const float longest = std::max({LengthSquared(t.b - t.a), LengthSquared(t.c - t.b), LengthSquared(t.a - t.c)});
        // Slivers (area negligible against the longest edge) have no stable normal.
        if (!(LengthSquared(cross) > 1.0e-10f * longest * longest) || LengthSquared(cross) < 1.0e-20f) continue;
        t.normal = Normalize(cross);
        mesh->triangles_.push_back(t);
        corners.push_back(c);
    }
    if (mesh->triangles_.empty()) {
        error = "triangle mesh has no non-degenerate triangles";
        return nullptr;
    }

    // Edge adjacency: an edge is active (can produce edge/vertex normals) when it
    // is a boundary, non-manifold or convex; flat and concave edges are internal.
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::vector<std::pair<std::uint32_t, int>>> edges;
    for (std::size_t t = 0; t < corners.size(); ++t) {
        for (int e = 0; e < 3; ++e) {
            const std::uint32_t u = corners[t][static_cast<std::size_t>(e)], v = corners[t][static_cast<std::size_t>((e + 1) % 3)];
            edges[{std::min(u, v), std::max(u, v)}].push_back({static_cast<std::uint32_t>(t), e});
        }
    }
    for (Triangle& t : mesh->triangles_) t.activeEdges = 0x7;
    for (const auto& entry : edges) {
        if (entry.second.size() != 2) continue;
        for (int side = 0; side < 2; ++side) {
            const auto self = entry.second[static_cast<std::size_t>(side)];
            const auto other = entry.second[static_cast<std::size_t>(1 - side)];
            Triangle& triangle = mesh->triangles_[self.first];
            const auto& otherCorners = corners[other.first];
            // The neighbour's vertex that is not on the shared edge.
            Vec3 opposite = positions[otherCorners[0]];
            for (std::uint32_t corner : otherCorners) {
                if (corner != entry.first.first && corner != entry.first.second) opposite = positions[corner];
            }
            const Vec3 p0 = positions[entry.first.first], p1 = positions[entry.first.second];
            const float reach = Distance(opposite, ClosestPointOnSegment(opposite, p0, p1));
            if (reach < 1.0e-12f) continue;
            const float sine = Dot(triangle.normal, opposite - p0) / reach;
            // Flat or concave (the neighbour rises in front of this face): internal.
            if (sine > -kFlatSine) triangle.activeEdges = static_cast<std::uint8_t>(triangle.activeEdges & ~(1u << self.second));
        }
    }

    const std::uint32_t count = static_cast<std::uint32_t>(mesh->triangles_.size());
    mesh->triangleBounds_.reserve(count);
    for (const Triangle& t : mesh->triangles_) mesh->triangleBounds_.push_back(TriangleBounds(t));
    mesh->nodes_.reserve(2 * static_cast<std::size_t>(count / kLeafSize + 1));
    mesh->Build(0, count, 1);
    return mesh;
}

std::uint32_t TriangleMesh::Build(std::uint32_t first, std::uint32_t count, int depth) {
    depth_ = std::max(depth_, depth);
    const std::uint32_t index = static_cast<std::uint32_t>(nodes_.size());
    nodes_.emplace_back();
    AABB bounds, centroids;
    for (std::uint32_t i = first; i < first + count; ++i) {
        bounds.Expand(triangleBounds_[i]);
        centroids.Expand(triangleBounds_[i].Center());
    }
    nodes_[index].bounds = bounds;
    if (count <= kLeafSize) {
        nodes_[index].first = first;
        nodes_[index].count = count;
        return index;
    }
    // Median split on the widest centroid axis keeps the tree balanced
    // (depth <= log2(triangles)), which bounds the traversal stacks.
    const Vec3 size = centroids.Size();
    const int axis = size.x >= size.y && size.x >= size.z ? 0 : (size.y >= size.z ? 1 : 2);
    std::vector<std::uint32_t> order(count);
    for (std::uint32_t i = 0; i < count; ++i) order[i] = first + i;
    const std::uint32_t half = count / 2;
    std::nth_element(order.begin(), order.begin() + half, order.end(), [&](std::uint32_t x, std::uint32_t y) {
        const float cx = Component(triangleBounds_[x].Center(), axis), cy = Component(triangleBounds_[y].Center(), axis);
        return cx != cy ? cx < cy : x < y;
    });
    std::vector<Triangle> triangles(count);
    std::vector<AABB> boxes(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        triangles[i] = triangles_[order[i]];
        boxes[i] = triangleBounds_[order[i]];
    }
    std::copy(triangles.begin(), triangles.end(), triangles_.begin() + first);
    std::copy(boxes.begin(), boxes.end(), triangleBounds_.begin() + first);
    Build(first, half, depth + 1);
    const std::uint32_t right = Build(first + half, count - half, depth + 1);
    nodes_[index].first = right;
    nodes_[index].count = 0;
    return index;
}

bool TriangleMesh::Raycast(const Ray& ray, float maxDistance, float& t, Vec3& normal, std::size_t* triangle) const {
    if (!IsFinite(ray.origin) || !IsFinite(ray.direction) || !(maxDistance > 0.0f)) return false;
    float best = maxDistance;
    bool hit = false;
    std::uint32_t stack[64];
    int top = 0;
    stack[top++] = 0;
    while (top > 0) {
        const std::uint32_t index = stack[--top];
        const Node& node = nodes_[index];
        float entry;
        if (!IntersectRayAABB(ray, node.bounds, best, entry)) continue;
        if (node.count == 0) {
            stack[top++] = node.first;
            stack[top++] = index + 1;
            continue;
        }
        for (std::uint32_t i = node.first; i < node.first + node.count; ++i) {
            const Triangle& tri = triangles_[i];
            float hitT, u, v;
            if (IntersectRayTriangle(ray, tri.a, tri.b, tri.c, best, hitT, u, v) && (!hit || hitT < best)) {
                best = hitT;
                hit = true;
                normal = Dot(tri.normal, ray.direction) > 0.0f ? -tri.normal : tri.normal;
                if (triangle) *triangle = i;
            }
        }
    }
    if (hit) t = best;
    return hit;
}

} // namespace Astral::Physics
