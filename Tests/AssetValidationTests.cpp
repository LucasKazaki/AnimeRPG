#include "Engine/Assets/StaticMesh.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <locale>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using Astral::Assets::MeshLoadLimits;
using Astral::Assets::StaticMesh;
const std::string Valid = "ASTRAL_MESH 1\nvertex -1 0 0\nvertex 1 0 0\nvertex 0 1 0\nedge 0 1\nedge 1 2\nedge 2 0\n";

void Require(bool condition, const std::string& message) {
    // Deliberately independent of assert/NDEBUG, including in Release.
    if (!condition) throw std::runtime_error(message);
}

class Fixture {
public:
    Fixture() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt) {
            auto candidate = std::filesystem::temp_directory_path()
                / ("astral-asset-" + std::to_string(stamp) + "-" + std::to_string(attempt));
            if (std::filesystem::create_directory(candidate)) { root_ = candidate; return; }
        }
        throw std::runtime_error("Cannot create exclusively owned test directory");
    }
    ~Fixture() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored); // Only this fixture's newly created directory.
    }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    std::filesystem::path Path() const { return root_ / "input.mesh"; }
    std::filesystem::path Directory() const { return root_; }
    void Write(const std::string& text) const {
        std::ofstream out(Path(), std::ios::binary | std::ios::trunc);
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        out.close();
        Require(static_cast<bool>(out), "Fixture write failed");
    }
    void Check(const std::string& text, bool expected, MeshLoadLimits limits = {}) const {
        Write(text);
        StaticMesh mesh;
        Require(mesh.LoadFromFile(Path().string(), limits) == expected, "Unexpected load result");
        Invariants(mesh, expected, limits);
    }
    static void Invariants(const StaticMesh& mesh, bool loaded, const MeshLoadLimits& limits) {
        if (!loaded) {
            Require(mesh.Vertices().empty() && mesh.Edges().empty(), "Failed load retained partial mesh");
            return;
        }
        Require(!mesh.Vertices().empty() && mesh.Vertices().size() <= limits.maxVertices, "Vertex budget");
        Require(!mesh.Edges().empty() && mesh.Edges().size() <= limits.maxEdges, "Edge budget");
        for (const auto& v : mesh.Vertices())
            Require(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z), "Non-finite vertex");
        for (const auto& edge : mesh.Edges())
            Require(edge.start < mesh.Vertices().size() && edge.end < mesh.Vertices().size(), "Index bounds");
    }
private:
    std::filesystem::path root_;
};

struct CommaPunctuation : std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
};
struct ScopedCommaLocale {
    std::locale prior{std::locale()};
    ScopedCommaLocale() { std::locale::global(std::locale(prior, new CommaPunctuation)); }
    ~ScopedCommaLocale() { std::locale::global(prior); }
};

} // namespace

int main() {
    try {
        Fixture fixture;
        const std::vector<std::pair<std::string, std::function<void()>>> cases = {
            {"valid triangle and legacy entry point", [&] {
                fixture.Write(Valid); StaticMesh mesh;
                Require(mesh.LoadFromFile(fixture.Path().string()), "Legacy entry point");
                Require(mesh.Vertices().size() == 3 && mesh.Edges().size() == 3, "Triangle counts");
                Require(mesh.Vertices()[0].x == -1.0f, "Triangle coordinates");
            }},
            {"whitespace and no final newline", [&] { fixture.Check(" \r\n\tASTRAL_MESH 1 vertex 0 0 0 edge 0 0", true); }},
            {"forward index references", [&] { fixture.Check("ASTRAL_MESH 1 edge 0 1 vertex 0 0 0 vertex 1 1 1", true); }},
            {"header must precede data", [&] { fixture.Check("vertex 0 0 0\nASTRAL_MESH 1\nedge 0 0", false); }},
            {"missing header", [&] { fixture.Check("vertex 0 0 0 edge 0 0", false); }},
            {"wrong version", [&] { fixture.Check("ASTRAL_MESH 2 vertex 0 0 0 edge 0 0", false); }},
            {"duplicate header", [&] { fixture.Check(Valid + "ASTRAL_MESH 1", false); }},
            {"empty input", [&] { fixture.Check("", false); }},
            {"header only", [&] { fixture.Check("ASTRAL_MESH 1", false); }},
            {"no edges", [&] { fixture.Check("ASTRAL_MESH 1 vertex 0 0 0", false); }},
            {"no vertices", [&] { fixture.Check("ASTRAL_MESH 1 edge 0 0", false); }},
            {"unknown and trailing records", [&] { fixture.Check(Valid + "junk", false); }},
            {"truncated records", [&] {
                for (const auto* tail : {"vertex", "vertex 1", "vertex 1 2", "edge", "edge 0", "ASTRAL_MESH"})
                    fixture.Check(Valid + tail, false);
            }},
            {"non-finite and overflowing coordinates", [&] {
                for (const auto* number : {"nan", "NaN", "inf", "-inf", "1e9999", "-1e9999"})
                    fixture.Check(std::string("ASTRAL_MESH 1 vertex ") + number + " 0 0 edge 0 0", false);
            }},
            {"whole float tokens", [&] { fixture.Check("ASTRAL_MESH 1 vertex 0 0 0edge 0 0", false); }},
            {"signed and overflowing indices", [&] {
                for (const auto* index : {"-0", "-1", "+0", "4294967296", "999999999999999999999", "0x0", "0.0", "0edge"})
                    fixture.Check(std::string("ASTRAL_MESH 1 vertex 0 0 0 edge ") + index + " 0", false);
            }},
            {"out-of-range edge", [&] { fixture.Check("ASTRAL_MESH 1 vertex 0 0 0 edge 0 1", false); }},
            {"embedded NUL", [&] { fixture.Check(Valid + std::string(1, '\0'), false); }},
            {"byte budget boundary", [&] {
                MeshLoadLimits limits; limits.maxFileBytes = Valid.size(); fixture.Check(Valid, true, limits);
                --limits.maxFileBytes; fixture.Check(Valid, false, limits);
            }},
            {"vertex budget boundary", [&] {
                MeshLoadLimits limits; limits.maxVertices = 3; fixture.Check(Valid, true, limits);
                limits.maxVertices = 2; fixture.Check(Valid, false, limits);
            }},
            {"edge budget boundary", [&] {
                MeshLoadLimits limits; limits.maxEdges = 3; fixture.Check(Valid, true, limits);
                limits.maxEdges = 2; fixture.Check(Valid, false, limits);
            }},
            {"zero budgets", [&] {
                MeshLoadLimits limits; limits.maxFileBytes = 0; fixture.Check(Valid, false, limits);
                limits = {}; limits.maxVertices = 0; fixture.Check(Valid, false, limits);
                limits = {}; limits.maxEdges = 0; fixture.Check(Valid, false, limits);
            }},
            {"large file rejected before parsing", [&] {
                const MeshLoadLimits limits;
                std::ofstream out(fixture.Path(), std::ios::binary | std::ios::trunc);
                out.seekp(static_cast<std::streamoff>(limits.maxFileBytes)); out.put('x'); out.close();
                Require(static_cast<bool>(out), "Large fixture write");
                StaticMesh mesh; Require(!mesh.LoadFromFile(fixture.Path().string()), "Oversize load");
                Fixture::Invariants(mesh, false, limits);
            }},
            {"missing file and directory", [&] {
                StaticMesh mesh;
                Require(!mesh.LoadFromFile((fixture.Directory() / "not-present.mesh").string()), "Missing file");
                Require(!mesh.LoadFromFile(fixture.Directory().string()), "Directory input");
            }},
            {"locale-independent coordinates", [&] {
                ScopedCommaLocale scoped;
                fixture.Check("ASTRAL_MESH 1 vertex 0.5 -2.25 1e2 edge 0 0", true);
                fixture.Check("ASTRAL_MESH 1 vertex 0,5 0 0 edge 0 0", false);
            }},
            {"valid failure valid reload cycles", [&] {
                StaticMesh mesh;
                for (int cycle = 0; cycle < 200; ++cycle) {
                    fixture.Write(Valid); Require(mesh.LoadFromFile(fixture.Path().string()), "Reload valid");
                    fixture.Write(Valid + "edge 0");
                    Require(!mesh.LoadFromFile(fixture.Path().string()), "Reload truncated");
                    Fixture::Invariants(mesh, false, {});
                }
                fixture.Write(Valid); Require(mesh.LoadFromFile(fixture.Path().string()), "Final recovery");
                Require(!mesh.LoadFromFile((fixture.Directory() / "missing.mesh").string()), "Missing reload");
                Fixture::Invariants(mesh, false, {});
            }},
            {"1000 seeded mutation cases with repeatability", [&] {
                std::mt19937 rng(20260920u);
                MeshLoadLimits limits; limits.maxFileBytes = 1024; limits.maxVertices = 32; limits.maxEdges = 64;
                StaticMesh mesh;
                std::size_t accepted = 0;
                for (int sample = 0; sample < 1000; ++sample) {
                    std::string text = Valid;
                    if (sample % 10 != 0) {
                        for (unsigned edit = 0, count = 1 + rng() % 8; edit < count; ++edit) {
                            const auto pos = static_cast<std::size_t>(rng()) % (text.size() + 1);
                            if (pos < text.size() && rng() % 2 == 0) text.erase(pos, 1);
                            else text.insert(pos, 1, static_cast<char>(rng() % 128));
                        }
                    }
                    fixture.Write(text);
                    const bool first = mesh.LoadFromFile(fixture.Path().string(), limits);
                    Fixture::Invariants(mesh, first, limits);
                    const auto vertices = mesh.Vertices(); const auto edges = mesh.Edges();
                    const bool second = mesh.LoadFromFile(fixture.Path().string(), limits);
                    Fixture::Invariants(mesh, second, limits);
                    Require(first == second && vertices.size() == mesh.Vertices().size()
                        && edges.size() == mesh.Edges().size(), "Non-repeatable load");
                    for (std::size_t i = 0; i < vertices.size(); ++i)
                        Require(vertices[i].x == mesh.Vertices()[i].x && vertices[i].y == mesh.Vertices()[i].y
                            && vertices[i].z == mesh.Vertices()[i].z, "Non-repeatable vertex");
                    for (std::size_t i = 0; i < edges.size(); ++i)
                        Require(edges[i].start == mesh.Edges()[i].start && edges[i].end == mesh.Edges()[i].end,
                            "Non-repeatable edge");
                    if (first) ++accepted;
                }
                Require(accepted >= 100 && accepted < 1000, "Mutation suite needs valid and invalid inputs");
                std::cout << "Mutation corpus: 1000 inputs, " << accepted << " accepted, "
                    << 1000 - accepted << " rejected; each replayed twice\n";
            }}
        };
        int failures = 0;
        for (const auto& item : cases) {
            try { item.second(); std::cout << "PASS: " << item.first << '\n'; }
            catch (const std::exception& ex) { ++failures; std::cerr << "FAIL: " << item.first << ": " << ex.what() << '\n'; }
        }
        std::cout << cases.size() << " named cases, " << failures << " failures\n";
        return failures == 0 ? 0 : 1;
    } catch (const std::exception& ex) {
        std::cerr << "Fixture failure: " << ex.what() << '\n';
        return 1;
    }
}
