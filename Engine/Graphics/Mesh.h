#pragma once

// Triangle meshes (positions, normals, UVs, vertex colours, optional skin
// weights) and a procedural builder used for engine fixtures and blockouts.

#include "Engine/Graphics/Color.h"
#include "Engine/Math/Geometry.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Astral::Graphics {

struct Vertex {
    Math::Vec3 position{};
    Math::Vec3 normal{0.0f, 1.0f, 0.0f};
    Math::Vec2 uv{};
    Math::Vec4 color{1.0f, 1.0f, 1.0f, 1.0f}; // linear RGBA multiplier
};

struct SkinInfluence {
    std::array<std::uint16_t, 4> joints{};
    std::array<float, 4> weights{1.0f, 0.0f, 0.0f, 0.0f};
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;  // triangle list, clockwise front faces seen from outside
    std::vector<SkinInfluence> skin;     // empty or one per vertex
    Math::AABB bounds;
    std::string name;

    std::size_t TriangleCount() const { return indices.size() / 3; }
    void ComputeBounds();
    void RecomputeNormals();
    // Structural validation: index ranges, finite data, skin weights.
    bool Validate(std::string& error) const;
};

// Appends primitives in local space, optionally through a transform.
// Winding: triangles are emitted clockwise when viewed from the outside, the
// front-face convention of the left-handed renderer.
class MeshBuilder {
public:
    explicit MeshBuilder(MeshData& mesh) : mesh_(mesh) {}

    void SetTransform(const Math::Mat4& transform);
    void SetColor(Math::Vec4 color) { color_ = color; }
    void SetColor(Color color) { color_ = {color.x, color.y, color.z, 1.0f}; }
    void SetSkinJoint(int joint) { skinJoint_ = joint; }
    void SetUvScale(float scale) { uvScale_ = scale; }

    std::uint32_t AddVertex(Math::Vec3 position, Math::Vec3 normal, Math::Vec2 uv);
    void AddTriangle(std::uint32_t a, std::uint32_t b, std::uint32_t c);
    void AddQuad(Math::Vec3 a, Math::Vec3 b, Math::Vec3 c, Math::Vec3 d, Math::Vec3 normal,
        Math::Vec2 uvSize = {1.0f, 1.0f});

    void AddBox(Math::Vec3 center, Math::Vec3 halfExtents);
    // Frustum-shaped box: different half extents at the bottom and top (obelisks, plinths).
    void AddTaperedBox(Math::Vec3 baseCenter, Math::Vec2 bottomHalf, Math::Vec2 topHalf, float height);
    void AddPyramid(Math::Vec3 baseCenter, Math::Vec2 baseHalf, float height);
    void AddCylinder(Math::Vec3 baseCenter, float radius, float height, int segments, bool caps = true,
        float topRadius = -1.0f);
    void AddSphere(Math::Vec3 center, float radius, int rings, int segments);
    // Capsule along +Y from baseCenter, total height including caps.
    void AddCapsule(Math::Vec3 baseCenter, float radius, float height, int rings, int segments);
    void AddPlane(Math::Vec3 center, Math::Vec2 halfSize, int subdivisions, Math::Vec2 uvTiles = {1, 1});
    void AddTorus(Math::Vec3 center, float majorRadius, float minorRadius, int majorSegments, int minorSegments);
    // Flat blade in the XY plane pointing +Y, thickness along Z.
    void AddBlade(Math::Vec3 base, float length, float width, float thickness);
    void Append(const MeshData& other);

private:
    Math::Vec3 ApplyPoint(Math::Vec3 p) const;
    Math::Vec3 ApplyNormal(Math::Vec3 n) const;

    MeshData& mesh_;
    Math::Mat4 transform_ = Math::Mat4::Identity();
    Math::Mat4 normalTransform_ = Math::Mat4::Identity();
    Math::Vec4 color_{1, 1, 1, 1};
    int skinJoint_{-1};
    float uvScale_{1.0f};
};

} // namespace Astral::Graphics
