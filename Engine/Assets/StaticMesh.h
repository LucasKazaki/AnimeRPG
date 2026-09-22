#pragma once

#include "Engine/Math/Math.h"

#include <cstddef>
#include <string>
#include <vector>

namespace Astral::Assets {

struct MeshEdge {
    unsigned int start{};
    unsigned int end{};
};

// Trusted caller budgets for the small ASTRAL_MESH 1 wireframe format.
// These are resource limits, not a general-purpose asset import pipeline.
struct MeshLoadLimits {
    std::size_t maxFileBytes{16u * 1024u * 1024u};
    std::size_t maxVertices{262144u};
    std::size_t maxEdges{524288u};
};

class StaticMesh {
public:
    bool LoadFromFile(const std::string& filePath);
    bool LoadFromFile(const std::string& filePath, const MeshLoadLimits& limits);

    const std::vector<Math::Vec3>& Vertices() const { return vertices_; }
    const std::vector<MeshEdge>& Edges() const { return edges_; }

private:
    std::vector<Math::Vec3> vertices_;
    std::vector<MeshEdge> edges_;
};

} // namespace Astral::Assets
