#pragma once

// Mesh simplification and levels of detail (the role of Unreal's automatic LOD
// generation and Unity's LOD Group).
//
// SimplifyMesh is quadric-error-metric edge collapse (Garland & Heckbert) with
// half-edge collapses: every output vertex is an input vertex, so UVs,
// normals, colours and skin weights survive untouched. Vertices on UV/normal
// seams are locked, as are open borders when asked (otherwise borders are
// held by constraint planes). Collapses that would fold a triangle over or
// break manifold topology are rejected, and an absolute error bound can stop
// simplification early.
//
// LodGroup picks a level from the projected screen size of the mesh bounds
// with hysteresis, so objects do not flicker between levels at a threshold.

#include "Engine/Graphics/Mesh.h"

#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

namespace Astral::Graphics {

struct SimplifyOptions {
    std::size_t targetTriangles{0};      // stop at or below this count (0 = use targetRatio)
    float targetRatio{0.5f};             // of the input triangle count
    float maxError{std::numeric_limits<float>::infinity()}; // world-space distance bound
    bool lockBorders{false};             // keep open-boundary vertices where they are
    float borderWeight{10.0f};           // strength of the planes that hold unlocked borders
};

struct SimplifyStats {
    std::size_t trianglesIn{};
    std::size_t trianglesOut{};
    std::size_t verticesOut{};
    std::size_t collapses{};
    float error{}; // estimated geometric error of the last collapse (world units)
};

// Returns false (out untouched) for invalid input. An already small mesh is copied.
bool SimplifyMesh(const MeshData& in, const SimplifyOptions& options, MeshData& out, SimplifyStats* stats = nullptr);

// Fraction of the view height covered by a sphere of `radius` at `distance`
// (Unreal's "screen size"): 1 = fills the height; >= 1 when the camera is inside it.
float ScreenSize(float radius, float distance, float verticalFovRadians);

struct LodLevel {
    std::shared_ptr<const MeshData> mesh;
    float minScreenSize{}; // this level is used down to this screen size
    float error{};         // simplification error estimate
};

struct LodGroup {
    std::vector<LodLevel> levels; // [0] = full detail; minScreenSize decreasing

    // Level for a screen size. With `current` >= 0, switching needs the size to
    // pass the threshold by `hysteresis` (fractional), which stops flicker.
    int Select(float screenSize, int current = -1, float hysteresis = 0.1f) const;
    std::size_t Count() const { return levels.size(); }
};

// LOD0 is `base`; each further level simplifies the base to `ratios[i]` of its
// triangles and is used below `screenSizes[i + 1]` (screenSizes[0] is LOD0's).
// Levels that fail to reduce the triangle count are skipped.
LodGroup BuildLodGroup(std::shared_ptr<const MeshData> base, const std::vector<float>& ratios = {0.5f, 0.25f, 0.1f},
    const std::vector<float>& screenSizes = {0.5f, 0.25f, 0.1f, 0.03f});

} // namespace Astral::Graphics
