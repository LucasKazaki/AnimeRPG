#include "Engine/Math/Geometry.h"
#include "Engine/Math/VectorMath.h"
#include "Tests/EngineTestSupport.h"

using namespace Astral::Math;

namespace {
void CheckVec(Vec3 actual, Vec3 expected, float tolerance = 1.0e-4f) {
    ASTRAL_CHECK_NEAR(actual.x, expected.x, tolerance);
    ASTRAL_CHECK_NEAR(actual.y, expected.y, tolerance);
    ASTRAL_CHECK_NEAR(actual.z, expected.z, tolerance);
}
} // namespace

ASTRAL_TEST(VectorBasics) {
    const Vec3 a{1.0f, 2.0f, 3.0f};
    const Vec3 b{4.0f, -5.0f, 6.0f};
    CheckVec(a + b, {5.0f, -3.0f, 9.0f});
    CheckVec(a - b, {-3.0f, 7.0f, -3.0f});
    ASTRAL_CHECK_NEAR(Dot(a, b), 12.0f, 1e-6);
    CheckVec(Cross(Vec3{1, 0, 0}, Vec3{0, 1, 0}), {0, 0, 1});
    CheckVec(Normalize(Vec3{0, 0, 0}, Vec3{0, 1, 0}), {0, 1, 0});
    CheckVec(Normalize(Vec3{3, 0, 4}), {0.6f, 0.0f, 0.8f});
    ASTRAL_CHECK(!IsFinite(Vec3{0.0f, std::nanf(""), 0.0f}));
    CheckVec(Reflect(Vec3{1, -1, 0}, Vec3{0, 1, 0}), {1, 1, 0});
    CheckVec(MoveTowards(Vec3{}, Vec3{10, 0, 0}, 3.0f), {3, 0, 0});
    CheckVec(MoveTowards(Vec3{}, Vec3{1, 0, 0}, 3.0f), {1, 0, 0});
    ASTRAL_CHECK_NEAR(WrapAngle(kPi * 2.5f), kHalfPi, 1e-4);
    ASTRAL_CHECK_NEAR(WrapAngle(-kPi * 2.5f), -kHalfPi, 1e-4);
    ASTRAL_CHECK_NEAR(SmoothStep(0.0f, 1.0f, 0.5f), 0.5f, 1e-6);
}

ASTRAL_TEST(OrthonormalBasisIsOrthonormal) {
    const Vec3 normals[] = {{0, 1, 0}, {0, 0, -1}, Normalize(Vec3{1, 2, 3}), Normalize(Vec3{-1, -1, -1})};
    for (Vec3 n : normals) {
        Vec3 t, b;
        OrthonormalBasis(n, t, b);
        ASTRAL_CHECK_NEAR(Length(t), 1.0f, 1e-5);
        ASTRAL_CHECK_NEAR(Length(b), 1.0f, 1e-5);
        ASTRAL_CHECK_NEAR(Dot(t, n), 0.0f, 1e-5);
        ASTRAL_CHECK_NEAR(Dot(b, n), 0.0f, 1e-5);
        ASTRAL_CHECK_NEAR(Dot(t, b), 0.0f, 1e-5);
    }
}

ASTRAL_TEST(MatrixInverseAndComposition) {
    const Mat4 m = Translation({3, -2, 5}) * RotationY(0.7f) * RotationX(-0.3f) * Scaling({2, 3, 0.5f});
    Mat4 inverse{};
    ASTRAL_CHECK(Inverse(m, inverse));
    ASTRAL_CHECK(NearlyEqual(m * inverse, Mat4::Identity()));
    const Vec3 p{1.5f, -4.0f, 2.0f};
    CheckVec(TransformPoint(inverse, TransformPoint(m, p)), p);
    Mat4 singular{};
    ASTRAL_CHECK(!Inverse(singular, inverse));
    ASTRAL_CHECK(NearlyEqual(Transpose(Transpose(m)), m));
}

ASTRAL_TEST(RotationConventionsAgree) {
    // Positive yaw turns forward (+Z) toward right (+X) in both representations.
    CheckVec(TransformVector(RotationY(kHalfPi), {0, 0, 1}), {1, 0, 0});
    CheckVec(Rotate(QuatFromAxisAngle({0, 1, 0}, kHalfPi), {0, 0, 1}), {1, 0, 0});
    for (float angle : {0.3f, -1.2f, 2.5f}) {
        ASTRAL_CHECK(NearlyEqual(ToMat4(QuatFromAxisAngle({1, 0, 0}, angle)), RotationX(angle)));
        ASTRAL_CHECK(NearlyEqual(ToMat4(QuatFromAxisAngle({0, 1, 0}, angle)), RotationY(angle)));
        ASTRAL_CHECK(NearlyEqual(ToMat4(QuatFromAxisAngle({0, 0, 1}, angle)), RotationZ(angle)));
    }
    ASTRAL_CHECK_NEAR(YawFromDirection({1, 0, 0}), kHalfPi, 1e-6);
    CheckVec(Rotate(QuatFromYaw(YawFromDirection(Normalize(Vec3{1, 0, 1}))), {0, 0, 1}),
        Normalize(Vec3{1, 0, 1}));
}

ASTRAL_TEST(QuaternionOperations) {
    const Quat a = QuatFromEuler(0.4f, -0.2f, 0.9f);
    const Quat b = QuatFromEuler(-1.1f, 0.5f, 0.1f);
    const Vec3 v{0.3f, -2.0f, 1.0f};
    CheckVec(Rotate(a * b, v), Rotate(a, Rotate(b, v)));
    CheckVec(Rotate(Inverse(a), Rotate(a, v)), v);
    const Quat halfway = Slerp(Quat{}, QuatFromAxisAngle({0, 1, 0}, 1.0f), 0.5f);
    CheckVec(Rotate(halfway, {0, 0, 1}), Rotate(QuatFromAxisAngle({0, 1, 0}, 0.5f), {0, 0, 1}));
    const Vec3 from = Normalize(Vec3{1, 2, -1});
    const Vec3 to = Normalize(Vec3{-3, 0.5f, 2});
    CheckVec(Rotate(QuatFromTo(from, to), from), to);
    CheckVec(Rotate(QuatFromTo(Vec3{0, 1, 0}, Vec3{0, -1, 0}), Vec3{0, 1, 0}), {0, -1, 0});
    // Slerp endpoints and shortest path.
    const Quat negB{-b.x, -b.y, -b.z, -b.w};
    CheckVec(Rotate(Slerp(a, negB, 1.0f), v), Rotate(b, v));
}

ASTRAL_TEST(TrsCombineMatchesMatrices) {
    const TRS parent{{1, 2, 3}, QuatFromEuler(0.5f, 0.1f, -0.2f), {2, 2, 2}};
    const TRS child{{-1, 0.5f, 4}, QuatFromEuler(-0.3f, 0.7f, 0.0f), {1, 1, 1}};
    const TRS combined = Combine(parent, child);
    ASTRAL_CHECK(NearlyEqual(ToMat4(combined), ToMat4(parent) * ToMat4(child), 1e-4f));
    const Vec3 p{0.2f, 0.3f, -0.4f};
    CheckVec(TransformPoint(combined, p), TransformPoint(ToMat4(parent) * ToMat4(child), p));
}

ASTRAL_TEST(CameraMatrices) {
    const Mat4 view = LookAtLH({0, 5, -10}, {0, 5, 0}, {0, 1, 0});
    CheckVec(TransformPoint(view, {0, 5, 0}), {0, 0, 10});
    CheckVec(TransformPoint(view, {1, 6, -10}), {1, 1, 0});
    const Mat4 projection = PerspectiveLH(Radians(60.0f), 16.0f / 9.0f, 0.5f, 200.0f);
    const Vec4 nearPoint = projection * Vec4{0, 0, 0.5f, 1};
    const Vec4 farPoint = projection * Vec4{0, 0, 200.0f, 1};
    ASTRAL_CHECK_NEAR(nearPoint.z / nearPoint.w, 0.0f, 1e-5);
    ASTRAL_CHECK_NEAR(farPoint.z / farPoint.w, 1.0f, 1e-5);
    const Mat4 ortho = OrthographicOffCenterLH(-2, 6, -1, 3, 0, 10);
    const Vec4 corner = ortho * Vec4{6, 3, 10, 1};
    ASTRAL_CHECK_NEAR(corner.x, 1.0f, 1e-5);
    ASTRAL_CHECK_NEAR(corner.y, 1.0f, 1e-5);
    ASTRAL_CHECK_NEAR(corner.z, 1.0f, 1e-5);
}

ASTRAL_TEST(FrustumCulling) {
    const Mat4 vp = PerspectiveLH(Radians(60.0f), 1.0f, 0.5f, 100.0f)
        * LookAtLH({0, 0, 0}, {0, 0, 1}, {0, 1, 0});
    const Frustum frustum = Frustum::FromViewProjection(vp);
    ASTRAL_CHECK(frustum.Intersects(AABB::FromCenterExtents({0, 0, 10}, {1, 1, 1})));
    ASTRAL_CHECK(!frustum.Intersects(AABB::FromCenterExtents({0, 0, -10}, {1, 1, 1})));
    ASTRAL_CHECK(!frustum.Intersects(AABB::FromCenterExtents({0, 0, 150}, {1, 1, 1})));
    ASTRAL_CHECK(!frustum.Intersects(AABB::FromCenterExtents({40, 0, 10}, {1, 1, 1})));
    ASTRAL_CHECK(frustum.Intersects(Sphere{{0, 0, 99.5f}, 1.0f}));
    ASTRAL_CHECK(!frustum.Intersects(AABB{}));
}

ASTRAL_TEST(RayQueries) {
    float t = 0.0f;
    Vec3 normal{};
    const AABB box = AABB::FromCenterExtents({0, 0, 5}, {1, 1, 1});
    ASTRAL_CHECK(IntersectRayAABB({{0, 0, 0}, {0, 0, 1}}, box, 100.0f, t, &normal));
    ASTRAL_CHECK_NEAR(t, 4.0f, 1e-5);
    CheckVec(normal, {0, 0, -1});
    ASTRAL_CHECK(!IntersectRayAABB({{0, 3, 0}, {0, 0, 1}}, box, 100.0f, t));
    ASTRAL_CHECK(!IntersectRayAABB({{0, 0, 0}, {0, 0, 1}}, box, 3.0f, t));
    ASTRAL_CHECK(IntersectRaySphere({{0, 0, 0}, {1, 0, 0}}, {5, 0, 0}, 1.0f, 100.0f, t));
    ASTRAL_CHECK_NEAR(t, 4.0f, 1e-5);
    float u = 0.0f, v = 0.0f;
    ASTRAL_CHECK(IntersectRayTriangle({{0.2f, 0.2f, -1}, {0, 0, 1}}, {0, 0, 0}, {1, 0, 0}, {0, 1, 0},
        10.0f, t, u, v));
    ASTRAL_CHECK_NEAR(t, 1.0f, 1e-5);
    ASTRAL_CHECK_NEAR(u, 0.2f, 1e-5);
    ASTRAL_CHECK_NEAR(v, 0.2f, 1e-5);
    ASTRAL_CHECK(IntersectRayPlane({{0, 5, 0}, {0, -1, 0}}, Plane::FromPointNormal({}, {0, 1, 0}), 10.0f, t));
    ASTRAL_CHECK_NEAR(t, 5.0f, 1e-5);
}

ASTRAL_TEST(ClosestPoints) {
    float s = 0.0f, t = 0.0f;
    Vec3 c1{}, c2{};
    const float distanceSquared = ClosestPointsSegmentSegment({-1, 0, 0}, {1, 0, 0},
        {0, 1, -1}, {0, 1, 1}, s, t, c1, c2);
    ASTRAL_CHECK_NEAR(distanceSquared, 1.0f, 1e-5);
    CheckVec(c1, {0, 0, 0});
    CheckVec(c2, {0, 1, 0});
    CheckVec(ClosestPointOnTriangle({0.25f, 5.0f, 0.25f}, {0, 0, 0}, {1, 0, 0}, {0, 0, 1}), {0.25f, 0, 0.25f});
    CheckVec(ClosestPointOnTriangle({-3, 0, -3}, {0, 0, 0}, {1, 0, 0}, {0, 0, 1}), {0, 0, 0});
    CheckVec(ClosestPointOnSegment({5, 5, 0}, {0, 0, 0}, {2, 0, 0}), {2, 0, 0});
    const AABB transformed = TransformAABB(RotationY(kHalfPi * 0.5f), AABB::FromCenterExtents({}, {1, 1, 1}));
    ASTRAL_CHECK_NEAR(transformed.max.x, std::sqrt(2.0f), 1e-4);
}

ASTRAL_TEST_MAIN("EngineMathTests")
