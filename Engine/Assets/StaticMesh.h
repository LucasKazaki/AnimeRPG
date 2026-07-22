#pragma once

#include "Engine/Math/Math.h"

#include <string>
#include <vector>

namespace Astral::Assets {

struct MeshEdge {
    unsigned int start{};
    unsigned int end{};
};

class StaticMesh {
public:
    bool LoadFromFile(const std::string& filePath);

    const std::vector<Math::Vec3>& Vertices() const { return vertices_; }
    const std::vector<MeshEdge>& Edges() const { return edges_; }

private:
    std::vector<Math::Vec3> vertices_;
    std::vector<MeshEdge> edges_;
};

} // namespace Astral::Assets
