#include "Engine/Assets/StaticMesh.h"

#include <fstream>
#include <string>

namespace Astral::Assets {

bool StaticMesh::LoadFromFile(const std::string& filePath) {
    vertices_.clear();
    edges_.clear();

    std::ifstream input(filePath);
    if (!input.is_open()) {
        return false;
    }

    std::string command;
    int version = 0;
    bool headerSeen = false;
    while (input >> command) {
        if (command == "ASTRAL_MESH") {
            if (headerSeen || !(input >> version) || version != 1) {
                vertices_.clear();
                edges_.clear();
                return false;
            }
            headerSeen = true;
        } else if (command == "vertex") {
            Math::Vec3 vertex{};
            if (!(input >> vertex.x >> vertex.y >> vertex.z)) {
                vertices_.clear();
                edges_.clear();
                return false;
            }
            vertices_.push_back(vertex);
        } else if (command == "edge") {
            MeshEdge edge{};
            if (!(input >> edge.start >> edge.end)) {
                vertices_.clear();
                edges_.clear();
                return false;
            }
            edges_.push_back(edge);
        } else {
            vertices_.clear();
            edges_.clear();
            return false;
        }
    }

    for (const MeshEdge& edge : edges_) {
        if (edge.start >= vertices_.size() || edge.end >= vertices_.size()) {
            vertices_.clear();
            edges_.clear();
            return false;
        }
    }

    if (!headerSeen || vertices_.empty() || edges_.empty()) {
        vertices_.clear();
        edges_.clear();
        return false;
    }
    return true;
}

} // namespace Astral::Assets
