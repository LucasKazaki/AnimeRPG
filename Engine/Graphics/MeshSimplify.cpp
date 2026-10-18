#include "Engine/Graphics/MeshSimplify.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <map>
#include <queue>
#include <string>
#include <utility>

namespace Astral::Graphics {

using namespace Math;

namespace {

// Symmetric 4x4 error quadric (Garland & Heckbert), double precision.
struct Quadric {
    double a2{}, ab{}, ac{}, ad{}, b2{}, bc{}, bd{}, c2{}, cd{}, d2{};

    static Quadric Plane(double a, double b, double c, double d, double weight) {
        Quadric q;
        q.a2 = a * a * weight; q.ab = a * b * weight; q.ac = a * c * weight; q.ad = a * d * weight;
        q.b2 = b * b * weight; q.bc = b * c * weight; q.bd = b * d * weight;
        q.c2 = c * c * weight; q.cd = c * d * weight; q.d2 = d * d * weight;
        return q;
    }
    void operator+=(const Quadric& o) {
        a2 += o.a2; ab += o.ab; ac += o.ac; ad += o.ad; b2 += o.b2;
        bc += o.bc; bd += o.bd; c2 += o.c2; cd += o.cd; d2 += o.d2;
    }
    // Sum of weighted squared distances of p to the accumulated planes.
    double Evaluate(Vec3 p) const {
        const double x = p.x, y = p.y, z = p.z;
        const double value = a2 * x * x + 2.0 * ab * x * y + 2.0 * ac * x * z + 2.0 * ad * x + b2 * y * y + 2.0 * bc * y * z
            + 2.0 * bd * y + c2 * z * z + 2.0 * cd * z + d2;
        return value > 0.0 ? value : 0.0;
    }
};

std::array<std::uint32_t, 3> PositionKey(Vec3 p) {
    std::array<std::uint32_t, 3> key{};
    const float values[3] = {p.x == 0.0f ? 0.0f : p.x, p.y == 0.0f ? 0.0f : p.y, p.z == 0.0f ? 0.0f : p.z};
    std::memcpy(key.data(), values, sizeof(values));
    return key;
}

bool SameAttributes(const Vertex& a, const Vertex& b) {
    return std::memcmp(&a.normal, &b.normal, sizeof(a.normal)) == 0 && std::memcmp(&a.uv, &b.uv, sizeof(a.uv)) == 0
        && std::memcmp(&a.color, &b.color, sizeof(a.color)) == 0;
}

struct Candidate {
    double cost{};
    std::uint32_t from{}, to{};
    std::uint32_t versionFrom{}, versionTo{};
    bool operator>(const Candidate& other) const {
        if (cost != other.cost) return cost > other.cost;
        if (from != other.from) return from > other.from; // deterministic ties
        return to > other.to;
    }
};

class Simplifier {
public:
    Simplifier(const MeshData& mesh, const SimplifyOptions& options) : mesh_(mesh), options_(options) {}

    bool Run(MeshData& out, SimplifyStats& stats);

private:
    void Weld();
    void BuildTopology();
    void Push(std::uint32_t from, std::uint32_t to);
    bool CanCollapse(std::uint32_t from, std::uint32_t to) const;
    void Collapse(std::uint32_t from, std::uint32_t to);
    std::uint32_t VertexAt(std::uint32_t position, std::uint32_t like) const;
    std::vector<std::uint32_t> Neighbours(std::uint32_t position) const;

    const MeshData& mesh_;
    const SimplifyOptions& options_;
    std::vector<std::uint32_t> vertexPosition_;              // vertex -> welded position
    std::vector<std::uint32_t> vertexCanonical_;             // vertex -> first identical vertex
    std::vector<Vec3> positions_;
    std::vector<std::vector<std::uint32_t>> positionVertices_; // distinct vertices per position
    std::vector<std::array<std::uint32_t, 3>> triangles_;    // vertex indices
    std::vector<bool> triangleAlive_;
    std::vector<std::vector<std::uint32_t>> incident_;       // position -> triangles
    std::vector<Quadric> quadrics_;
    std::vector<bool> locked_, dead_;
    std::vector<std::uint32_t> version_;
    std::priority_queue<Candidate, std::vector<Candidate>, std::greater<Candidate>> heap_;
    std::size_t aliveTriangles_{};
};

void Simplifier::Weld() {
    const std::size_t count = mesh_.vertices.size();
    vertexPosition_.resize(count);
    vertexCanonical_.resize(count);
    std::map<std::array<std::uint32_t, 3>, std::uint32_t> welded;
    for (std::size_t v = 0; v < count; ++v) {
        const auto inserted = welded.emplace(PositionKey(mesh_.vertices[v].position), static_cast<std::uint32_t>(positions_.size()));
        if (inserted.second) {
            positions_.push_back(mesh_.vertices[v].position);
            positionVertices_.emplace_back();
        }
        const std::uint32_t position = inserted.first->second;
        vertexPosition_[v] = position;
        // Vertices identical in every attribute are one vertex (split only by the exporter).
        std::uint32_t canonical = static_cast<std::uint32_t>(v);
        for (std::uint32_t other : positionVertices_[position]) {
            const bool sameSkin = mesh_.skin.empty()
                || (std::memcmp(&mesh_.skin[other], &mesh_.skin[v], sizeof(SkinInfluence)) == 0);
            if (SameAttributes(mesh_.vertices[other], mesh_.vertices[v]) && sameSkin) {
                canonical = other;
                break;
            }
        }
        vertexCanonical_[v] = canonical;
        if (canonical == v) positionVertices_[position].push_back(static_cast<std::uint32_t>(v));
    }
}

void Simplifier::BuildTopology() {
    const std::size_t positionCount = positions_.size();
    incident_.assign(positionCount, {});
    quadrics_.assign(positionCount, {});
    locked_.assign(positionCount, false);
    dead_.assign(positionCount, false);
    version_.assign(positionCount, 0);
    std::map<std::pair<std::uint32_t, std::uint32_t>, int> edgeUse;
    for (std::size_t i = 0; i + 2 < mesh_.indices.size(); i += 3) {
        std::array<std::uint32_t, 3> tri{vertexCanonical_[mesh_.indices[i]], vertexCanonical_[mesh_.indices[i + 1]],
            vertexCanonical_[mesh_.indices[i + 2]]};
        const std::uint32_t p0 = vertexPosition_[tri[0]], p1 = vertexPosition_[tri[1]], p2 = vertexPosition_[tri[2]];
        if (p0 == p1 || p1 == p2 || p0 == p2) continue; // degenerate
        const Vec3 cross = Cross(positions_[p1] - positions_[p0], positions_[p2] - positions_[p0]);
        const float length = Length(cross);
        if (!(length > 1.0e-20f)) continue;
        const auto index = static_cast<std::uint32_t>(triangles_.size());
        triangles_.push_back(tri);
        triangleAlive_.push_back(true);
        const Vec3 n = cross / length;
        const double d = -static_cast<double>(Dot(n, positions_[p0]));
        const Quadric plane = Quadric::Plane(n.x, n.y, n.z, d, 1.0);
        for (std::uint32_t p : {p0, p1, p2}) {
            incident_[p].push_back(index);
            quadrics_[p] += plane;
        }
        const std::uint32_t ps[3] = {p0, p1, p2};
        for (int e = 0; e < 3; ++e) ++edgeUse[{std::min(ps[e], ps[(e + 1) % 3]), std::max(ps[e], ps[(e + 1) % 3])}];
    }
    aliveTriangles_ = triangles_.size();
    // Seams (several distinct vertices at one position) cannot move without
    // losing an attribute discontinuity.
    for (std::size_t p = 0; p < positionCount; ++p) {
        if (positionVertices_[p].size() > 1) locked_[p] = true;
    }
    // Borders: lock, or hold with planes perpendicular to the boundary.
    for (std::size_t t = 0; t < triangles_.size(); ++t) {
        const std::uint32_t ps[3] = {vertexPosition_[triangles_[t][0]], vertexPosition_[triangles_[t][1]], vertexPosition_[triangles_[t][2]]};
        const Vec3 normal = Normalize(Cross(positions_[ps[1]] - positions_[ps[0]], positions_[ps[2]] - positions_[ps[0]]));
        for (int e = 0; e < 3; ++e) {
            const std::uint32_t a = ps[e], b = ps[(e + 1) % 3];
            if (edgeUse[{std::min(a, b), std::max(a, b)}] != 1) continue;
            if (options_.lockBorders) {
                locked_[a] = locked_[b] = true;
                continue;
            }
            const Vec3 edge = positions_[b] - positions_[a];
            const Vec3 side = Normalize(Cross(edge, normal));
            if (LengthSquared(side) < 0.5f) continue;
            const double d = -static_cast<double>(Dot(side, positions_[a]));
            const double weight = static_cast<double>(options_.borderWeight);
            const Quadric plane = Quadric::Plane(side.x, side.y, side.z, d, weight);
            quadrics_[a] += plane;
            quadrics_[b] += plane;
        }
    }
}

std::vector<std::uint32_t> Simplifier::Neighbours(std::uint32_t position) const {
    std::vector<std::uint32_t> result;
    for (std::uint32_t t : incident_[position]) {
        if (!triangleAlive_[t]) continue;
        for (std::uint32_t v : triangles_[t]) {
            const std::uint32_t p = vertexPosition_[v];
            if (p != position) result.push_back(p);
        }
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

void Simplifier::Push(std::uint32_t from, std::uint32_t to) {
    if (locked_[from] || dead_[from] || dead_[to]) return;
    Quadric q = quadrics_[from];
    q += quadrics_[to];
    heap_.push({q.Evaluate(positions_[to]), from, to, version_[from], version_[to]});
}

bool Simplifier::CanCollapse(std::uint32_t from, std::uint32_t to) const {
    // Link condition: shared neighbours must be exactly the far corners of the
    // triangles on the edge, otherwise the collapse pinches the surface.
    std::size_t shared = 0;
    for (std::uint32_t t : incident_[from]) {
        if (!triangleAlive_[t]) continue;
        for (std::uint32_t v : triangles_[t]) {
            if (vertexPosition_[v] == to) ++shared;
        }
    }
    if (shared == 0) return false;
    const std::vector<std::uint32_t> a = Neighbours(from), b = Neighbours(to);
    std::vector<std::uint32_t> common;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(common));
    if (common.size() > shared) return false;
    // No triangle may flip or collapse to a sliver when `from` moves onto `to`.
    const Vec3 target = positions_[to];
    for (std::uint32_t t : incident_[from]) {
        if (!triangleAlive_[t]) continue;
        const std::uint32_t p[3] = {vertexPosition_[triangles_[t][0]], vertexPosition_[triangles_[t][1]], vertexPosition_[triangles_[t][2]]};
        if (p[0] == to || p[1] == to || p[2] == to) continue; // removed by the collapse
        Vec3 before[3], after[3];
        for (int k = 0; k < 3; ++k) {
            before[k] = positions_[p[k]];
            after[k] = p[k] == from ? target : before[k];
        }
        const Vec3 n0 = Cross(before[1] - before[0], before[2] - before[0]);
        const Vec3 n1 = Cross(after[1] - after[0], after[2] - after[0]);
        const float l0 = Length(n0), l1 = Length(n1);
        if (!(l1 > 1.0e-12f) || Dot(n0, n1) < 0.2f * l0 * l1) return false;
    }
    return true;
}

std::uint32_t Simplifier::VertexAt(std::uint32_t position, std::uint32_t like) const {
    const std::vector<std::uint32_t>& candidates = positionVertices_[position];
    if (candidates.size() == 1) return candidates.front();
    // Seam target: keep the attribute set closest to the moving corner's.
    const Vertex& reference = mesh_.vertices[like];
    std::uint32_t best = candidates.front();
    float bestScore = std::numeric_limits<float>::max();
    for (std::uint32_t v : candidates) {
        const Vertex& candidate = mesh_.vertices[v];
        const Vec2 duv{candidate.uv.x - reference.uv.x, candidate.uv.y - reference.uv.y};
        const float score = duv.x * duv.x + duv.y * duv.y + (1.0f - Dot(candidate.normal, reference.normal));
        if (score < bestScore) {
            bestScore = score;
            best = v;
        }
    }
    return best;
}

void Simplifier::Collapse(std::uint32_t from, std::uint32_t to) {
    for (std::uint32_t t : incident_[from]) {
        if (!triangleAlive_[t]) continue;
        auto& tri = triangles_[t];
        bool hasTo = false;
        for (std::uint32_t v : tri) hasTo = hasTo || vertexPosition_[v] == to;
        if (hasTo) {
            triangleAlive_[t] = false;
            --aliveTriangles_;
            continue;
        }
        for (std::uint32_t& v : tri) {
            if (vertexPosition_[v] == from) v = VertexAt(to, v);
        }
        incident_[to].push_back(t);
    }
    incident_[from].clear();
    dead_[from] = true;
    quadrics_[to] += quadrics_[from];
    ++version_[to];
    // Drop dead triangles from the target's list, then re-queue its edges.
    auto& list = incident_[to];
    list.erase(std::remove_if(list.begin(), list.end(), [this](std::uint32_t t) { return !triangleAlive_[t]; }), list.end());
    std::sort(list.begin(), list.end());
    list.erase(std::unique(list.begin(), list.end()), list.end());
    for (std::uint32_t neighbour : Neighbours(to)) {
        Push(to, neighbour);
        Push(neighbour, to);
    }
}

bool Simplifier::Run(MeshData& out, SimplifyStats& stats) {
    Weld();
    BuildTopology();
    stats.trianglesIn = mesh_.TriangleCount();
    std::size_t target = options_.targetTriangles;
    if (target == 0) {
        const float ratio = std::clamp(options_.targetRatio, 0.0f, 1.0f);
        target = static_cast<std::size_t>(std::floor(static_cast<float>(triangles_.size()) * ratio));
    }
    const double maxErrorSquared = std::isfinite(options_.maxError)
        ? static_cast<double>(options_.maxError) * static_cast<double>(options_.maxError)
        : std::numeric_limits<double>::infinity();
    for (std::uint32_t p = 0; p < positions_.size(); ++p) {
        for (std::uint32_t neighbour : Neighbours(p)) Push(p, neighbour);
    }
    while (aliveTriangles_ > target && !heap_.empty()) {
        const Candidate candidate = heap_.top();
        heap_.pop();
        if (dead_[candidate.from] || dead_[candidate.to] || version_[candidate.from] != candidate.versionFrom
            || version_[candidate.to] != candidate.versionTo) {
            continue; // stale
        }
        if (candidate.cost > maxErrorSquared) break;
        if (!CanCollapse(candidate.from, candidate.to)) continue;
        Collapse(candidate.from, candidate.to);
        ++stats.collapses;
        stats.error = std::max(stats.error, static_cast<float>(std::sqrt(candidate.cost)));
    }
    // Compact: surviving triangles, referenced vertices only.
    out = MeshData{};
    out.name = mesh_.name;
    std::vector<std::uint32_t> remap(mesh_.vertices.size(), 0xFFFFFFFFu);
    for (std::size_t t = 0; t < triangles_.size(); ++t) {
        if (!triangleAlive_[t]) continue;
        for (std::uint32_t v : triangles_[t]) {
            if (remap[v] == 0xFFFFFFFFu) {
                remap[v] = static_cast<std::uint32_t>(out.vertices.size());
                out.vertices.push_back(mesh_.vertices[v]);
                if (!mesh_.skin.empty()) out.skin.push_back(mesh_.skin[v]);
            }
            out.indices.push_back(remap[v]);
        }
    }
    out.ComputeBounds();
    stats.trianglesOut = out.TriangleCount();
    stats.verticesOut = out.vertices.size();
    return true;
}

} // namespace

bool SimplifyMesh(const MeshData& in, const SimplifyOptions& options, MeshData& out, SimplifyStats* stats) {
    std::string error;
    if (!in.Validate(error)) return false;
    SimplifyStats local;
    Simplifier simplifier(in, options);
    MeshData result;
    if (!simplifier.Run(result, local)) return false;
    out = std::move(result);
    if (stats) *stats = local;
    return true;
}

float ScreenSize(float radius, float distance, float verticalFovRadians) {
    if (!(radius > 0.0f) || !std::isfinite(radius)) return 0.0f;
    if (!(distance > radius)) return 1.0e6f; // camera inside the bounds
    const float halfHeight = distance * std::tan(std::clamp(verticalFovRadians, 1.0e-3f, 3.1f) * 0.5f);
    return radius / halfHeight;
}

int LodGroup::Select(float screenSize, int current, float hysteresis) const {
    const int count = static_cast<int>(levels.size());
    if (count == 0) return -1;
    if (!std::isfinite(screenSize)) screenSize = 0.0f;
    int raw = count - 1;
    for (int i = 0; i < count; ++i) {
        if (screenSize >= levels[static_cast<std::size_t>(i)].minScreenSize) {
            raw = i;
            break;
        }
    }
    if (current < 0 || current >= count || raw == current) return raw;
    hysteresis = std::clamp(hysteresis, 0.0f, 0.9f);
    int chosen = current;
    if (raw < current) {
        // Refine only once the size is clearly above the finer level's threshold.
        for (int i = current - 1; i >= raw; --i) {
            if (screenSize >= levels[static_cast<std::size_t>(i)].minScreenSize * (1.0f + hysteresis)) chosen = i;
        }
    } else {
        // Coarsen only once the size is clearly below the current level's threshold.
        for (int i = current; i < raw; ++i) {
            if (screenSize < levels[static_cast<std::size_t>(i)].minScreenSize * (1.0f - hysteresis)) chosen = i + 1;
        }
    }
    return chosen;
}

LodGroup BuildLodGroup(std::shared_ptr<const MeshData> base, const std::vector<float>& ratios, const std::vector<float>& screenSizes) {
    LodGroup group;
    if (!base) return group;
    auto threshold = [&](std::size_t level) {
        if (level < screenSizes.size()) return screenSizes[level];
        return group.levels.empty() ? 0.5f : group.levels.back().minScreenSize * 0.5f;
    };
    group.levels.push_back({base, threshold(0), 0.0f});
    std::size_t previous = base->TriangleCount();
    for (std::size_t i = 0; i < ratios.size(); ++i) {
        SimplifyOptions options;
        options.targetRatio = ratios[i];
        auto mesh = std::make_shared<MeshData>();
        SimplifyStats stats;
        if (!SimplifyMesh(*base, options, *mesh, &stats) || mesh->TriangleCount() == 0 || mesh->TriangleCount() >= previous) continue;
        previous = mesh->TriangleCount();
        group.levels.push_back({std::move(mesh), threshold(group.levels.size()), stats.error});
    }
    // The coarsest level covers everything below the previous threshold.
    group.levels.back().minScreenSize = 0.0f;
    return group;
}

} // namespace Astral::Graphics
