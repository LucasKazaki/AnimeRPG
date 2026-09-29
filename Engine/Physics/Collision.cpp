#include "Engine/Physics/Collision.h"

#include <algorithm>
#include <cmath>

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
    if (candidateCount == 0) return;
    // Keep at most four: deepest, farthest from it, then two maximising area.
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

bool Collide(const Shape& a, const Pose& pa, const Shape& b, const Pose& pb, Manifold& m) {
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

bool ShapeCast(const Shape& moving, const Pose& start, Vec3 motion, const Shape& target, const Pose& targetPose,
    ShapeCastResult& result, float tolerance) {
    result = {};
    if (!IsRounded(moving) || !IsFinite(motion)) return false;
    const float length = Length(motion);
    Pose pose = start;
    Vec3 normal, point;
    float distance = RoundedDistance(moving, pose, target, targetPose, normal, point);
    if (distance < -tolerance) {
        result.fraction = 0.0f;
        result.normal = normal;
        result.point = point;
        result.startPenetrating = true;
        return true;
    }
    if (length < 1.0e-9f) return false;
    const Vec3 direction = motion / length;
    // Already touching: only a motion into the surface is blocked. Sliding along
    // or leaving a resting contact (e.g. walking on the ground) is not a hit.
    if (distance <= tolerance && Dot(direction, normal) >= -1.0e-4f) return false;
    // The distance between convex sets is a convex function of translation, so
    // along the motion g(t) >= g(0) + g'(0) t with g'(0) = dot(direction, normal).
    // A non-negative slope means the shapes can never meet; otherwise stepping
    // to the tangent's root is always safe and converges monotonically (Newton
    // from below). This handles grazing and parallel motion in a few iterations.
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
        pose.position = start.position + direction * travelled;
        distance = RoundedDistance(moving, pose, target, targetPose, normal, point);
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

} // namespace Astral::Physics
