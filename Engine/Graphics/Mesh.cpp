#include "Engine/Graphics/Mesh.h"

#include <algorithm>
#include <cmath>

namespace Astral::Graphics {

using namespace Math;

void MeshData::ComputeBounds() {
    bounds = AABB{};
    for (const Vertex& v : vertices) bounds.Expand(v.position);
}

void MeshData::RecomputeNormals() {
    for (Vertex& v : vertices) v.normal = {};
    for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
        Vertex& a = vertices[indices[i]];
        Vertex& b = vertices[indices[i + 1]];
        Vertex& c = vertices[indices[i + 2]];
        const Vec3 n = Cross(b.position - a.position, c.position - a.position);
        a.normal += n;
        b.normal += n;
        c.normal += n;
    }
    for (Vertex& v : vertices) v.normal = Normalize(v.normal, {0.0f, 1.0f, 0.0f});
}

bool MeshData::Validate(std::string& error) const {
    if (indices.size() % 3 != 0) {
        error = "index count is not a multiple of 3";
        return false;
    }
    for (std::uint32_t index : indices) {
        if (index >= vertices.size()) {
            error = "index out of range";
            return false;
        }
    }
    for (const Vertex& v : vertices) {
        if (!IsFinite(v.position) || !IsFinite(v.normal) || !IsFinite(v.uv.x) || !IsFinite(v.uv.y)) {
            error = "non-finite vertex data";
            return false;
        }
    }
    if (!skin.empty()) {
        if (skin.size() != vertices.size()) {
            error = "skin influence count does not match vertex count";
            return false;
        }
        for (const SkinInfluence& influence : skin) {
            float sum = 0.0f;
            for (float w : influence.weights) {
                if (!IsFinite(w) || w < 0.0f) {
                    error = "invalid skin weight";
                    return false;
                }
                sum += w;
            }
            if (std::fabs(sum - 1.0f) > 1.0e-3f) {
                error = "skin weights do not sum to 1";
                return false;
            }
        }
    }
    return true;
}

void MeshBuilder::SetTransform(const Mat4& transform) {
    transform_ = transform;
    Mat4 inverse{};
    normalTransform_ = Inverse(transform, inverse) ? Transpose(inverse) : Mat4::Identity();
}

Vec3 MeshBuilder::ApplyPoint(Vec3 p) const { return TransformPoint(transform_, p); }
Vec3 MeshBuilder::ApplyNormal(Vec3 n) const {
    return Normalize(TransformVector(normalTransform_, n), n);
}

std::uint32_t MeshBuilder::AddVertex(Vec3 position, Vec3 normal, Vec2 uv) {
    Vertex v;
    v.position = ApplyPoint(position);
    v.normal = ApplyNormal(normal);
    v.uv = uv * uvScale_;
    v.color = color_;
    mesh_.vertices.push_back(v);
    if (skinJoint_ >= 0 || !mesh_.skin.empty()) {
        // Keep one influence per vertex; unskinned vertices bind rigidly to joint 0.
        mesh_.skin.resize(mesh_.vertices.size() - 1);
        SkinInfluence influence;
        influence.joints[0] = static_cast<std::uint16_t>(std::max(0, skinJoint_));
        mesh_.skin.push_back(influence);
    }
    return static_cast<std::uint32_t>(mesh_.vertices.size() - 1);
}

void MeshBuilder::AddTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c) {
    const Vertex& va = mesh_.vertices[a];
    const Vertex& vb = mesh_.vertices[b];
    const Vertex& vc = mesh_.vertices[c];
    const Vec3 geometric = Cross(vb.position - va.position, vc.position - va.position);
    const Vec3 shading = va.normal + vb.normal + vc.normal;
    // Front faces satisfy cross(b - a, c - a) . outward > 0; fix inverted input.
    if (Dot(geometric, shading) < 0.0f) std::swap(b, c);
    mesh_.indices.push_back(a);
    mesh_.indices.push_back(b);
    mesh_.indices.push_back(c);
}

void MeshBuilder::AddQuad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 normal, Vec2 uvSize) {
    const std::uint32_t ia = AddVertex(a, normal, {0.0f, uvSize.y});
    const std::uint32_t ib = AddVertex(b, normal, {0.0f, 0.0f});
    const std::uint32_t ic = AddVertex(c, normal, {uvSize.x, 0.0f});
    const std::uint32_t id = AddVertex(d, normal, {uvSize.x, uvSize.y});
    AddTriangle(ia, ib, ic);
    AddTriangle(ia, ic, id);
}

void MeshBuilder::AddBox(Vec3 center, Vec3 h) {
    AddTaperedBox({center.x, center.y - h.y, center.z}, {h.x, h.z}, {h.x, h.z}, h.y * 2.0f);
}

void MeshBuilder::AddTaperedBox(Vec3 base, Vec2 bottom, Vec2 top, float height) {
    const Vec3 b0{base.x - bottom.x, base.y, base.z - bottom.y};
    const Vec3 b1{base.x + bottom.x, base.y, base.z - bottom.y};
    const Vec3 b2{base.x + bottom.x, base.y, base.z + bottom.y};
    const Vec3 b3{base.x - bottom.x, base.y, base.z + bottom.y};
    const float y = base.y + height;
    const Vec3 t0{base.x - top.x, y, base.z - top.y};
    const Vec3 t1{base.x + top.x, y, base.z - top.y};
    const Vec3 t2{base.x + top.x, y, base.z + top.y};
    const Vec3 t3{base.x - top.x, y, base.z + top.y};
    auto face = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d) {
        const Vec3 normal = Normalize(Cross(b - a, c - a) + Cross(c - a, d - a), {0, 1, 0});
        const float width = Distance(a, d);
        const float tall = Distance(a, b);
        AddQuad(a, b, c, d, normal, {width, tall});
    };
    face(b0, t0, t1, b1); // -Z
    face(b1, t1, t2, b2); // +X
    face(b2, t2, t3, b3); // +Z
    face(b3, t3, t0, b0); // -X
    face(t0, t3, t2, t1); // top
    face(b0, b1, b2, b3); // bottom
}

void MeshBuilder::AddPyramid(Vec3 base, Vec2 half, float height) {
    const Vec3 apex{base.x, base.y + height, base.z};
    const Vec3 corners[4] = {
        {base.x - half.x, base.y, base.z - half.y}, {base.x + half.x, base.y, base.z - half.y},
        {base.x + half.x, base.y, base.z + half.y}, {base.x - half.x, base.y, base.z + half.y}};
    for (int i = 0; i < 4; ++i) {
        const Vec3 a = corners[i];
        const Vec3 b = corners[(i + 1) % 4];
        const Vec3 outward = Normalize(Cross(apex - a, b - a), {0, 1, 0});
        const Vec3 n = Dot(outward, Horizontal((a + b) * 0.5f - base)) >= 0.0f ? outward : -outward;
        const std::uint32_t ia = AddVertex(a, n, {0.0f, 0.0f});
        const std::uint32_t ib = AddVertex(b, n, {Distance(a, b), 0.0f});
        const std::uint32_t ic = AddVertex(apex, n, {Distance(a, b) * 0.5f, height});
        AddTriangle(ia, ib, ic);
    }
    AddQuad(corners[0], corners[1], corners[2], corners[3], {0, -1, 0}, {half.x * 2, half.y * 2});
}

void MeshBuilder::AddCylinder(Vec3 base, float radius, float height, int segments, bool caps, float topRadius) {
    segments = std::max(3, segments);
    if (topRadius < 0.0f) topRadius = radius;
    const float slope = (radius - topRadius) / std::max(height, 1.0e-4f);
    const float circumference = kTwoPi * std::max(radius, topRadius);
    std::vector<std::uint32_t> ring;
    for (int i = 0; i <= segments; ++i) {
        const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(segments);
        const float c = std::cos(angle), s = std::sin(angle);
        const Vec3 normal = Normalize(Vec3{c, slope, s});
        const float u = circumference * static_cast<float>(i) / static_cast<float>(segments);
        ring.push_back(AddVertex({base.x + c * radius, base.y, base.z + s * radius}, normal, {u, 0.0f}));
        ring.push_back(AddVertex({base.x + c * topRadius, base.y + height, base.z + s * topRadius}, normal, {u, height}));
    }
    for (int i = 0; i < segments; ++i) {
        const std::uint32_t b0 = ring[static_cast<std::size_t>(i * 2)], t0 = ring[static_cast<std::size_t>(i * 2 + 1)];
        const std::uint32_t b1 = ring[static_cast<std::size_t>(i * 2 + 2)], t1 = ring[static_cast<std::size_t>(i * 2 + 3)];
        AddTriangle(b0, t0, t1);
        AddTriangle(b0, t1, b1);
    }
    if (!caps) return;
    for (int cap = 0; cap < 2; ++cap) {
        const float y = cap == 0 ? base.y : base.y + height;
        const float r = cap == 0 ? radius : topRadius;
        if (r <= 0.0f) continue;
        const Vec3 n{0.0f, cap == 0 ? -1.0f : 1.0f, 0.0f};
        const std::uint32_t center = AddVertex({base.x, y, base.z}, n, {0.0f, 0.0f});
        std::vector<std::uint32_t> rim;
        for (int i = 0; i <= segments; ++i) {
            const float angle = kTwoPi * static_cast<float>(i) / static_cast<float>(segments);
            rim.push_back(AddVertex({base.x + std::cos(angle) * r, y, base.z + std::sin(angle) * r}, n,
                {std::cos(angle) * r, std::sin(angle) * r}));
        }
        for (int i = 0; i < segments; ++i) AddTriangle(center, rim[static_cast<std::size_t>(i)], rim[static_cast<std::size_t>(i + 1)]);
    }
}

void MeshBuilder::AddSphere(Vec3 center, float radius, int rings, int segments) {
    rings = std::max(2, rings);
    segments = std::max(3, segments);
    const std::uint32_t first = static_cast<std::uint32_t>(mesh_.vertices.size());
    for (int r = 0; r <= rings; ++r) {
        const float phi = kPi * static_cast<float>(r) / static_cast<float>(rings);
        for (int s = 0; s <= segments; ++s) {
            const float theta = kTwoPi * static_cast<float>(s) / static_cast<float>(segments);
            const Vec3 n{std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)};
            AddVertex(center + n * radius, n,
                {static_cast<float>(s) / static_cast<float>(segments) * kTwoPi * radius, phi * radius});
        }
    }
    const std::uint32_t stride = static_cast<std::uint32_t>(segments + 1);
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < segments; ++s) {
            const std::uint32_t a = first + static_cast<std::uint32_t>(r) * stride + static_cast<std::uint32_t>(s);
            const std::uint32_t b = a + stride;
            if (r != 0) AddTriangle(a, b, a + 1);
            if (r != rings - 1) AddTriangle(a + 1, b, b + 1);
        }
    }
}

void MeshBuilder::AddCapsule(Vec3 base, float radius, float height, int rings, int segments) {
    rings = std::max(2, rings / 2 * 2);
    segments = std::max(3, segments);
    const float cylinder = std::max(0.0f, height - 2.0f * radius);
    const std::uint32_t first = static_cast<std::uint32_t>(mesh_.vertices.size());
    const int half = rings / 2;
    int rowCount = 0;
    for (int r = 0; r <= rings + 1; ++r) {
        // Upper hemisphere rows then lower hemisphere rows, duplicated equator.
        const bool upper = r <= half;
        const float phi = kPi * static_cast<float>(upper ? r : r - 1) / static_cast<float>(rings);
        const float yOffset = upper ? base.y + radius + cylinder : base.y + radius;
        for (int s = 0; s <= segments; ++s) {
            const float theta = kTwoPi * static_cast<float>(s) / static_cast<float>(segments);
            const Vec3 n{std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta)};
            AddVertex({base.x + n.x * radius, yOffset + n.y * radius, base.z + n.z * radius}, n,
                {static_cast<float>(s) / static_cast<float>(segments), yOffset + n.y * radius - base.y});
        }
        ++rowCount;
    }
    const std::uint32_t stride = static_cast<std::uint32_t>(segments + 1);
    for (int r = 0; r < rowCount - 1; ++r) {
        for (int s = 0; s < segments; ++s) {
            const std::uint32_t a = first + static_cast<std::uint32_t>(r) * stride + static_cast<std::uint32_t>(s);
            const std::uint32_t b = a + stride;
            if (r != 0) AddTriangle(a, b, a + 1);
            if (r != rowCount - 2) AddTriangle(a + 1, b, b + 1);
        }
    }
}

void MeshBuilder::AddPlane(Vec3 center, Vec2 halfSize, int subdivisions, Vec2 uvTiles) {
    subdivisions = std::max(1, subdivisions);
    const std::uint32_t first = static_cast<std::uint32_t>(mesh_.vertices.size());
    const int n = subdivisions;
    for (int z = 0; z <= n; ++z) {
        for (int x = 0; x <= n; ++x) {
            const float fx = static_cast<float>(x) / static_cast<float>(n);
            const float fz = static_cast<float>(z) / static_cast<float>(n);
            AddVertex({center.x - halfSize.x + fx * halfSize.x * 2.0f, center.y,
                          center.z - halfSize.y + fz * halfSize.y * 2.0f},
                {0, 1, 0}, {fx * uvTiles.x, fz * uvTiles.y});
        }
    }
    const std::uint32_t stride = static_cast<std::uint32_t>(n + 1);
    for (int z = 0; z < n; ++z) {
        for (int x = 0; x < n; ++x) {
            const std::uint32_t a = first + static_cast<std::uint32_t>(z) * stride + static_cast<std::uint32_t>(x);
            AddTriangle(a, a + stride, a + stride + 1);
            AddTriangle(a, a + stride + 1, a + 1);
        }
    }
}

void MeshBuilder::AddTorus(Vec3 center, float majorRadius, float minorRadius, int majorSegments, int minorSegments) {
    majorSegments = std::max(3, majorSegments);
    minorSegments = std::max(3, minorSegments);
    const std::uint32_t first = static_cast<std::uint32_t>(mesh_.vertices.size());
    for (int i = 0; i <= majorSegments; ++i) {
        const float u = kTwoPi * static_cast<float>(i) / static_cast<float>(majorSegments);
        const Vec3 ringCenter{std::cos(u) * majorRadius, 0.0f, std::sin(u) * majorRadius};
        for (int j = 0; j <= minorSegments; ++j) {
            const float v = kTwoPi * static_cast<float>(j) / static_cast<float>(minorSegments);
            const Vec3 n{std::cos(u) * std::cos(v), std::sin(v), std::sin(u) * std::cos(v)};
            AddVertex(center + ringCenter + n * minorRadius, n,
                {static_cast<float>(i) / static_cast<float>(majorSegments), static_cast<float>(j) / static_cast<float>(minorSegments)});
        }
    }
    const std::uint32_t stride = static_cast<std::uint32_t>(minorSegments + 1);
    for (int i = 0; i < majorSegments; ++i) {
        for (int j = 0; j < minorSegments; ++j) {
            const std::uint32_t a = first + static_cast<std::uint32_t>(i) * stride + static_cast<std::uint32_t>(j);
            AddTriangle(a, a + stride, a + stride + 1);
            AddTriangle(a, a + stride + 1, a + 1);
        }
    }
}

void MeshBuilder::AddBlade(Vec3 base, float length, float width, float thickness) {
    const float hw = width * 0.5f, ht = thickness * 0.5f;
    const Vec3 tip{base.x, base.y + length, base.z};
    const Vec3 left{base.x - hw, base.y + length * 0.08f, base.z};
    const Vec3 right{base.x + hw, base.y + length * 0.08f, base.z};
    const Vec3 upperLeft{base.x - hw * 0.8f, base.y + length * 0.82f, base.z};
    const Vec3 upperRight{base.x + hw * 0.8f, base.y + length * 0.82f, base.z};
    const Vec3 spineFront{base.x, base.y + length * 0.5f, base.z - ht};
    const Vec3 spineBack{base.x, base.y + length * 0.5f, base.z + ht};
    const Vec3 outline[6] = {left, upperLeft, tip, upperRight, right, {base.x, base.y, base.z}};
    for (int side = 0; side < 2; ++side) {
        const Vec3 spine = side == 0 ? spineFront : spineBack;
        for (int i = 0; i < 6; ++i) {
            const Vec3 a = outline[i];
            const Vec3 b = outline[(i + 1) % 6];
            const Vec3 n0 = Normalize(Cross(b - a, spine - a), {0, 0, side == 0 ? -1.0f : 1.0f});
            const Vec3 n = (side == 0) == (n0.z < 0.0f) ? n0 : -n0;
            const std::uint32_t ia = AddVertex(a, n, {0, 0});
            const std::uint32_t ib = AddVertex(b, n, {1, 0});
            const std::uint32_t is = AddVertex(spine, n, {0.5f, 1});
            AddTriangle(ia, ib, is);
        }
    }
}

void MeshBuilder::Append(const MeshData& other) {
    const std::uint32_t offset = static_cast<std::uint32_t>(mesh_.vertices.size());
    for (const Vertex& source : other.vertices) {
        Vertex v = source;
        v.position = ApplyPoint(source.position);
        v.normal = ApplyNormal(source.normal);
        v.color = {v.color.x * color_.x, v.color.y * color_.y, v.color.z * color_.z, v.color.w * color_.w};
        mesh_.vertices.push_back(v);
    }
    if (!other.skin.empty() || !mesh_.skin.empty()) {
        mesh_.skin.resize(offset);
        if (!other.skin.empty()) mesh_.skin.insert(mesh_.skin.end(), other.skin.begin(), other.skin.end());
        else mesh_.skin.resize(mesh_.vertices.size());
    }
    for (std::uint32_t index : other.indices) mesh_.indices.push_back(index + offset);
}

} // namespace Astral::Graphics
