#include "Engine/Physics/Collision.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace Astral::Physics {

using namespace Math;

namespace {

bool IsRounded(const Shape& s) { return s.type == ShapeType::Sphere || s.type == ShapeType::Capsule; }

void CoreSegment(const Shape& s, const Pose& pose, Vec3& a, Vec3& b) {
    if (s.type == ShapeType::Capsule) {
        const Vec3 axis = pose.TransformVector({0.0f, s.halfHeight, 0.0f});
        a = pose.position - axis;
        b = pose.position + axis;
    } else {
        a = b = pose.position;
    }
}

void AddPoint(Manifold& m, Vec3 position, float penetration) {
    for (int i = 0; i < m.count; ++i) {
        if (DistanceSquared(m.points[i].position, position) < 1.0e-6f) {
            m.points[i].penetration = std::max(m.points[i].penetration, penetration);
            return;
        }
    }
    if (m.count < 4) m.points[m.count++] = {position, penetration};
}

Vec3 AnyPerpendicular(Vec3 v) {
    Vec3 t, b;
    OrthonormalBasis(Normalize(v, {0, 1, 0}), t, b);
    return t;
}

// ---------------------------------------------------------------- rounded vs rounded
bool CollideRounded(const Shape& a, const Pose& pa, const Shape& b, const Pose& pb, Manifold& m) {
    Vec3 a0, a1, b0, b1;
    CoreSegment(a, pa, a0, a1);
    CoreSegment(b, pb, b0, b1);
    float s, t;
    Vec3 c1, c2;
    const float distanceSquared = ClosestPointsSegmentSegment(a0, a1, b0, b1, s, t, c1, c2);
    const float radii = a.radius + b.radius;
    if (distanceSquared > radii * radii) return false;
    const float distance = std::sqrt(distanceSquared);
    Vec3 normal;
    if (distance > 1.0e-6f) normal = (c2 - c1) / distance;
    else {
        const Vec3 centers = pb.position - pa.position;
        normal = LengthSquared(centers) > 1.0e-10f ? Normalize(centers) : Vec3{0, 1, 0};
        if (a.type == ShapeType::Capsule) normal = Normalize(ProjectOnPlane(normal, Normalize(a1 - a0, {0, 1, 0})), AnyPerpendicular(a1 - a0));
    }
    m.normal = normal;
    m.count = 0;
    AddPoint(m, (c1 + normal * a.radius + c2 - normal * b.radius) * 0.5f, radii - distance);
    // Parallel capsules resting side by side get a second contact to stop rolling.
    if (a.type == ShapeType::Capsule && b.type == ShapeType::Capsule) {
        const Vec3 da = a1 - a0, db = b1 - b0;
        const float la = Length(da), lb = Length(db);
        if (la > 1.0e-5f && lb > 1.0e-5f && std::fabs(Dot(da / la, db / lb)) > 0.98f) {
            for (const Vec3& endpoint : {b0, b1}) {
                const Vec3 onA = ClosestPointOnSegment(endpoint, a0, a1);
                const Vec3 onB = ClosestPointOnSegment(onA, b0, b1);
                const float d = Distance(onA, onB);
                if (d < radii) AddPoint(m, (onA + normal * a.radius + onB - normal * b.radius) * 0.5f, radii - d);
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------- rounded vs box
// Contact with normal pointing from the box toward the rounded shape.
bool CollideRoundedBox(const Shape& r, const Pose& pr, const Shape& box, const Pose& pb, Manifold& m) {
    Vec3 w0, w1;
    CoreSegment(r, pr, w0, w1);
    const Vec3 a = pb.InverseTransformPoint(w0);
    const Vec3 b = pb.InverseTransformPoint(w1);
    const Vec3 e = box.halfExtents;
    float t;
    Vec3 onSegment, onBox;
    ClosestSegmentBox(a, b, e, t, onSegment, onBox);
    const Vec3 delta = onSegment - onBox;
    const float distance = Length(delta);
    m.count = 0;
    if (distance > 1.0e-6f) {
        if (distance > r.radius) return false;
        const Vec3 localNormal = delta / distance;
        m.normal = pb.TransformVector(localNormal);
        AddPoint(m, pb.TransformPoint(onBox), r.radius - distance);
        if (r.type == ShapeType::Capsule) {
            for (const Vec3& endpoint : {a, b}) {
                const Vec3 q = Clamp(endpoint, -e, e);
                const Vec3 d = endpoint - q;
                const float dl = Length(d);
                if (dl > 1.0e-6f && dl < r.radius && Dot(d / dl, localNormal) > 0.9f)
                    AddPoint(m, pb.TransformPoint(q), r.radius - dl);
            }
        }
        return true;
    }
    // Deep: the core segment intersects the box. Minimum-push SAT over box axes
    // and segment x box-axis cross products.
    Vec3 axes[6];
    int axisCount = 0;
    axes[axisCount++] = {1, 0, 0};
    axes[axisCount++] = {0, 1, 0};
    axes[axisCount++] = {0, 0, 1};
    const Vec3 direction = b - a;
    if (LengthSquared(direction) > 1.0e-10f) {
        for (const Vec3& boxAxis : {Vec3{1, 0, 0}, Vec3{0, 1, 0}, Vec3{0, 0, 1}}) {
            const Vec3 c = Cross(direction, boxAxis);
            if (LengthSquared(c) > 1.0e-8f) axes[axisCount++] = Normalize(c);
        }
    }
    float bestPush = 1.0e30f;
    Vec3 bestNormal{0, 1, 0};
    for (int i = 0; i < axisCount; ++i) {
        const Vec3 l = axes[i];
        const float boxRadius = e.x * std::fabs(l.x) + e.y * std::fabs(l.y) + e.z * std::fabs(l.z);
        const float pa = Dot(l, a), pbv = Dot(l, b);
        const float capsuleMin = std::min(pa, pbv) - r.radius, capsuleMax = std::max(pa, pbv) + r.radius;
        const float pushPositive = boxRadius - capsuleMin;  // move capsule along +l
        const float pushNegative = capsuleMax + boxRadius;  // move capsule along -l
        if (pushPositive < bestPush) {
            bestPush = pushPositive;
            bestNormal = l;
        }
        if (pushNegative < bestPush) {
            bestPush = pushNegative;
            bestNormal = -l;
        }
    }
    m.normal = pb.TransformVector(bestNormal);
    const Vec3 deepest = Dot(a, bestNormal) < Dot(b, bestNormal) ? a : b;
    AddPoint(m, pb.TransformPoint(Clamp(deepest - bestNormal * r.radius, -e, e)), bestPush);
    return true;
}

// ---------------------------------------------------------------- box vs box
struct Obb {
    Vec3 center;
    Vec3 axis[3];
    float extent[3];
};

Obb MakeObb(const Shape& s, const Pose& p) {
    Obb o;
    o.center = p.position;
    o.axis[0] = p.TransformVector({1, 0, 0});
    o.axis[1] = p.TransformVector({0, 1, 0});
    o.axis[2] = p.TransformVector({0, 0, 1});
    o.extent[0] = s.halfExtents.x;
    o.extent[1] = s.halfExtents.y;
    o.extent[2] = s.halfExtents.z;
    return o;
}

int ClipPolygon(const Vec3* in, int count, Vec3 planeNormal, float planeOffset, Vec3* out) {
    // Keeps the part with dot(n, p) <= offset.
    int produced = 0;
    for (int i = 0; i < count; ++i) {
        const Vec3 p = in[i], q = in[(i + 1) % count];
        const float dp = Dot(planeNormal, p) - planeOffset;
        const float dq = Dot(planeNormal, q) - planeOffset;
        if (dp <= 0.0f) out[produced++] = p;
        if ((dp <= 0.0f) != (dq <= 0.0f)) out[produced++] = p + (q - p) * (dp / (dp - dq));
    }
    return produced;
}

void ReduceContacts(const ContactPoint* candidates, int candidateCount, Vec3 n, Manifold& m);

void FaceContacts(const Obb& reference, int axis, float sign, const Obb& incident, Manifold& m) {
    const Vec3 n = reference.axis[axis] * sign; // outward normal of the reference face
    // Incident face: most anti-parallel to n.
    int incidentAxis = 0;
    float best = -1.0f;
    for (int k = 0; k < 3; ++k) {
        const float d = std::fabs(Dot(incident.axis[k], n));
        if (d > best) {
            best = d;
            incidentAxis = k;
        }
    }
    const float incidentSign = Dot(incident.axis[incidentAxis], n) > 0.0f ? -1.0f : 1.0f;
    const Vec3 faceCenter = incident.center + incident.axis[incidentAxis] * (incidentSign * incident.extent[incidentAxis]);
    const int u = (incidentAxis + 1) % 3, v = (incidentAxis + 2) % 3;
    const Vec3 du = incident.axis[u] * incident.extent[u], dv = incident.axis[v] * incident.extent[v];
    Vec3 polygon[16] = {faceCenter + du + dv, faceCenter - du + dv, faceCenter - du - dv, faceCenter + du - dv};
    Vec3 scratch[16];
    int count = 4;
    const int ru = (axis + 1) % 3, rv = (axis + 2) % 3;
    const Vec3 refCenter = reference.center + n * reference.extent[axis];
    const Vec3 sides[4] = {reference.axis[ru], -reference.axis[ru], reference.axis[rv], -reference.axis[rv]};
    const float sideExtent[4] = {reference.extent[ru], reference.extent[ru], reference.extent[rv], reference.extent[rv]};
    for (int s = 0; s < 4 && count > 0; ++s) {
        count = ClipPolygon(polygon, count, sides[s], Dot(sides[s], reference.center) + sideExtent[s], scratch);
        for (int i = 0; i < count; ++i) polygon[i] = scratch[i];
    }
    ContactPoint candidates[16];
    int candidateCount = 0;
    for (int i = 0; i < count; ++i) {
        const float depth = Dot(refCenter - polygon[i], n);
        if (depth >= -1.0e-4f) candidates[candidateCount++] = {polygon[i] + n * (depth * 0.5f), std::max(0.0f, depth)};
    }
    ReduceContacts(candidates, candidateCount, n, m);
}

// Keeps at most four contacts: deepest, farthest from it, then the two that
// maximise the contact area on either side (stable support polygons).
void ReduceContacts(const ContactPoint* candidates, int candidateCount, Vec3 n, Manifold& m) {
    if (candidateCount <= 0) return;
    if (candidateCount <= 4) {
        for (int i = 0; i < candidateCount; ++i) AddPoint(m, candidates[i].position, candidates[i].penetration);
        return;
    }
    int chosen[4] = {0, -1, -1, -1};
    for (int i = 1; i < candidateCount; ++i)
        if (candidates[i].penetration > candidates[chosen[0]].penetration) chosen[0] = i;
    float bestDistance = -1.0f;
    for (int i = 0; i < candidateCount; ++i) {
        const float d = DistanceSquared(candidates[i].position, candidates[chosen[0]].position);
        if (d > bestDistance) {
            bestDistance = d;
            chosen[1] = i;
        }
    }
    const Vec3 p0 = candidates[chosen[0]].position, p1 = candidates[chosen[1]].position;
    float bestPositive = 0.0f, bestNegative = 0.0f;
    for (int i = 0; i < candidateCount; ++i) {
        const float area = Dot(Cross(p1 - p0, candidates[i].position - p0), n);
        if (area > bestPositive) {
            bestPositive = area;
            chosen[2] = i;
        }
        if (area < bestNegative) {
            bestNegative = area;
            chosen[3] = i;
        }
    }
    for (int index : chosen) {
        if (index >= 0) AddPoint(m, candidates[index].position, candidates[index].penetration);
    }
}

bool CollideBoxes(const Shape& a, const Pose& pa, const Shape& b, const Pose& pb, Manifold& m) {
    const Obb A = MakeObb(a, pa), B = MakeObb(b, pb);
    const Vec3 t = B.center - A.center;
    float R[3][3], absR[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            R[i][j] = Dot(A.axis[i], B.axis[j]);
            absR[i][j] = std::fabs(R[i][j]) + 1.0e-6f;
        }
    // Best face of A, face of B and edge pair (separation = negative penetration).
    float bestA = -1.0e30f, bestB = -1.0e30f, bestE = -1.0e30f;
    int axisA = 0, axisB = 0, edgeI = 0, edgeJ = 0;
    Vec3 normalA, normalB, normalE;
    for (int i = 0; i < 3; ++i) {
        const float ra = A.extent[i];
        const float rb = B.extent[0] * absR[i][0] + B.extent[1] * absR[i][1] + B.extent[2] * absR[i][2];
        const float s = Dot(t, A.axis[i]);
        const float separation = std::fabs(s) - (ra + rb);
        if (separation > 0.0f) return false;
        if (separation > bestA) {
            bestA = separation;
            axisA = i;
            normalA = A.axis[i] * (s >= 0.0f ? 1.0f : -1.0f);
        }
    }
    for (int j = 0; j < 3; ++j) {
        const float ra = A.extent[0] * absR[0][j] + A.extent[1] * absR[1][j] + A.extent[2] * absR[2][j];
        const float rb = B.extent[j];
        const float s = Dot(t, B.axis[j]);
        const float separation = std::fabs(s) - (ra + rb);
        if (separation > 0.0f) return false;
        if (separation > bestB) {
            bestB = separation;
            axisB = j;
            normalB = B.axis[j] * (s >= 0.0f ? 1.0f : -1.0f);
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            Vec3 l = Cross(A.axis[i], B.axis[j]);
            const float length = Length(l);
            if (length < 1.0e-4f) continue;
            l = l / length;
            const float ra = A.extent[0] * std::fabs(Dot(A.axis[0], l)) + A.extent[1] * std::fabs(Dot(A.axis[1], l))
                + A.extent[2] * std::fabs(Dot(A.axis[2], l));
            const float rb = B.extent[0] * std::fabs(Dot(B.axis[0], l)) + B.extent[1] * std::fabs(Dot(B.axis[1], l))
                + B.extent[2] * std::fabs(Dot(B.axis[2], l));
            const float s = Dot(t, l);
            const float separation = std::fabs(s) - (ra + rb);
            if (separation > 0.0f) return false;
            if (separation > bestE) {
                bestE = separation;
                edgeI = i;
                edgeJ = j;
                normalE = l * (s >= 0.0f ? 1.0f : -1.0f);
            }
        }
    }
    constexpr float kRelative = 0.95f, kAbsolute = 0.005f;
    m.count = 0;
    const bool preferB = bestB > kRelative * bestA + kAbsolute;
    const float bestFace = preferB ? bestB : bestA;
    if (bestE > kRelative * bestFace + kAbsolute) {
        // Edge-edge: closest points between the two supporting edges.
        Vec3 edgeCenterA = A.center, edgeCenterB = B.center;
        for (int k = 0; k < 3; ++k) {
            if (k != edgeI) edgeCenterA += A.axis[k] * (A.extent[k] * (Dot(A.axis[k], normalE) > 0.0f ? 1.0f : -1.0f));
            if (k != edgeJ) edgeCenterB += B.axis[k] * (B.extent[k] * (Dot(B.axis[k], normalE) < 0.0f ? 1.0f : -1.0f));
        }
        const Vec3 a0 = edgeCenterA - A.axis[edgeI] * A.extent[edgeI], a1 = edgeCenterA + A.axis[edgeI] * A.extent[edgeI];
        const Vec3 b0 = edgeCenterB - B.axis[edgeJ] * B.extent[edgeJ], b1 = edgeCenterB + B.axis[edgeJ] * B.extent[edgeJ];
        float s, u;
        Vec3 c1, c2;
        ClosestPointsSegmentSegment(a0, a1, b0, b1, s, u, c1, c2);
        m.normal = normalE;
        AddPoint(m, (c1 + c2) * 0.5f, -bestE);
        return true;
    }
    if (preferB) {
        // Reference face on B; its outward normal points from B toward A.
        FaceContacts(B, axisB, Dot(normalB, B.axis[axisB]) > 0.0f ? -1.0f : 1.0f, A, m);
        m.normal = normalB;
    } else {
        FaceContacts(A, axisA, Dot(normalA, A.axis[axisA]) > 0.0f ? 1.0f : -1.0f, B, m);
        m.normal = normalA;
    }
    if (m.count == 0) AddPoint(m, (A.center + B.center) * 0.5f, -bestFace);
    return true;
}


// ---------------------------------------------------------------- generic cast
// Conservative advancement: distanceAt(offset, normal, point) is the signed
// distance with the moving shape translated by `offset`. The distance between
// convex sets is a convex function of translation, so along the motion
// g(t) >= g(0) + g'(0) t with g'(0) = dot(direction, normal). A non-negative
// slope means the shapes can never meet; otherwise stepping to the tangent's
// root is always safe and converges monotonically (Newton from below). This
// handles grazing and parallel motion in a few iterations.
template <typename DistanceFn>
bool ConservativeCast(DistanceFn&& distanceAt, Vec3 motion, float tolerance, ShapeCastResult& result) {
    result = {};
    const float length = Length(motion);
    Vec3 normal, point;
    float distance = distanceAt(Vec3{}, normal, point);
    if (distance < -tolerance) {
        result.fraction = 0.0f;
        result.normal = normal;
        result.point = point;
        result.startPenetrating = true;
        return true;
    }
    if (length < 1.0e-9f) return false;
    const Vec3 direction = motion / length;
    // Already touching: sliding along or leaving a resting contact (e.g. walking
    // on the ground) is not a hit, as long as the whole motion stays within the
    // tolerance (by convexity g(t) >= g(0) + slope * t). A motion that would dig
    // deeper is blocked where it starts, so collide-and-slide can project it.
    if (distance <= tolerance && distance + std::min(0.0f, Dot(direction, normal)) * length >= -tolerance) return false;
    float travelled = 0.0f;
    for (int iteration = 0; iteration < 64; ++iteration) {
        if (distance <= tolerance) {
            result.fraction = Saturate(travelled / length);
            result.normal = normal;
            result.point = point;
            return true;
        }
        const float slope = Dot(direction, normal);
        if (slope >= -1.0e-6f) return false;
        travelled += distance / -slope;
        if (travelled >= length) return false;
        distance = distanceAt(direction * travelled, normal, point);
        if (distance < 0.0f) {
            // Overshoot from numerical error: back off to a safe spot.
            travelled = std::max(0.0f, travelled + distance - tolerance);
            result.fraction = Saturate(travelled / length);
            result.normal = normal;
            result.point = point;
            return true;
        }
    }
    result.fraction = Saturate(travelled / length);
    result.normal = normal;
    result.point = point;
    return true;
}

// ---------------------------------------------------------------- triangle meshes
// Voronoi feature of the closest point on a triangle.
enum TriangleFeature : int { kFace = 0, kEdgeAB = 1, kEdgeBC = 2, kEdgeCA = 3, kVertexA = 4, kVertexB = 5, kVertexC = 6 };

// Ericson 5.1.5 (as Math::ClosestPointOnTriangle) that also reports the region.
Vec3 ClosestOnTriangle(Vec3 p, Vec3 a, Vec3 b, Vec3 c, int& feature) {
    const Vec3 ab = b - a, ac = c - a, ap = p - a;
    const float d1 = Dot(ab, ap), d2 = Dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) {
        feature = kVertexA;
        return a;
    }
    const Vec3 bp = p - b;
    const float d3 = Dot(ab, bp), d4 = Dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) {
        feature = kVertexB;
        return b;
    }
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        feature = kEdgeAB;
        return a + ab * (d1 / (d1 - d3));
    }
    const Vec3 cp = p - c;
    const float d5 = Dot(ab, cp), d6 = Dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) {
        feature = kVertexC;
        return c;
    }
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        feature = kEdgeCA;
        return a + ac * (d2 / (d2 - d6));
    }
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        feature = kEdgeBC;
        return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
    }
    const float denominator = 1.0f / (va + vb + vc);
    feature = kFace;
    return a + ab * (vb * denominator) + ac * (vc * denominator);
}

// Whether a feature may produce its own (non-face) normal. Vertices are active
// when either incident edge is; internal edges defer to the face normal.
bool FeatureActive(const TriangleMesh::Triangle& t, int feature) {
    switch (feature) {
    case kEdgeAB: return (t.activeEdges & 1u) != 0;
    case kEdgeBC: return (t.activeEdges & 2u) != 0;
    case kEdgeCA: return (t.activeEdges & 4u) != 0;
    case kVertexA: return (t.activeEdges & 5u) != 0;
    case kVertexB: return (t.activeEdges & 3u) != 0;
    case kVertexC: return (t.activeEdges & 6u) != 0;
    default: return true;
    }
}

struct SegmentTriangleResult {
    Vec3 onSegment{};
    Vec3 onTriangle{};
    float distance{};
    int feature{kFace};
    bool crossing{};
};

// Closest candidate so far. Kept as plain value code on purpose: MSVC x64
// Release builds dropped the edge candidates when they were recorded through a
// by-reference lambda capture of the returned result (see the
// CapsuleBesideATriangleMeasuresItsNearestVertex regression test).
struct SegmentTriangleCandidate {
    float distanceSquared{std::numeric_limits<float>::infinity()};
    Vec3 onSegment{};
    Vec3 onTriangle{};
    int feature{kFace};
};

void KeepCloser(SegmentTriangleCandidate& best, Vec3 onSegment, Vec3 onTriangle, int feature) {
    const float distanceSquared = DistanceSquared(onSegment, onTriangle);
    if (distanceSquared < best.distanceSquared) {
        best.distanceSquared = distanceSquared;
        best.onSegment = onSegment;
        best.onTriangle = onTriangle;
        best.feature = feature;
    }
}

// Segment p0-p1 against the triangle edge from-to (edge index 0 = AB, 1 = BC, 2 = CA).
void KeepCloserEdge(SegmentTriangleCandidate& best, Vec3 p0, Vec3 p1, Vec3 from, Vec3 to, int edge) {
    float s = 0.0f, w = 0.0f;
    Vec3 onSegment, onEdge;
    ClosestPointsSegmentSegment(p0, p1, from, to, s, w, onSegment, onEdge);
    const int feature = w <= 0.0f ? kVertexA + edge : (w >= 1.0f ? kVertexA + (edge + 1) % 3 : kEdgeAB + edge);
    KeepCloser(best, onSegment, onEdge, feature);
}

SegmentTriangleResult ClosestSegmentTriangle(Vec3 p0, Vec3 p1, const TriangleMesh::Triangle& t) {
    const Vec3 d = p1 - p0;
    const float length = Length(d);
    if (length > 1.0e-7f) {
        float hit = 0.0f, u = 0.0f, v = 0.0f;
        if (IntersectRayTriangle({p0, d / length}, t.a, t.b, t.c, length, hit, u, v)) {
            SegmentTriangleResult crossing;
            crossing.onSegment = crossing.onTriangle = p0 + d * (hit / length);
            crossing.crossing = true;
            return crossing;
        }
    }
    // Endpoints first so ties keep the face region.
    SegmentTriangleCandidate best;
    int feature0 = kFace;
    const Vec3 q0 = ClosestOnTriangle(p0, t.a, t.b, t.c, feature0);
    KeepCloser(best, p0, q0, feature0);
    if (length > 1.0e-7f) {
        int feature1 = kFace;
        const Vec3 q1 = ClosestOnTriangle(p1, t.a, t.b, t.c, feature1);
        KeepCloser(best, p1, q1, feature1);
        KeepCloserEdge(best, p0, p1, t.a, t.b, 0);
        KeepCloserEdge(best, p0, p1, t.b, t.c, 1);
        KeepCloserEdge(best, p0, p1, t.c, t.a, 2);
    }
    SegmentTriangleResult result;
    result.onSegment = best.onSegment;
    result.onTriangle = best.onTriangle;
    result.feature = best.feature;
    result.distance = std::sqrt(best.distanceSquared);
    return result;
}

// Signed distance from a rounded core (segment p0-p1 plus radius) to one
// triangle; `normal` points from the triangle toward the shape. A core that
// touches or passes through the face is pushed out of the front face. A contact
// on an internal (flat or concave) edge or vertex uses the face normal, with the
// depth needed to clear that point along it, so seams between neighbouring
// triangles never push sideways. Its point is where the shape meets the face
// plane (under the core), so it coincides with the neighbour's face contact
// instead of acting off-axis on a rolling sphere.
float RoundedTriangle(Vec3 p0, Vec3 p1, float radius, const TriangleMesh::Triangle& t, Vec3& normal, Vec3& point) {
    const SegmentTriangleResult closest = ClosestSegmentTriangle(p0, p1, t);
    if (closest.crossing || closest.distance < 1.0e-6f) {
        const float h0 = Dot(p0 - t.a, t.normal), h1 = Dot(p1 - t.a, t.normal);
        normal = t.normal;
        point = ClosestPointOnTriangle(h0 <= h1 ? p0 : p1, t.a, t.b, t.c);
        return std::min(std::min(h0, h1), 0.0f) - radius;
    }
    const float d = closest.distance;
    normal = (closest.onSegment - closest.onTriangle) / d;
    point = closest.onTriangle;
    if (d >= radius || FeatureActive(t, closest.feature)) return d - radius;
    const float along = Dot(closest.onSegment - point, t.normal);
    normal = along >= 0.0f ? t.normal : -t.normal;
    const float height = std::fabs(along);
    const float lateralSquared = std::max(0.0f, d * d - height * height);
    point = closest.onSegment - normal * height;
    return height - std::sqrt(std::max(0.0f, radius * radius - lateralSquared));
}

struct MeshContact {
    Vec3 normal;   // mesh space, from the mesh toward the other shape
    Vec3 point;    // mesh space
    float penetration;
};

void RoundedMeshContacts(const Shape& r, const Pose& pr, const TriangleMesh& mesh, const Pose& pm, std::vector<MeshContact>& out) {
    Vec3 w0, w1;
    CoreSegment(r, pr, w0, w1);
    const Vec3 p0 = pm.InverseTransformPoint(w0), p1 = pm.InverseTransformPoint(w1);
    AABB box;
    box.Expand(p0);
    box.Expand(p1);
    const bool capsule = DistanceSquared(p0, p1) > 1.0e-10f;
    mesh.Query(box.Inflated(r.radius), [&](std::size_t index) {
        const TriangleMesh::Triangle& t = mesh.GetTriangle(index);
        Vec3 normal, point;
        const float distance = RoundedTriangle(p0, p1, r.radius, t, normal, point);
        if (distance >= 0.0f) return true;
        out.push_back({normal, point, -distance});
        // A capsule lying on a face also rests on its ends (no rolling on one point).
        if (capsule) {
            for (const Vec3& end : {p0, p1}) {
                Vec3 endNormal, endPoint;
                const float endDistance = RoundedTriangle(end, end, r.radius, t, endNormal, endPoint);
                if (endDistance < 0.0f && Dot(endNormal, normal) > 0.9f) out.push_back({endNormal, endPoint, -endDistance});
            }
        }
        return true;
    });
}

// The triangle feature furthest along `direction`: a vertex, an edge or the face.
int SupportFeature(const Vec3 (&v)[3], Vec3 direction) {
    const float d[3] = {Dot(v[0], direction), Dot(v[1], direction), Dot(v[2], direction)};
    const float top = std::max(d[0], std::max(d[1], d[2]));
    const float scale = std::max({Length(v[1] - v[0]), Length(v[2] - v[1]), Length(v[0] - v[2])});
    const float tolerance = 1.0e-4f * std::max(1.0f, scale);
    const bool at[3] = {d[0] >= top - tolerance, d[1] >= top - tolerance, d[2] >= top - tolerance};
    if (at[0] && at[1] && at[2]) return kFace;
    if (at[0] && at[1]) return kEdgeAB;
    if (at[1] && at[2]) return kEdgeBC;
    if (at[2] && at[0]) return kEdgeCA;
    return at[0] ? kVertexA : (at[1] ? kVertexB : kVertexC);
}

// Box against one triangle: 13-axis SAT, preferring the triangle's face (a box
// sliding over a triangulated floor must not snag on seams), then clipping.
// Box and edge axes are only used where the triangle's penetrating feature is
// an active edge or vertex; internal seams defer to the face normal.
void BoxTriangleContacts(Vec3 center, const Vec3 (&axis)[3], const float (&extent)[3], const TriangleMesh::Triangle& t,
    std::vector<MeshContact>& out) {
    const Vec3 v[3] = {t.a, t.b, t.c};
    // Overlap along `l`; direction is the way to push the box out.
    auto test = [&](Vec3 l, float& penetration, Vec3& direction) {
        const float c = Dot(center, l);
        const float r = extent[0] * std::fabs(Dot(axis[0], l)) + extent[1] * std::fabs(Dot(axis[1], l))
            + extent[2] * std::fabs(Dot(axis[2], l));
        const float t0 = Dot(v[0], l), t1 = Dot(v[1], l), t2 = Dot(v[2], l);
        const float pushPositive = std::max(t0, std::max(t1, t2)) - (c - r);
        const float pushNegative = (c + r) - std::min(t0, std::min(t1, t2));
        if (pushPositive <= 0.0f || pushNegative <= 0.0f) return false;
        penetration = std::min(pushPositive, pushNegative);
        direction = pushPositive <= pushNegative ? l : -l;
        return true;
    };
    float facePenetration;
    Vec3 faceDirection;
    if (!test(t.normal, facePenetration, faceDirection)) return;
    float boxPenetration = std::numeric_limits<float>::max();
    Vec3 boxDirection;
    int boxAxis = 0;
    for (int k = 0; k < 3; ++k) {
        float penetration;
        Vec3 direction;
        if (!test(axis[k], penetration, direction)) return;
        if (penetration < boxPenetration) {
            boxPenetration = penetration;
            boxDirection = direction;
            boxAxis = k;
        }
    }
    float edgePenetration = std::numeric_limits<float>::max();
    Vec3 edgeDirection;
    int edgeBox = -1, edgeTriangle = -1;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            Vec3 l = Cross(axis[i], v[(j + 1) % 3] - v[j]);
            const float length = Length(l);
            if (length < 1.0e-4f) continue;
            l = l / length;
            float penetration;
            Vec3 direction;
            if (!test(l, penetration, direction)) return;
            // Internal edges are covered by their neighbours' faces.
            if ((t.activeEdges & (1u << j)) && penetration < edgePenetration) {
                edgePenetration = penetration;
                edgeDirection = direction;
                edgeBox = i;
                edgeTriangle = j;
            }
        }
    }
    constexpr float kRelative = 0.95f, kAbsolute = 0.005f;
    const bool useBox = boxPenetration < kRelative * facePenetration - kAbsolute
        && FeatureActive(t, SupportFeature(v, boxDirection));
    const float reference = useBox ? boxPenetration : facePenetration;
    if (edgeBox >= 0 && edgePenetration < kRelative * reference - kAbsolute) {
        Vec3 edgeCenter = center;
        for (int k = 0; k < 3; ++k) {
            if (k != edgeBox) edgeCenter += axis[k] * (extent[k] * (Dot(axis[k], edgeDirection) > 0.0f ? -1.0f : 1.0f));
        }
        const Vec3 b0 = edgeCenter - axis[edgeBox] * extent[edgeBox], b1 = edgeCenter + axis[edgeBox] * extent[edgeBox];
        float s, w;
        Vec3 c1, c2;
        ClosestPointsSegmentSegment(b0, b1, v[edgeTriangle], v[(edgeTriangle + 1) % 3], s, w, c1, c2);
        out.push_back({edgeDirection, (c1 + c2) * 0.5f, edgePenetration});
        return;
    }
    const std::size_t before = out.size();
    Vec3 polygon[16], scratch[16];
    int count = 0;
    if (!useBox) {
        // Reference: the triangle. Incident: the box face most against its normal.
        const Vec3 n = faceDirection;
        int k = 0;
        for (int i = 1; i < 3; ++i)
            if (std::fabs(Dot(axis[i], n)) > std::fabs(Dot(axis[k], n))) k = i;
        const Vec3 faceCenter = center + axis[k] * (extent[k] * (Dot(axis[k], n) > 0.0f ? -1.0f : 1.0f));
        const Vec3 du = axis[(k + 1) % 3] * extent[(k + 1) % 3], dv = axis[(k + 2) % 3] * extent[(k + 2) % 3];
        polygon[0] = faceCenter + du + dv;
        polygon[1] = faceCenter - du + dv;
        polygon[2] = faceCenter - du - dv;
        polygon[3] = faceCenter + du - dv;
        count = 4;
        for (int e = 0; e < 3 && count > 0; ++e) {
            const Vec3 p = v[e], q = v[(e + 1) % 3], opposite = v[(e + 2) % 3];
            Vec3 side = Normalize(Cross(q - p, t.normal));
            if (Dot(side, opposite - p) > 0.0f) side = -side;
            count = ClipPolygon(polygon, count, side, Dot(side, p), scratch);
            for (int i = 0; i < count; ++i) polygon[i] = scratch[i];
        }
        for (int i = 0; i < count; ++i) {
            const float depth = Dot(t.a - polygon[i], n);
            if (depth >= -1.0e-4f) out.push_back({n, polygon[i] + n * (depth * 0.5f), std::max(0.0f, depth)});
        }
    } else {
        // Reference: the box face toward the triangle. Incident: the triangle.
        const Vec3 outward = -boxDirection;
        const Vec3 faceCenter = center + outward * extent[boxAxis];
        polygon[0] = v[0];
        polygon[1] = v[1];
        polygon[2] = v[2];
        count = 3;
        for (int a = 1; a <= 2 && count > 0; ++a) {
            const int k = (boxAxis + a) % 3;
            for (float sign : {1.0f, -1.0f}) {
                const Vec3 side = axis[k] * sign;
                count = ClipPolygon(polygon, count, side, Dot(side, center) + extent[k], scratch);
                for (int i = 0; i < count; ++i) polygon[i] = scratch[i];
                if (count == 0) break;
            }
        }
        for (int i = 0; i < count; ++i) {
            const float depth = Dot(faceCenter - polygon[i], outward);
            if (depth >= -1.0e-4f) out.push_back({boxDirection, polygon[i] + outward * (depth * 0.5f), std::max(0.0f, depth)});
        }
    }
    if (out.size() == before) {
        const Vec3 direction = useBox ? boxDirection : faceDirection;
        out.push_back({direction, ClosestPointOnTriangle(center, t.a, t.b, t.c), reference});
    }
}

void BoxMeshContacts(const Shape& box, const Pose& pb, const TriangleMesh& mesh, const Pose& pm, std::vector<MeshContact>& out) {
    const Vec3 center = pm.InverseTransformPoint(pb.position);
    const Vec3 axis[3] = {pm.InverseTransformVector(pb.TransformVector({1, 0, 0})),
        pm.InverseTransformVector(pb.TransformVector({0, 1, 0})), pm.InverseTransformVector(pb.TransformVector({0, 0, 1}))};
    const float extent[3] = {box.halfExtents.x, box.halfExtents.y, box.halfExtents.z};
    const Vec3 reach = Abs(axis[0]) * extent[0] + Abs(axis[1]) * extent[1] + Abs(axis[2]) * extent[2];
    mesh.Query(AABB::FromCenterExtents(center, reach), [&](std::size_t index) {
        BoxTriangleContacts(center, axis, extent, mesh.GetTriangle(index), out);
        return true;
    });
}

// Groups per-triangle contacts into manifolds by normal (deepest first) and
// reduces each to four points. Normals follow Collide's A-to-B convention.
int BuildManifolds(std::vector<MeshContact>& contacts, const Pose& meshPose, bool meshIsA, Manifold* out, int capacity) {
    if (contacts.empty() || capacity <= 0) return 0;
    std::stable_sort(contacts.begin(), contacts.end(),
        [](const MeshContact& x, const MeshContact& y) { return x.penetration > y.penetration; });
    struct Cluster {
        Vec3 normal;
        std::vector<ContactPoint> points;
    };
    std::vector<Cluster> clusters;
    for (const MeshContact& contact : contacts) {
        int best = -1;
        float bestDot = -2.0f;
        for (std::size_t i = 0; i < clusters.size(); ++i) {
            const float d = Dot(clusters[i].normal, contact.normal);
            if (d > bestDot) {
                bestDot = d;
                best = static_cast<int>(i);
            }
        }
        if (best < 0 || (bestDot < 0.999f && static_cast<int>(clusters.size()) < capacity)) {
            clusters.push_back({contact.normal, {}});
            best = static_cast<int>(clusters.size()) - 1;
        }
        clusters[static_cast<std::size_t>(best)].points.push_back({meshPose.TransformPoint(contact.point), contact.penetration});
    }
    int produced = 0;
    for (const Cluster& cluster : clusters) {
        Manifold& m = out[produced];
        m = {};
        const Vec3 normal = meshPose.TransformVector(cluster.normal);
        m.normal = meshIsA ? normal : -normal;
        ReduceContacts(cluster.points.data(), static_cast<int>(cluster.points.size()), normal, m);
        if (m.count > 0) ++produced;
    }
    return produced;
}

float AabbGap(const AABB& a, const AABB& b) {
    const Vec3 gap = Max(Max(a.min - b.max, b.min - a.max), Vec3{});
    return Length(gap);
}

float RoundedMeshDistance(const Shape& a, const Pose& pa, const TriangleMesh& mesh, const Pose& pm, Vec3& normalFromB, Vec3& pointOnB) {
    Vec3 w0, w1;
    CoreSegment(a, pa, w0, w1);
    const Vec3 p0 = pm.InverseTransformPoint(w0), p1 = pm.InverseTransformPoint(w1);
    AABB core;
    core.Expand(p0);
    core.Expand(p1);
    float best = std::numeric_limits<float>::max();
    Vec3 bestNormal{0, 1, 0}, bestPoint = mesh.Bounds().Center();
    const std::vector<TriangleMesh::Node>& nodes = mesh.Nodes();
    std::uint32_t stack[64];
    int top = 0;
    stack[top++] = 0;
    while (top > 0) {
        const std::uint32_t index = stack[--top];
        const TriangleMesh::Node& node = nodes[index];
        const float gap = AabbGap(node.bounds, core);
        // Touching triangles can report face-plane depths below gap - radius.
        if (gap >= a.radius && gap - a.radius >= best) continue;
        if (node.count == 0) {
            const float left = AabbGap(nodes[index + 1].bounds, core), right = AabbGap(nodes[node.first].bounds, core);
            // Visit the nearer child first.
            if (left <= right) {
                stack[top++] = node.first;
                stack[top++] = index + 1;
            } else {
                stack[top++] = index + 1;
                stack[top++] = node.first;
            }
            continue;
        }
        for (std::uint32_t i = node.first; i < node.first + node.count; ++i) {
            Vec3 normal, point;
            const float distance = RoundedTriangle(p0, p1, a.radius, mesh.GetTriangle(i), normal, point);
            if (distance < best) {
                best = distance;
                bestNormal = normal;
                bestPoint = point;
            }
        }
    }
    normalFromB = pm.TransformVector(bestNormal);
    pointOnB = pm.TransformPoint(bestPoint);
    return best;
}

bool ShapeCastMesh(const Shape& moving, const Pose& start, Vec3 motion, const TriangleMesh& mesh, const Pose& pm,
    ShapeCastResult& result, float tolerance) {
    Vec3 w0, w1;
    CoreSegment(moving, start, w0, w1);
    const Vec3 p0 = pm.InverseTransformPoint(w0), p1 = pm.InverseTransformPoint(w1);
    const Vec3 localMotion = pm.InverseTransformVector(motion);
    const Vec3 direction = Normalize(localMotion, {});
    AABB swept;
    swept.Expand(p0);
    swept.Expand(p1);
    swept.Expand(p0 + localMotion);
    swept.Expand(p1 + localMotion);
    bool found = false;
    ShapeCastResult best;
    mesh.Query(swept.Inflated(moving.radius + tolerance), [&](std::size_t index) {
        const TriangleMesh::Triangle& t = mesh.GetTriangle(index);
        int feature = kFace;
        // Exact triangle distance (a valid bound for the cast); the normal is
        // fixed up for internal features once the hit is known.
        auto distanceAt = [&](Vec3 offset, Vec3& normal, Vec3& point) {
            const Vec3 a = p0 + offset, b = p1 + offset;
            const SegmentTriangleResult closest = ClosestSegmentTriangle(a, b, t);
            point = closest.onTriangle;
            if (closest.crossing || closest.distance < 1.0e-6f) {
                feature = kFace;
                normal = t.normal;
                return std::min(std::min(Dot(a - t.a, t.normal), Dot(b - t.a, t.normal)), 0.0f) - moving.radius;
            }
            feature = closest.feature;
            normal = (closest.onSegment - closest.onTriangle) / closest.distance;
            return closest.distance - moving.radius;
        };
        ShapeCastResult hit;
        if (!ConservativeCast(distanceAt, localMotion, tolerance, hit)) return true;
        if (!FeatureActive(t, feature)) hit.normal = Dot(hit.normal, t.normal) >= 0.0f ? t.normal : -t.normal;
        // Earliest hit wins; ties prefer the most head-on surface.
        const bool better = !found || hit.fraction < best.fraction - 1.0e-6f
            || (hit.fraction <= best.fraction + 1.0e-6f && Dot(hit.normal, direction) < Dot(best.normal, direction));
        if (better) {
            best = hit;
            found = true;
        }
        return true;
    });
    if (!found) return false;
    result = best;
    result.normal = pm.TransformVector(best.normal);
    result.point = pm.TransformPoint(best.point);
    return true;
}

// ---------------------------------------------------------------- convex dispatch
bool CollideConvex(const Shape& a, const Pose& pa, const Shape& b, const Pose& pb, Manifold& m) {
    m.count = 0;
    if (IsRounded(a) && IsRounded(b)) return CollideRounded(a, pa, b, pb, m);
    if (IsRounded(a) && b.type == ShapeType::Box) {
        if (!CollideRoundedBox(a, pa, b, pb, m)) return false;
        m.normal = -m.normal; // box->rounded becomes A(rounded)->B(box)
        return true;
    }
    if (a.type == ShapeType::Box && IsRounded(b)) return CollideRoundedBox(b, pb, a, pa, m);
    return CollideBoxes(a, pa, b, pb, m);
}

} // namespace

void ClosestSegmentBox(Vec3 a, Vec3 b, Vec3 e, float& t, Vec3& pointOnSegment, Vec3& pointOnBox) {
    // dist(P(t), box) is convex in t: golden-section search, then check ends.
    auto distanceAt = [&](float s) {
        const Vec3 p = a + (b - a) * s;
        return LengthSquared(p - Clamp(p, -e, e));
    };
    if (LengthSquared(b - a) < 1.0e-12f) {
        t = 0.0f;
    } else {
        constexpr float kGolden = 0.6180339887f;
        float lo = 0.0f, hi = 1.0f;
        float x1 = hi - kGolden * (hi - lo), x2 = lo + kGolden * (hi - lo);
        float f1 = distanceAt(x1), f2 = distanceAt(x2);
        for (int i = 0; i < 40; ++i) {
            if (f1 < f2) {
                hi = x2;
                x2 = x1;
                f2 = f1;
                x1 = hi - kGolden * (hi - lo);
                f1 = distanceAt(x1);
            } else {
                lo = x1;
                x1 = x2;
                f1 = f2;
                x2 = lo + kGolden * (hi - lo);
                f2 = distanceAt(x2);
            }
        }
        t = 0.5f * (lo + hi);
        // Plateaus (segment inside/parallel) keep the most central zero; also try ends.
        float bestT = t, bestD = distanceAt(t);
        for (float candidate : {0.0f, 1.0f}) {
            const float d = distanceAt(candidate);
            if (d < bestD - 1.0e-9f) {
                bestD = d;
                bestT = candidate;
            }
        }
        t = bestT;
    }
    pointOnSegment = a + (b - a) * t;
    pointOnBox = Clamp(pointOnSegment, -e, e);
}

int CollideAll(const Shape& a, const Pose& pa, const Shape& b, const Pose& pb, Manifold* manifolds, int capacity) {
    if (!manifolds || capacity <= 0) return 0;
    const bool meshA = a.type == ShapeType::Mesh, meshB = b.type == ShapeType::Mesh;
    if (!meshA && !meshB) {
        manifolds[0] = {};
        return CollideConvex(a, pa, b, pb, manifolds[0]) && manifolds[0].count > 0 ? 1 : 0;
    }
    if (meshA && meshB) return 0; // meshes are static geometry; they never collide with each other
    const Shape& mesh = meshA ? a : b;
    const Shape& other = meshA ? b : a;
    const Pose& meshPose = meshA ? pa : pb;
    const Pose& otherPose = meshA ? pb : pa;
    if (!mesh.mesh) return 0;
    std::vector<MeshContact> contacts;
    if (IsRounded(other)) RoundedMeshContacts(other, otherPose, *mesh.mesh, meshPose, contacts);
    else BoxMeshContacts(other, otherPose, *mesh.mesh, meshPose, contacts);
    return BuildManifolds(contacts, meshPose, meshA, manifolds, capacity);
}

bool Collide(const Shape& a, const Pose& pa, const Shape& b, const Pose& pb, Manifold& m) {
    m = {};
    if (a.type != ShapeType::Mesh && b.type != ShapeType::Mesh) return CollideConvex(a, pa, b, pb, m);
    Manifold all[kMaxManifolds];
    const int count = CollideAll(a, pa, b, pb, all, kMaxManifolds);
    if (count == 0) return false;
    int deepest = 0;
    float depth = -1.0f;
    for (int i = 0; i < count; ++i) {
        for (int p = 0; p < all[i].count; ++p) {
            if (all[i].points[p].penetration > depth) {
                depth = all[i].points[p].penetration;
                deepest = i;
            }
        }
    }
    m = all[deepest];
    return true;
}

bool RaycastShape(const Shape& shape, const Pose& pose, const Ray& ray, float maxDistance, float& t, Vec3& normal) {
    switch (shape.type) {
    case ShapeType::Sphere:
        if (!IntersectRaySphere(ray, pose.position, shape.radius, maxDistance, t)) return false;
        normal = Normalize(ray.At(t) - pose.position, -ray.direction);
        return true;
    case ShapeType::Box: {
        const Ray local{pose.InverseTransformPoint(ray.origin), pose.InverseTransformVector(ray.direction)};
        Vec3 localNormal;
        if (!IntersectRayAABB(local, {-shape.halfExtents, shape.halfExtents}, maxDistance, t, &localNormal)) return false;
        normal = LengthSquared(localNormal) > 0.0f ? pose.TransformVector(localNormal) : -ray.direction;
        return true;
    }
    case ShapeType::Capsule: {
        const Vec3 o = pose.InverseTransformPoint(ray.origin);
        const Vec3 d = pose.InverseTransformVector(ray.direction);
        const float r = shape.radius, h = shape.halfHeight;
        const Vec3 core0{0, -h, 0}, core1{0, h, 0};
        if (DistanceSquared(o, ClosestPointOnSegment(o, core0, core1)) <= r * r) {
            t = 0.0f;
            normal = -ray.direction;
            return true;
        }
        float best = maxDistance;
        bool hit = false;
        Vec3 bestNormal;
        const float a = d.x * d.x + d.z * d.z;
        if (a > 1.0e-10f) {
            const float bq = o.x * d.x + o.z * d.z;
            const float c = o.x * o.x + o.z * o.z - r * r;
            const float disc = bq * bq - a * c;
            if (disc >= 0.0f) {
                const float tc = (-bq - std::sqrt(disc)) / a;
                const float y = o.y + d.y * tc;
                if (tc >= 0.0f && tc <= best && y >= -h && y <= h) {
                    best = tc;
                    hit = true;
                    const Vec3 p = o + d * tc;
                    bestNormal = Normalize(Vec3{p.x, 0.0f, p.z});
                }
            }
        }
        for (const Vec3& center : {core0, core1}) {
            float ts;
            if (IntersectRaySphere({o, d}, center, r, best, ts) && ts <= best) {
                best = ts;
                hit = true;
                bestNormal = Normalize(o + d * ts - center, {0, 1, 0});
            }
        }
        if (!hit) return false;
        t = best;
        normal = pose.TransformVector(bestNormal);
        return true;
    }
    case ShapeType::Mesh: {
        if (!shape.mesh) return false;
        const Ray local{pose.InverseTransformPoint(ray.origin), pose.InverseTransformVector(ray.direction)};
        Vec3 localNormal;
        if (!shape.mesh->Raycast(local, maxDistance, t, localNormal)) return false;
        normal = pose.TransformVector(localNormal);
        return true;
    }
    }
    return false;
}

float RoundedDistance(const Shape& a, const Pose& pa, const Shape& b, const Pose& pb, Vec3& normalFromB, Vec3& pointOnB) {
    Vec3 a0, a1;
    CoreSegment(a, pa, a0, a1);
    if (IsRounded(b)) {
        Vec3 b0, b1;
        CoreSegment(b, pb, b0, b1);
        float s, t;
        Vec3 c1, c2;
        const float d = std::sqrt(ClosestPointsSegmentSegment(a0, a1, b0, b1, s, t, c1, c2));
        normalFromB = d > 1.0e-6f ? (c1 - c2) / d : Normalize(pa.position - pb.position, {0, 1, 0});
        pointOnB = c2 + normalFromB * b.radius;
        return d - a.radius - b.radius;
    }
    if (b.type == ShapeType::Mesh) {
        if (!b.mesh) {
            normalFromB = {0, 1, 0};
            pointOnB = pb.position;
            return std::numeric_limits<float>::max();
        }
        return RoundedMeshDistance(a, pa, *b.mesh, pb, normalFromB, pointOnB);
    }
    const Vec3 la = pb.InverseTransformPoint(a0), lb = pb.InverseTransformPoint(a1);
    float t;
    Vec3 onSegment, onBox;
    ClosestSegmentBox(la, lb, b.halfExtents, t, onSegment, onBox);
    const Vec3 delta = onSegment - onBox;
    const float d = Length(delta);
    if (d > 1.0e-6f) {
        normalFromB = pb.TransformVector(delta / d);
        pointOnB = pb.TransformPoint(onBox);
        return d - a.radius;
    }
    Manifold m;
    CollideRoundedBox(a, pa, b, pb, m);
    normalFromB = m.normal;
    pointOnB = m.count > 0 ? m.points[0].position : pb.position;
    return -(m.count > 0 ? m.points[0].penetration : a.radius);
}

int RoundedProximities(const Shape& a, const Pose& pa, const Shape& b, const Pose& pb, float maxDistance,
    Proximity* out, int capacity) {
    if (!out || capacity <= 0 || !IsRounded(a)) return 0;
    if (b.type != ShapeType::Mesh) {
        Proximity p;
        p.distance = RoundedDistance(a, pa, b, pb, p.normalFromB, p.pointOnB);
        if (p.distance > maxDistance) return 0;
        out[0] = p;
        return 1;
    }
    if (!b.mesh) return 0;
    Vec3 w0, w1;
    CoreSegment(a, pa, w0, w1);
    const Vec3 p0 = pb.InverseTransformPoint(w0), p1 = pb.InverseTransformPoint(w1);
    AABB core;
    core.Expand(p0);
    core.Expand(p1);
    std::vector<Proximity> found;
    b.mesh->Query(core.Inflated(a.radius + std::max(0.0f, maxDistance)), [&](std::size_t index) {
        Vec3 normal, point;
        const float distance = RoundedTriangle(p0, p1, a.radius, b.mesh->GetTriangle(index), normal, point);
        if (distance <= maxDistance) found.push_back({distance, pb.TransformVector(normal), pb.TransformPoint(point)});
        return true;
    });
    std::stable_sort(found.begin(), found.end(), [](const Proximity& x, const Proximity& y) { return x.distance < y.distance; });
    const int count = std::min(capacity, static_cast<int>(found.size()));
    for (int i = 0; i < count; ++i) out[i] = found[static_cast<std::size_t>(i)];
    return count;
}

bool ShapeCast(const Shape& moving, const Pose& start, Vec3 motion, const Shape& target, const Pose& targetPose,
    ShapeCastResult& result, float tolerance) {
    result = {};
    if (!IsRounded(moving) || !IsFinite(motion)) return false;
    if (target.type == ShapeType::Mesh) {
        return target.mesh && ShapeCastMesh(moving, start, motion, *target.mesh, targetPose, result, tolerance);
    }
    auto distanceAt = [&](Vec3 offset, Vec3& normal, Vec3& point) {
        return RoundedDistance(moving, {start.position + offset, start.rotation}, target, targetPose, normal, point);
    };
    return ConservativeCast(distanceAt, motion, tolerance, result);
}

} // namespace Astral::Physics
