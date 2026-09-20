#include "Engine/Assets/StaticMesh.h"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <locale>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace Astral::Assets {
namespace {

bool ReadFloat(std::istream& input, float& value) {
    std::string token;
    if (!(input >> token)) return false;
    std::istringstream number(token);
    number.imbue(std::locale::classic());
    number >> std::noskipws >> value;
    return number && std::isfinite(value)
        && number.rdbuf()->sgetc() == std::char_traits<char>::eof();
}

bool ReadIndex(std::istream& input, unsigned int& value) {
    std::string token;
    if (!(input >> token) || token.empty()) return false;
    // Reject signs (including -0), suffixes, and overflow consistently.
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    return result.ec == std::errc{} && result.ptr == token.data() + token.size();
}

} // namespace

bool StaticMesh::LoadFromFile(const std::string& filePath) {
    return LoadFromFile(filePath, MeshLoadLimits{});
}

bool StaticMesh::LoadFromFile(const std::string& filePath, const MeshLoadLimits& limits) {
    // Keep the existing observable contract: a failed load leaves an empty mesh.
    vertices_.clear();
    edges_.clear();
    if (limits.maxFileBytes == 0 || limits.maxVertices == 0 || limits.maxEdges == 0) {
        return false;
    }

    try {
        std::error_code error;
        if (!std::filesystem::is_regular_file(filePath, error) || error) return false;
        const auto bytes = std::filesystem::file_size(filePath, error);
        if (error || bytes == 0 || bytes > limits.maxFileBytes
            || bytes > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
            return false;
        }

        std::ifstream file(filePath, std::ios::binary);
        if (!file) return false;
        std::string text(static_cast<std::size_t>(bytes), '\0');
        file.read(text.data(), static_cast<std::streamsize>(bytes));
        if (!file || file.peek() != std::char_traits<char>::eof() || file.bad()) return false;

        std::istringstream input(text);
        input.imbue(std::locale::classic());
        std::string command;
        std::string version;
        if (!(input >> command >> version) || command != "ASTRAL_MESH" || version != "1") {
            return false;
        }

        std::vector<Math::Vec3> vertices;
        std::vector<MeshEdge> edges;
        while (input >> command) {
            if (command == "vertex") {
                if (vertices.size() >= limits.maxVertices) return false;
                Math::Vec3 vertex{};
                if (!ReadFloat(input, vertex.x) || !ReadFloat(input, vertex.y)
                    || !ReadFloat(input, vertex.z)) return false;
                vertices.push_back(vertex);
            } else if (command == "edge") {
                if (edges.size() >= limits.maxEdges) return false;
                MeshEdge edge{};
                if (!ReadIndex(input, edge.start) || !ReadIndex(input, edge.end)) return false;
                edges.push_back(edge);
            } else {
                // Includes a second header, unknown records, and trailing garbage.
                return false;
            }
        }
        if (input.bad() || !input.eof() || vertices.empty() || edges.empty()) return false;
        for (const MeshEdge& edge : edges) {
            if (edge.start >= vertices.size() || edge.end >= vertices.size()) return false;
        }
        vertices_.swap(vertices);
        edges_.swap(edges);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    } catch (const std::length_error&) {
        return false;
    }
}

} // namespace Astral::Assets
