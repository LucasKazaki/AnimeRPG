#include "Engine/Core/Random.h"
#include "Engine/Math/Geometry.h"
#include "Engine/Physics/BroadPhase.h"
#include "Engine/Physics/CharacterController.h"
#include "Engine/Physics/Collision.h"
#include "Engine/Physics/Destruction.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/TriangleMesh.h"
#include "Tests/EngineTestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace Astral;
using namespace Astral::Physics;
using Math::Vec3;

namespace {
BodyId AddGround(PhysicsWorld& world, float friction = 0.8f) {
    BodyDesc ground;
    ground.type = BodyType::Static;
    ground.shape = Shape::Box({50.0f, 0.5f, 50.0f});
    ground.position = {0.0f, -0.5f, 0.0f};
    ground.friction = friction;
    return world.CreateBody(ground);
}
BodyId AddStaticBox(PhysicsWorld& world, Vec3 center, Vec3 half, Math::Quat rotation = {}) {
    BodyDesc d;
    d.type = BodyType::Static;
    d.shape = Shape::Box(half);
    d.position = center;
    d.rotation = rotation;
    return world.CreateBody(d);
}
void Simulate(PhysicsWorld& world, float seconds, float dt = 1.0f / 60.0f) {
    const int steps = static_cast<int>(seconds / dt + 0.5f);
    for (int i = 0; i < steps; ++i) world.Step(dt);
}

// Triangle soup builder. Every triangle gets its own three vertices, the way
// imported meshes split vertices at UV/normal seams, so welding is exercised.
struct MeshBuilder {
    std::vector<Vec3> vertices;
    std::vector<std::uint32_t> indices;

    // Adds a triangle whose front face points along `outward`.
    void Triangle(Vec3 a, Vec3 b, Vec3 c, Vec3 outward) {
        if (Math::Dot(Math::Cross(b - a, c - a), outward) < 0.0f) std::swap(b, c);
        const auto base = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);
        indices.push_back(base);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
    }
    // Corners in cyclic order.
    void Quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 outward) {
        Triangle(a, b, c, outward);
        Triangle(a, c, d, outward);
    }
    // Upward-facing height field over [x0, x0 + cells * size] x [z0, z0 + cells * size].
    template <typename Height>
    void Grid(float x0, float z0, int cellsX, int cellsZ, float size, Height height) {
        for (int i = 0; i < cellsX; ++i) {
            for (int j = 0; j < cellsZ; ++j) {
                const float x = x0 + static_cast<float>(i) * size, z = z0 + static_cast<float>(j) * size;
                const Vec3 a{x, height(x, z), z}, b{x + size, height(x + size, z), z};
                const Vec3 c{x + size, height(x + size, z + size), z + size}, d{x, height(x, z + size), z + size};
                Quad(a, b, c, d, {0, 1, 0});
            }
        }
    }
    void Box(Vec3 center, Vec3 half) {
        for (int axis = 0; axis < 3; ++axis) {
            for (float sign : {-1.0f, 1.0f}) {
                Vec3 n{};
                Math::SetComponent(n, axis, sign);
                Vec3 u{}, v{};
                Math::SetComponent(u, (axis + 1) % 3, Math::Component(half, (axis + 1) % 3));
                Math::SetComponent(v, (axis + 2) % 3, Math::Component(half, (axis + 2) % 3));
                const Vec3 c = center + n * Math::Component(half, axis);
                Quad(c - u - v, c + u - v, c + u + v, c - u + v, n);
            }
        }
    }
    std::shared_ptr<const TriangleMesh> Build() const {
        std::string error;
        auto mesh = TriangleMesh::Create(vertices, indices, error);
        if (!mesh) std::fprintf(stderr, "mesh error: %s\n", error.c_str());
        ASTRAL_CHECK(mesh != nullptr);
        return mesh;
    }
};

float Flat(float, float) { return 0.0f; }

BodyId AddStaticMesh(PhysicsWorld& world, const MeshBuilder& builder, Vec3 position = {}, Math::Quat rotation = {}) {
    BodyDesc d;
    d.type = BodyType::Static;
    d.shape = Shape::Mesh(builder.Build());
    d.position = position;
    d.rotation = rotation;
    d.friction = 0.8f;
    const BodyId id = world.CreateBody(d);
    ASTRAL_CHECK(!id.IsNull());
    return id;
}

int ActiveEdgeBits(const TriangleMesh& mesh) {
    int bits = 0;
    for (std::size_t i = 0; i < mesh.TriangleCount(); ++i) {
        const std::uint8_t flags = mesh.GetTriangle(i).activeEdges;
        bits += (flags & 1) + ((flags >> 1) & 1) + ((flags >> 2) & 1);
    }
    return bits;
}
} // namespace

ASTRAL_TEST(NarrowphasePairsReportConsistentNormals) {
    Manifold m;
    // Sphere-sphere overlapping along +x: normal from A to B.
    ASTRAL_CHECK(Collide(Shape::Sphere(1.0f), {{0, 0, 0}, {}}, Shape::Sphere(1.0f), {{1.5f, 0, 0}, {}}, m));
    ASTRAL_CHECK_NEAR(m.normal.x, 1.0f, 1e-5);
    ASTRAL_CHECK_NEAR(m.points[0].penetration, 0.5f, 1e-5);
    ASTRAL_CHECK(!Collide(Shape::Sphere(1.0f), {{0, 0, 0}, {}}, Shape::Sphere(1.0f), {{2.1f, 0, 0}, {}}, m));
    // Sphere resting on a box: normal from box (A) up to the sphere (B).
    ASTRAL_CHECK(Collide(Shape::Box({2, 0.5f, 2}), {{0, 0, 0}, {}}, Shape::Sphere(0.5f), {{0.3f, 0.95f, 0}, {}}, m));
    ASTRAL_CHECK_NEAR(m.normal.y, 1.0f, 1e-4);
    ASTRAL_CHECK_NEAR(m.points[0].penetration, 0.05f, 1e-4);
    // Same pair reversed flips the normal.
    ASTRAL_CHECK(Collide(Shape::Sphere(0.5f), {{0.3f, 0.95f, 0}, {}}, Shape::Box({2, 0.5f, 2}), {{0, 0, 0}, {}}, m));
    ASTRAL_CHECK_NEAR(m.normal.y, -1.0f, 1e-4);
    // Sphere centre inside the box: pushed out through the nearest face (+y).
    ASTRAL_CHECK(Collide(Shape::Box({2, 0.5f, 2}), {{0, 0, 0}, {}}, Shape::Sphere(0.25f), {{0, 0.4f, 0}, {}}, m));
    ASTRAL_CHECK(m.normal.y > 0.99f);
    ASTRAL_CHECK_NEAR(m.points[0].penetration, 0.35f, 1e-3);
    // Capsule lying on a box gets two contacts (no rolling on a single point).
    const Math::Quat lying = Math::QuatFromAxisAngle({0, 0, 1}, Math::kHalfPi);
    ASTRAL_CHECK(Collide(Shape::Box({3, 0.5f, 3}), {{0, 0, 0}, {}}, Shape::CapsuleFromHeight(0.3f, 2.0f), {{0, 0.78f, 0}, lying}, m));
    ASTRAL_CHECK(m.count >= 2);
    ASTRAL_CHECK(m.normal.y > 0.99f);
    // Box resting on a box: face contact with four points.
    ASTRAL_CHECK(Collide(Shape::Box({3, 0.5f, 3}), {{0, 0, 0}, {}}, Shape::Box({0.5f, 0.5f, 0.5f}), {{0.2f, 0.98f, -0.1f}, Math::QuatFromYaw(0.3f)}, m));
    ASTRAL_CHECK(m.count == 4);
    ASTRAL_CHECK(m.normal.y > 0.99f);
    for (int i = 0; i < m.count; ++i) ASTRAL_CHECK_NEAR(m.points[i].penetration, 0.02f, 2e-3);
    // Separated boxes.
    ASTRAL_CHECK(!Collide(Shape::Box({1, 1, 1}), {{0, 0, 0}, {}}, Shape::Box({1, 1, 1}), {{2.5f, 0, 0}, Math::QuatFromYaw(0.7f)}, m));
    // Edge-edge: two boxes rotated 45 degrees about perpendicular axes meeting edge to edge.
    const Math::Quat edgeA = Math::QuatFromAxisAngle({0, 0, 1}, Math::kPi * 0.25f);
    const Math::Quat edgeB = Math::QuatFromAxisAngle({1, 0, 0}, Math::kPi * 0.25f);
    ASTRAL_CHECK(Collide(Shape::Box({1, 1, 1}), {{0, 0, 0}, edgeA}, Shape::Box({1, 1, 1}), {{0, 2.78f, 0}, edgeB}, m));
    ASTRAL_CHECK(m.count == 1);
    ASTRAL_CHECK(m.normal.y > 0.99f);
    ASTRAL_CHECK_NEAR(m.points[0].penetration, 2.0f * std::sqrt(2.0f) - 2.78f, 5e-3);
    // Capsule-capsule crossing.
    ASTRAL_CHECK(Collide(Shape::CapsuleFromHeight(0.5f, 3.0f), {{0, 0, 0}, {}}, Shape::CapsuleFromHeight(0.5f, 3.0f),
        {{0.9f, 0, 0}, Math::QuatFromAxisAngle({1, 0, 0}, Math::kHalfPi)}, m));
    ASTRAL_CHECK_NEAR(m.normal.x, 1.0f, 1e-4);
    ASTRAL_CHECK_NEAR(m.points[0].penetration, 0.1f, 1e-4);
}

ASTRAL_TEST(ShapeRaycastsAndCasts) {
    float t;
    Vec3 n;
    const Math::Ray ray{{-10, 0, 0}, {1, 0, 0}};
    ASTRAL_CHECK(RaycastShape(Shape::Box({1, 1, 1}), {{0, 0, 0}, Math::QuatFromYaw(Math::kPi * 0.25f)}, ray, 100.0f, t, n));
    ASTRAL_CHECK_NEAR(t, 10.0f - std::sqrt(2.0f), 1e-4);
    ASTRAL_CHECK(n.x < -0.6f);
    ASTRAL_CHECK(RaycastShape(Shape::CapsuleFromHeight(0.5f, 3.0f), {{0, 0, 0}, {}}, {{0, 10, 0}, {0, -1, 0}}, 100.0f, t, n));
    ASTRAL_CHECK_NEAR(t, 8.5f, 1e-4);
    ASTRAL_CHECK_NEAR(n.y, 1.0f, 1e-4);
    ASTRAL_CHECK(RaycastShape(Shape::CapsuleFromHeight(0.5f, 3.0f), {{0, 0, 0}, {}}, ray, 100.0f, t, n));
    ASTRAL_CHECK_NEAR(t, 9.5f, 1e-4);
    ASTRAL_CHECK(!RaycastShape(Shape::Sphere(1.0f), {{0, 5, 0}, {}}, ray, 100.0f, t, n));

    ShapeCastResult r;
    ASTRAL_CHECK(ShapeCast(Shape::Sphere(0.5f), {{-5, 0, 0}, {}}, {10, 0, 0}, Shape::Box({1, 1, 1}), {{0, 0, 0}, {}}, r));
    ASTRAL_CHECK_NEAR(r.fraction, 0.35f, 2e-3);
    ASTRAL_CHECK(r.normal.x < -0.99f);
    // Grazing miss: passes 0.1 above the box top.
    ASTRAL_CHECK(!ShapeCast(Shape::Sphere(0.5f), {{-5, 1.6f, 0}, {}}, {10, 0, 0}, Shape::Box({1, 1, 1}), {{0, 0, 0}, {}}, r));
    // Start penetrating reports fraction 0.
    ASTRAL_CHECK(ShapeCast(Shape::CapsuleFromHeight(0.4f, 1.8f), {{0.5f, 0, 0}, {}}, {1, 0, 0}, Shape::Box({1, 1, 1}), {{0, 0, 0}, {}}, r));
    ASTRAL_CHECK(r.startPenetrating && r.fraction == 0.0f);
}

ASTRAL_TEST(BroadphaseTreeMatchesBruteForce) {
    DynamicAabbTree tree;
    Core::Random random(99);
    std::vector<int> proxies;
    std::vector<Math::AABB> boxes;
    for (int i = 0; i < 400; ++i) {
        const Vec3 c{random.Range(-50, 50), random.Range(-5, 5), random.Range(-50, 50)};
        const Math::AABB box = Math::AABB::FromCenterExtents(c, {random.Range(0.2f, 2), random.Range(0.2f, 2), random.Range(0.2f, 2)});
        boxes.push_back(box);
        proxies.push_back(tree.CreateProxy(box, static_cast<std::uint32_t>(i), 0.0f));
    }
    ASTRAL_CHECK(tree.Validate());
    ASTRAL_CHECK(tree.Height() < 20);
    for (int round = 0; round < 200; ++round) {
        const std::size_t i = random.NextBounded(static_cast<std::uint32_t>(proxies.size()));
        const Vec3 d{random.Range(-3, 3), 0, random.Range(-3, 3)};
        boxes[i] = {boxes[i].min + d, boxes[i].max + d};
        tree.MoveProxy(proxies[i], boxes[i], d, 0.0f);
    }
    for (int i = 0; i < 100; i += 3) {
        tree.DestroyProxy(proxies[static_cast<std::size_t>(i)]);
        proxies[static_cast<std::size_t>(i)] = -1;
    }
    ASTRAL_CHECK(tree.Validate());
    for (int query = 0; query < 50; ++query) {
        const Math::AABB q = Math::AABB::FromCenterExtents({random.Range(-50, 50), 0, random.Range(-50, 50)}, {5, 5, 5});
        std::set<std::uint32_t> found;
        tree.Query(q, [&](int proxy) {
            found.insert(tree.UserData(proxy));
            return true;
        });
        for (std::size_t i = 0; i < boxes.size(); ++i) {
            if (proxies[i] < 0) continue;
            const bool overlaps = boxes[i].Overlaps(q);
            if (overlaps) ASTRAL_CHECK(found.count(static_cast<std::uint32_t>(i)) == 1);
        }
    }
}

ASTRAL_TEST(FallingSphereRestsAndSleeps) {
    PhysicsWorld world;
    AddGround(world);
    BodyDesc ball;
    ball.shape = Shape::Sphere(0.5f);
    ball.position = {0, 5, 0};
    ball.restitution = 0.0f;
    const BodyId id = world.CreateBody(ball);
    Simulate(world, 4.0f);
    const Body* body = world.GetBody(id);
    ASTRAL_CHECK_NEAR(body->pose.position.y, 0.5f, 0.03f);
    ASTRAL_CHECK(!body->awake);
    // Waking and nudging works.
    world.ApplyImpulse(id, {2, 0, 0}, body->pose.position);
    ASTRAL_CHECK(world.GetBody(id)->awake);
}

ASTRAL_TEST(BoxStackStaysStable) {
    PhysicsWorld world;
    AddGround(world);
    std::vector<BodyId> boxes;
    for (int i = 0; i < 6; ++i) {
        BodyDesc d;
        d.shape = Shape::Box({0.5f, 0.5f, 0.5f});
        d.position = {0.0f, 0.5f + static_cast<float>(i) * 1.0f, 0.0f};
        d.mass = 2.0f;
        boxes.push_back(world.CreateBody(d));
    }
    Simulate(world, 6.0f);
    for (int i = 0; i < 6; ++i) {
        const Body* b = world.GetBody(boxes[static_cast<std::size_t>(i)]);
        ASTRAL_CHECK_NEAR(b->pose.position.x, 0.0f, 0.05f);
        ASTRAL_CHECK_NEAR(b->pose.position.z, 0.0f, 0.05f);
        ASTRAL_CHECK_NEAR(b->pose.position.y, 0.5f + static_cast<float>(i), 0.06f);
    }
}

ASTRAL_TEST(RestitutionAndFrictionBehave) {
    // Bouncy ball rebounds, dead ball does not.
    for (float e : {0.0f, 0.8f}) {
        PhysicsWorld world;
        world.Settings().allowSleep = false;
        AddGround(world);
        BodyDesc d;
        d.shape = Shape::Sphere(0.25f);
        d.position = {0, 3, 0};
        d.restitution = e;
        d.linearDamping = 0.0f;
        const BodyId id = world.CreateBody(d);
        float maxAfterBounce = 0.0f;
        bool bounced = false;
        for (int i = 0; i < 180; ++i) {
            world.Step(1.0f / 120.0f);
            const Body* b = world.GetBody(id);
            if (b->linearVelocity.y > 0.5f) bounced = true;
            if (bounced) maxAfterBounce = std::max(maxAfterBounce, b->pose.position.y);
        }
        if (e > 0.5f) ASTRAL_CHECK(maxAfterBounce > 1.2f);
        else ASTRAL_CHECK(maxAfterBounce < 0.4f);
    }
    // A box on a 20 degree ramp: sticks with high friction, slides with low friction.
    for (float friction : {1.0f, 0.05f}) {
        PhysicsWorld world;
        const Math::Quat tilt = Math::QuatFromAxisAngle({0, 0, 1}, Math::Radians(20.0f));
        BodyDesc ramp;
        ramp.type = BodyType::Static;
        ramp.shape = Shape::Box({10, 0.5f, 5});
        ramp.rotation = tilt;
        ramp.friction = friction;
        world.CreateBody(ramp);
        BodyDesc d;
        d.shape = Shape::Box({0.4f, 0.4f, 0.4f});
        d.rotation = tilt;
        d.position = Math::Rotate(tilt, {0, 0.9f, 0});
        d.friction = friction;
        const BodyId id = world.CreateBody(d);
        const Vec3 start = world.GetBody(id)->pose.position;
        Simulate(world, 2.0f);
        const float moved = Math::Distance(world.GetBody(id)->pose.position, start);
        if (friction > 0.5f) ASTRAL_CHECK(moved < 0.1f);
        else ASTRAL_CHECK(moved > 1.0f);
    }
}

ASTRAL_TEST(SimulationIsDeterministic) {
    auto run = [] {
        PhysicsWorld world;
        AddGround(world);
        Core::Random random(5);
        std::vector<BodyId> ids;
        for (int i = 0; i < 20; ++i) {
            BodyDesc d;
            d.shape = (i % 3 == 0) ? Shape::Sphere(0.3f) : (i % 3 == 1 ? Shape::Box({0.3f, 0.3f, 0.3f}) : Shape::CapsuleFromHeight(0.2f, 1.0f));
            d.position = {random.Range(-1, 1), 1.0f + static_cast<float>(i) * 0.7f, random.Range(-1, 1)};
            d.rotation = Math::QuatFromEuler(random.Range(0, 3), random.Range(0, 3), 0);
            ids.push_back(world.CreateBody(d));
        }
        Simulate(world, 3.0f);
        std::vector<float> state;
        for (BodyId id : ids) {
            const Body* b = world.GetBody(id);
            state.push_back(b->pose.position.x);
            state.push_back(b->pose.position.y);
            state.push_back(b->pose.rotation.w);
        }
        return state;
    };
    const auto first = run();
    const auto second = run();
    ASTRAL_CHECK(first == second);
    for (std::size_t i = 1; i < first.size(); i += 3) ASTRAL_CHECK(first[i] > 0.0f && first[i] < 15.0f);
}

ASTRAL_TEST(TriggersAndQueries) {
    PhysicsWorld world;
    AddGround(world);
    BodyDesc zone;
    zone.type = BodyType::Static;
    zone.isTrigger = true;
    zone.shape = Shape::Box({1, 1, 1});
    zone.position = {0, 1, 5};
    zone.userData = 42;
    const BodyId trigger = world.CreateBody(zone);
    BodyDesc mover;
    mover.type = BodyType::Kinematic;
    mover.shape = Shape::Sphere(0.3f);
    mover.position = {0, 1, 0};
    const BodyId kinematic = world.CreateBody(mover);
    int begins = 0, ends = 0;
    for (int i = 0; i < 120; ++i) {
        const Body* k = world.GetBody(kinematic);
        world.MoveKinematic(kinematic, {k->pose.position + Vec3{0, 0, 0.1f}, {}}, 1.0f / 60.0f);
        world.Step(1.0f / 60.0f);
        for (const ContactEvent& e : world.Events()) {
            if (!e.trigger) continue;
            ASTRAL_CHECK(e.a == trigger || e.b == trigger);
            begins += e.type == ContactEventType::Begin;
            ends += e.type == ContactEventType::End;
        }
    }
    ASTRAL_CHECK(begins == 1 && ends == 1);
    // Ray ignores triggers by default, hits the ground.
    RaycastHit hit;
    ASTRAL_CHECK(world.Raycast({{0, 5, 5}, {0, -1, 0}}, 20.0f, hit));
    ASTRAL_CHECK_NEAR(hit.distance, 5.0f, 1e-4);
    ASTRAL_CHECK(world.Raycast({{0, 5, 5}, {0, -1, 0}}, 20.0f, hit, 0xFFFFFFFFu, {}, true));
    ASTRAL_CHECK(hit.userData == 42);
    std::vector<BodyId> overlaps;
    world.Overlap(Shape::Sphere(0.5f), {{0, 1, 5.8f}, {}}, overlaps);
    ASTRAL_CHECK(overlaps.size() == 1 && overlaps[0] == trigger);
    // Layer masks filter.
    ASTRAL_CHECK(!world.Raycast({{0, 5, 5}, {0, -1, 0}}, 20.0f, hit, 0x2u));
    ASTRAL_CHECK(world.CreateBody(BodyDesc{BodyType::Dynamic, Shape::Sphere(-1.0f)}).IsNull());
    ASTRAL_CHECK(world.DestroyBody(trigger));
    ASTRAL_CHECK(!world.DestroyBody(trigger));
}

ASTRAL_TEST(KinematicBodiesFollowFastMovesExactly) {
    // A kinematic body reaches its MoveKinematic target in one step however
    // far it is (teleported characters, fast platforms); the dynamic-body
    // speed limit must not make it lag behind. Arriving inside a trigger
    // reports the overlap on the next step.
    PhysicsWorld world;
    BodyDesc zone;
    zone.type = BodyType::Static;
    zone.isTrigger = true;
    zone.shape = Shape::Sphere(0.5f);
    zone.position = {0, 1, 20};
    const BodyId trigger = world.CreateBody(zone);
    BodyDesc mover;
    mover.type = BodyType::Kinematic;
    mover.shape = Shape::Sphere(0.3f);
    mover.position = {0, 1, 0};
    const BodyId kinematic = world.CreateBody(mover);
    const float dt = 1.0f / 60.0f; // 20 m per step is 1200 m/s, ten times the dynamic limit
    world.MoveKinematic(kinematic, {{0, 1, 20}, {}}, dt);
    world.Step(dt);
    ASTRAL_CHECK(Math::Length(world.GetBody(kinematic)->pose.position - Vec3{0, 1, 20}) < 1.0e-3f);
    world.MoveKinematic(kinematic, {{0, 1, 20}, {}}, dt);
    world.Step(dt);
    bool entered = false;
    for (const ContactEvent& e : world.Events()) entered |= e.trigger && e.type == ContactEventType::Begin && (e.a == trigger || e.b == trigger);
    ASTRAL_CHECK(entered);
    // Dynamic bodies stay limited.
    BodyDesc ball;
    ball.shape = Shape::Sphere(0.2f);
    ball.position = {5, 50, 0};
    ball.linearVelocity = {1000, 0, 0};
    const BodyId fast = world.CreateBody(ball);
    world.Step(dt);
    ASTRAL_CHECK(Math::Length(world.GetBody(fast)->linearVelocity) <= world.Settings().maxLinearSpeed + 1.0e-3f);
}

ASTRAL_TEST(CharacterWalksBlocksStepsAndJumps) {
    PhysicsWorld world;
    AddGround(world);
    AddStaticBox(world, {0, 1.5f, 6}, {5, 1.5f, 0.5f});        // wall at z = 5.5
    AddStaticBox(world, {10, 0.15f, 0}, {5, 0.15f, 3});         // 0.3 m kerb from x = 5 to 15
    AddStaticBox(world, {-10, 0.5f, 0}, {5, 0.5f, 3});          // 1.0 m ledge from x = -15 to -5
    CharacterSettings settings;
    CharacterController character(world, settings, {0, 0, 0});
    const float dt = 1.0f / 60.0f;
    for (int i = 0; i < 180; ++i) character.Move({0, 0, 5}, false, dt);
    ASTRAL_CHECK(character.Grounded());
    ASTRAL_CHECK(character.FootPosition().z < 5.5f - settings.radius + 0.01f);
    ASTRAL_CHECK(character.FootPosition().z > 5.5f - settings.radius - 0.1f);
    ASTRAL_CHECK_NEAR(character.FootPosition().y, 0.0f, 0.03f);
    // Step up the kerb.
    character.Teleport({3, 0, 0});
    for (int i = 0; i < 90; ++i) character.Move({5, 0, 0}, false, dt);
    ASTRAL_CHECK(character.FootPosition().x > 5.5f);
    ASTRAL_CHECK_NEAR(character.FootPosition().y, 0.3f, 0.05f);
    // The 1 m ledge blocks walking...
    character.Teleport({-3, 0, 0});
    for (int i = 0; i < 90; ++i) character.Move({-5, 0, 0}, false, dt);
    ASTRAL_CHECK(character.FootPosition().x > -5.0f);
    // ...but a jump clears it.
    bool jumped = false;
    for (int i = 0; i < 120; ++i) {
        character.Move({-5, 0, 0}, !jumped, dt);
        jumped = true;
    }
    ASTRAL_CHECK(character.FootPosition().x < -5.2f);
    ASTRAL_CHECK_NEAR(character.FootPosition().y, 1.0f, 0.05f);
    ASTRAL_CHECK(character.Grounded());
}

ASTRAL_TEST(CharacterSlopesDashAndDepenetration) {
    PhysicsWorld world;
    AddGround(world);
    // Gentle 25 degree ramp rising toward +z, steep 70 degree ramp toward -z.
    const Math::Quat gentle = Math::QuatFromAxisAngle({1, 0, 0}, -Math::Radians(25.0f));
    AddStaticBox(world, Math::Rotate(gentle, {0, -0.5f, 6}) + Vec3{0, 0, 2}, {2, 0.5f, 6}, gentle);
    const Math::Quat steep = Math::QuatFromAxisAngle({1, 0, 0}, Math::Radians(70.0f));
    AddStaticBox(world, {10, 0, -4}, {2, 0.5f, 6}, steep);
    CharacterSettings settings;
    CharacterController character(world, settings, {0, 0, 0});
    const float dt = 1.0f / 60.0f;
    for (int i = 0; i < 150; ++i) character.Move({0, 0, 4}, false, dt);
    ASTRAL_CHECK(character.FootPosition().y > 1.5f); // climbed the gentle ramp
    character.Teleport({10, 0, 2});
    for (int i = 0; i < 120; ++i) character.Move({0, 0, -4}, false, dt);
    ASTRAL_CHECK(character.FootPosition().y < 0.6f); // cannot walk up 70 degrees
    // Dash stops before a wall instead of passing through it.
    AddStaticBox(world, {-10, 1, 0}, {0.5f, 1, 3});
    character.Teleport({-5, 0, 0});
    const Vec3 reached = character.SweepTo({-15, 0, 0});
    ASTRAL_CHECK(reached.x > -9.5f + settings.radius - 0.05f);
    ASTRAL_CHECK(reached.x < -9.0f);
    // Unobstructed dash reaches its target.
    const Vec3 free = character.SweepTo({-5, 0, 6});
    ASTRAL_CHECK_NEAR(free.z, 6.0f, 1e-3);
    // Spawned inside a pillar: pushed out.
    AddStaticBox(world, {20, 1, 0}, {0.5f, 1, 0.5f});
    CharacterController stuck(world, settings, {20.2f, 0, 0});
    ASTRAL_CHECK(stuck.FootPosition().x > 20.5f + settings.radius - 0.05f || stuck.FootPosition().x < 19.5f - settings.radius + 0.05f
        || std::fabs(stuck.FootPosition().z) > 0.5f + settings.radius - 0.05f);
}

ASTRAL_TEST(DestructibleBreaksAndResets) {
    const auto pieces = FractureBox({1.0f, 2.0f, 0.5f}, 3, 4, 2, 7);
    ASTRAL_CHECK(pieces.size() == 24);
    float volume = 0.0f;
    for (const FracturePiece& p : pieces) {
        ASTRAL_CHECK(p.halfExtents.x > 0.05f && p.halfExtents.y > 0.05f && p.halfExtents.z > 0.05f);
        volume += 8.0f * p.halfExtents.x * p.halfExtents.y * p.halfExtents.z;
    }
    ASTRAL_CHECK_NEAR(volume, 8.0f, 1e-3);
    ASTRAL_CHECK(FractureBox({1, 1, 1}, 2, 2, 2, 3).size() == 8);
    ASTRAL_CHECK(FractureBox({-1, 1, 1}, 2, 2, 2, 3).empty());

    PhysicsWorld world;
    AddGround(world);
    DestructibleSettings settings;
    settings.health = 50.0f;
    Destructible pillar(world, {{0, 2, 0}, {}}, {0.5f, 2.0f, 0.5f}, settings);
    const std::size_t baseline = world.BodyCount();
    ASTRAL_CHECK(!pillar.ApplyDamage(20.0f, {0, 2, -0.5f}, {0, 0, 1}));
    ASTRAL_CHECK(!pillar.Broken() && pillar.Health() == 30.0f);
    ASTRAL_CHECK(!pillar.ApplyDamage(std::nanf(""), {0, 2, -0.5f}, {0, 0, 1}));
    ASTRAL_CHECK(pillar.ApplyDamage(40.0f, {0, 2, -0.5f}, {0, 0, 1}));
    ASTRAL_CHECK(pillar.Broken());
    ASTRAL_CHECK(pillar.IntactBody().IsNull());
    ASTRAL_CHECK(pillar.Debris().size() == 18);
    Simulate(world, 2.0f);
    float averageZ = 0.0f;
    for (BodyId id : pillar.Debris()) averageZ += world.GetBody(id)->pose.position.z;
    ASTRAL_CHECK(averageZ / 18.0f > 0.3f); // pushed along the hit direction
    pillar.Reset();
    ASTRAL_CHECK(!pillar.Broken() && pillar.Debris().empty() && world.BodyCount() == baseline);
    pillar.ApplyDamage(100.0f, {0, 2, 0}, {1, 0, 0});
    pillar.Update(settings.debrisLifetime + 0.1f);
    ASTRAL_CHECK(pillar.Debris().empty());
}

ASTRAL_TEST(TriangleMeshBuildsBvhAndFlagsInternalEdges) {
    MeshBuilder floor;
    floor.Grid(-10.0f, -10.0f, 20, 20, 1.0f, Flat);
    const auto mesh = floor.Build();
    ASTRAL_CHECK(mesh->TriangleCount() == 800);
    ASTRAL_CHECK(mesh->Depth() <= 10);
    ASTRAL_CHECK_NEAR(mesh->Bounds().min.x, -10.0f, 1e-6);
    ASTRAL_CHECK_NEAR(mesh->Bounds().max.z, 10.0f, 1e-6);
    for (std::size_t i = 0; i < mesh->TriangleCount(); ++i) ASTRAL_CHECK(mesh->GetTriangle(i).normal.y > 0.999f);
    // Welded split vertices: only the 80 rim edges stay active; seams are internal.
    ASTRAL_CHECK(ActiveEdgeBits(*mesh) == 80);

    // A convex ridge keeps its shared edge; a concave valley does not.
    MeshBuilder roof, valley;
    roof.Quad({-1, 0, 0}, {0, 1, 0}, {0, 1, 1}, {-1, 0, 1}, {-1, 1, 0});
    roof.Quad({0, 1, 0}, {1, 0, 0}, {1, 0, 1}, {0, 1, 1}, {1, 1, 0});
    valley.Quad({-1, 1, 0}, {0, 0, 0}, {0, 0, 1}, {-1, 1, 1}, {1, 1, 0});
    valley.Quad({0, 0, 0}, {1, 1, 0}, {1, 1, 1}, {0, 0, 1}, {-1, 1, 0});
    ASTRAL_CHECK(ActiveEdgeBits(*roof.Build()) == 8);
    ASTRAL_CHECK(ActiveEdgeBits(*valley.Build()) == 6);

    std::string error;
    ASTRAL_CHECK(!TriangleMesh::Create(std::vector<Vec3>{{0, 0, 0}, {1, 0, 0}}, {0, 1}, error) && !error.empty());
    ASTRAL_CHECK(!TriangleMesh::Create(std::vector<Vec3>{{0, 0, 0}, {1, 0, 0}, {0, 0, 1}}, {0, 1, 3}, error));
    ASTRAL_CHECK(!TriangleMesh::Create(std::vector<Vec3>{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}}, {0, 1, 2}, error));
    ASTRAL_CHECK(!TriangleMesh::Create(std::vector<Vec3>{{0, 0, 0}, {std::nanf(""), 0, 0}, {0, 0, 1}}, {0, 1, 2}, error));
    ASTRAL_CHECK(!TriangleMesh::Create(std::vector<Vec3>{{0, 0, 0}, {1, 0, 0}, {0, 0, 1}}, {0, 0, 1}, error));

    // BVH ray casts and box queries agree with brute force on a bumpy terrain.
    MeshBuilder terrain;
    terrain.Grid(-12.0f, -12.0f, 24, 24, 1.0f, [](float x, float z) { return 0.5f * std::sin(0.7f * x) + 0.3f * std::cos(1.3f * z); });
    const auto bumpy = terrain.Build();
    Core::Random random(99);
    for (int i = 0; i < 400; ++i) {
        const Vec3 origin{random.Range(-13.0f, 13.0f), random.Range(-3.0f, 3.0f), random.Range(-13.0f, 13.0f)};
        const Vec3 direction = Math::Normalize(Vec3{random.Range(-1.0f, 1.0f), random.Range(-1.0f, 1.0f), random.Range(-1.0f, 1.0f)}, {0, -1, 0});
        const Math::Ray ray{origin, direction};
        float bestT = 30.0f;
        bool bruteHit = false;
        for (std::size_t t = 0; t < bumpy->TriangleCount(); ++t) {
            const TriangleMesh::Triangle& tri = bumpy->GetTriangle(t);
            float hit, u, v;
            if (Math::IntersectRayTriangle(ray, tri.a, tri.b, tri.c, bestT, hit, u, v)) {
                bestT = hit;
                bruteHit = true;
            }
        }
        float t = 0.0f;
        Vec3 normal;
        const bool bvhHit = bumpy->Raycast(ray, 30.0f, t, normal);
        ASTRAL_CHECK(bvhHit == bruteHit);
        if (bvhHit) {
            ASTRAL_CHECK_NEAR(t, bestT, 1e-5);
            ASTRAL_CHECK(Math::Dot(normal, direction) <= 0.0f); // faces the ray
        }
        const Math::AABB box = Math::AABB::FromCenterExtents(origin, {random.Range(0.1f, 3.0f), 2.0f, random.Range(0.1f, 3.0f)});
        std::size_t queried = 0, brute = 0;
        bumpy->Query(box, [&](std::size_t) {
            ++queried;
            return true;
        });
        for (std::size_t k = 0; k < bumpy->TriangleCount(); ++k) {
            const TriangleMesh::Triangle& tri = bumpy->GetTriangle(k);
            Math::AABB bounds;
            bounds.Expand(tri.a);
            bounds.Expand(tri.b);
            bounds.Expand(tri.c);
            brute += bounds.Overlaps(box) ? 1u : 0u;
        }
        ASTRAL_CHECK(queried == brute);
    }
}

ASTRAL_TEST(MeshFloorSupportsBodiesWithoutCatchingOnSeams) {
    PhysicsWorld world;
    MeshBuilder floor;
    floor.Grid(-20.0f, -20.0f, 40, 40, 1.0f, Flat);
    AddStaticMesh(world, floor);
    BodyDesc dynamicMesh;
    dynamicMesh.shape = Shape::Mesh(floor.Build());
    ASTRAL_CHECK(world.CreateBody(dynamicMesh).IsNull()); // meshes are static or kinematic only
    ASTRAL_CHECK(!Shape::Mesh(nullptr).IsValid());

    BodyDesc ball;
    ball.shape = Shape::Sphere(0.5f);
    ball.position = {0.3f, 3.0f, 0.7f};
    const BodyId sphere = world.CreateBody(ball);
    BodyDesc crate;
    crate.shape = Shape::Box({0.5f, 0.5f, 0.5f});
    crate.position = {5.25f, 2.0f, 5.6f};
    crate.rotation = Math::QuatFromAxisAngle({0, 1, 0}, 0.4f);
    crate.friction = 0.1f;
    const BodyId box = world.CreateBody(crate);
    BodyDesc log;
    log.shape = Shape::CapsuleFromHeight(0.3f, 2.0f);
    log.position = {-5.4f, 1.5f, -5.3f};
    log.rotation = Math::QuatFromAxisAngle({0, 0, 1}, Math::kHalfPi);
    const BodyId capsule = world.CreateBody(log);
    Simulate(world, 3.0f);
    ASTRAL_CHECK_NEAR(world.GetBody(sphere)->pose.position.y, 0.5f, 0.03f);
    ASTRAL_CHECK_NEAR(world.GetBody(box)->pose.position.y, 0.5f, 0.03f);
    ASTRAL_CHECK(Math::Rotate(world.GetBody(box)->pose.rotation, {0, 1, 0}).y > 0.99f);
    ASTRAL_CHECK_NEAR(world.GetBody(capsule)->pose.position.y, 0.3f, 0.03f);
    ASTRAL_CHECK(std::fabs(Math::Rotate(world.GetBody(capsule)->pose.rotation, {0, 1, 0}).y) < 0.05f); // still lying

    // Rolling and sliding across dozens of triangle seams: no bumps, no tipping.
    world.SetLinearVelocity(sphere, {4.0f, 0.0f, 1.3f});
    world.SetLinearVelocity(box, {5.0f, 0.0f, -0.7f});
    const float sphereStart = world.GetBody(sphere)->pose.position.x;
    const float boxStart = world.GetBody(box)->pose.position.x;
    float maxSphereVy = 0.0f, maxBoxVy = 0.0f, minBoxUp = 1.0f;
    for (int i = 0; i < 120; ++i) {
        world.Step(1.0f / 60.0f);
        maxSphereVy = std::max(maxSphereVy, std::fabs(world.GetBody(sphere)->linearVelocity.y));
        maxBoxVy = std::max(maxBoxVy, std::fabs(world.GetBody(box)->linearVelocity.y));
        minBoxUp = std::min(minBoxUp, Math::Rotate(world.GetBody(box)->pose.rotation, {0, 1, 0}).y);
    }
    ASTRAL_CHECK(world.GetBody(sphere)->pose.position.x > sphereStart + 3.0f);
    ASTRAL_CHECK(world.GetBody(box)->pose.position.x > boxStart + 2.0f);
    ASTRAL_CHECK(maxSphereVy < 0.1f);
    ASTRAL_CHECK(maxBoxVy < 0.2f);
    ASTRAL_CHECK(minBoxUp > 0.98f);
    ASTRAL_CHECK_NEAR(world.GetBody(sphere)->pose.position.y, 0.5f, 0.03f);
    ASTRAL_CHECK_NEAR(world.GetBody(box)->pose.position.y, 0.5f, 0.03f);

    // A ball dropped into a V-shaped mesh trough settles at the bottom crease.
    PhysicsWorld trough;
    MeshBuilder v;
    v.Quad({-3, 3, -3}, {0, 0, -3}, {0, 0, 3}, {-3, 3, 3}, {1, 1, 0});
    v.Quad({0, 0, -3}, {3, 3, -3}, {3, 3, 3}, {0, 0, 3}, {-1, 1, 0});
    AddStaticMesh(trough, v);
    ball.position = {0.9f, 3.0f, 0.2f};
    const BodyId rolling = trough.CreateBody(ball);
    Simulate(trough, 5.0f);
    const Vec3 rest = trough.GetBody(rolling)->pose.position;
    ASTRAL_CHECK(std::fabs(rest.x) < 0.05f);
    ASTRAL_CHECK_NEAR(rest.y, 0.5f * std::sqrt(2.0f), 0.03f); // touching both 45 degree faces
}

ASTRAL_TEST(MeshCubeMatchesPrimitiveBox) {
    const Vec3 half{1.0f, 0.5f, 1.5f};
    MeshBuilder cube;
    cube.Box({}, half);
    const Shape mesh = Shape::Mesh(cube.Build());
    const Shape box = Shape::Box(half);
    ASTRAL_CHECK(ActiveEdgeBits(*mesh.mesh) == 24); // 12 convex cube edges, both sides
    const Pose pose{{2.0f, 1.0f, -3.0f}, Math::QuatFromAxisAngle(Math::Normalize(Vec3{1, 2, 0.5f}), 0.7f)};
    const Math::AABB meshBounds = mesh.WorldBounds(pose), boxBounds = box.WorldBounds(pose);
    ASTRAL_CHECK_NEAR(meshBounds.min.x, boxBounds.min.x, 1e-4);
    ASTRAL_CHECK_NEAR(meshBounds.max.y, boxBounds.max.y, 1e-4);

    Core::Random random(7);
    int rayMismatches = 0, castMismatches = 0;
    for (int i = 0; i < 400; ++i) {
        const Vec3 origin = pose.position + Math::Normalize(Vec3{random.Range(-1.0f, 1.0f), random.Range(-1.0f, 1.0f), random.Range(-1.0f, 1.0f)}, {1, 0, 0}) * 6.0f;
        const Vec3 target = pose.position + Vec3{random.Range(-1.5f, 1.5f), random.Range(-1.5f, 1.5f), random.Range(-1.5f, 1.5f)};
        const Math::Ray ray{origin, Math::Normalize(target - origin)};
        float tMesh = 0.0f, tBox = 0.0f;
        Vec3 nMesh, nBox;
        const bool hitMesh = RaycastShape(mesh, pose, ray, 20.0f, tMesh, nMesh);
        const bool hitBox = RaycastShape(box, pose, ray, 20.0f, tBox, nBox);
        if (hitMesh != hitBox) {
            ++rayMismatches;
        } else if (hitMesh) {
            ASTRAL_CHECK_NEAR(tMesh, tBox, 1e-3);
            if (Math::Dot(nMesh, nBox) < 0.999f) ++rayMismatches; // exact edge grazes only
        }
        // Sphere casts: same time of impact and normal against the mesh cube and the box.
        const Shape probe = Shape::Sphere(0.3f);
        ShapeCastResult castMesh, castBox;
        const Vec3 motion = (target - origin) * 1.5f;
        const bool sweptMesh = ShapeCast(probe, {origin, {}}, motion, mesh, pose, castMesh);
        const bool sweptBox = ShapeCast(probe, {origin, {}}, motion, box, pose, castBox);
        if (sweptMesh != sweptBox) {
            ++castMismatches;
        } else if (sweptMesh) {
            ASTRAL_CHECK_NEAR(castMesh.fraction, castBox.fraction, 2e-3);
            ASTRAL_CHECK(Math::Dot(castMesh.normal, castBox.normal) > 0.98f);
        }
        // Distances from outside agree exactly (the cube's edges are all active).
        const Vec3 outside = pose.position + Vec3{random.Range(-3.0f, 3.0f), random.Range(-3.0f, 3.0f), random.Range(-3.0f, 3.0f)};
        Vec3 normalMesh, pointMesh, normalBox, pointBox;
        const float dBox = RoundedDistance(probe, {outside, {}}, box, pose, normalBox, pointBox);
        if (dBox > 0.05f) {
            const float dMesh = RoundedDistance(probe, {outside, {}}, mesh, pose, normalMesh, pointMesh);
            ASTRAL_CHECK_NEAR(dMesh, dBox, 1e-3);
            ASTRAL_CHECK(Math::Dot(normalMesh, normalBox) > 0.999f);
        }
    }
    ASTRAL_CHECK(rayMismatches <= 2);
    ASTRAL_CHECK(castMismatches <= 2);

    // Resting contacts: same normal and depth against a face.
    Manifold fromMesh, fromBox;
    const Vec3 top = pose.TransformPoint({0.2f, 0.5f + 0.25f, 0.3f});
    ASTRAL_CHECK(Collide(Shape::Sphere(0.3f), {top, {}}, mesh, pose, fromMesh));
    ASTRAL_CHECK(Collide(Shape::Sphere(0.3f), {top, {}}, box, pose, fromBox));
    ASTRAL_CHECK(Math::Dot(fromMesh.normal, fromBox.normal) > 0.999f);
    ASTRAL_CHECK_NEAR(fromMesh.points[0].penetration, fromBox.points[0].penetration, 1e-4);
    // Box resting on the mesh cube's top face gets a four-point manifold.
    const Pose onTop{pose.TransformPoint({0.0f, 0.5f + 0.24f, 0.0f}), pose.rotation};
    ASTRAL_CHECK(Collide(Shape::Box({0.25f, 0.25f, 0.25f}), onTop, mesh, pose, fromMesh));
    ASTRAL_CHECK(fromMesh.count == 4);
    ASTRAL_CHECK(Math::Dot(fromMesh.normal, pose.TransformVector({0, -1, 0})) > 0.999f);
    for (int i = 0; i < 4; ++i) ASTRAL_CHECK_NEAR(fromMesh.points[i].penetration, 0.01f, 1e-3);
}

ASTRAL_TEST(CharacterWalksMeshTerrain) {
    PhysicsWorld world;
    const float rise = std::tan(Math::Radians(25.0f));
    MeshBuilder terrain;
    // Flat floor, a 25 degree ramp from z = 2 to 8, a plateau, and a wall at x = -6.
    terrain.Grid(-10.0f, -10.0f, 20, 12, 1.0f, Flat);
    terrain.Grid(-10.0f, 2.0f, 20, 6, 1.0f, [&](float, float z) { return (z - 2.0f) * rise; });
    terrain.Grid(-10.0f, 8.0f, 20, 4, 1.0f, [&](float, float) { return 6.0f * rise; });
    for (int j = 0; j < 12; ++j) {
        const float z = -10.0f + static_cast<float>(j);
        terrain.Quad({-6, 0, z}, {-6, 3, z}, {-6, 3, z + 1}, {-6, 0, z + 1}, {1, 0, 0});
    }
    AddStaticMesh(world, terrain);
    // A separate 70 degree slope rising toward -z.
    MeshBuilder cliff;
    cliff.Grid(20.0f, 0.0f, 10, 6, 1.0f, Flat);
    const float steep = std::tan(Math::Radians(70.0f));
    cliff.Grid(20.0f, -2.0f, 10, 2, 1.0f, [&](float, float z) { return -z * steep; });
    AddStaticMesh(world, cliff);

    CharacterSettings settings;
    const float dt = 1.0f / 60.0f;
    CharacterController character(world, settings, {0.0f, 0.0f, -5.0f});
    for (int i = 0; i < 5; ++i) character.Move({}, false, dt);
    ASTRAL_CHECK(character.Grounded());
    ASTRAL_CHECK_NEAR(character.FootPosition().y, 0.0f, 0.02f);
    // Walking across seams stays grounded at floor height, then stops at the mesh wall.
    for (int i = 0; i < 120; ++i) {
        character.Move({-4.0f, 0.0f, 0.0f}, false, dt);
        ASTRAL_CHECK(character.Grounded());
        ASTRAL_CHECK_NEAR(character.FootPosition().y, 0.0f, 0.02f);
    }
    ASTRAL_CHECK(character.FootPosition().x > -6.0f + settings.radius - 0.02f);
    ASTRAL_CHECK(character.FootPosition().x < -6.0f + settings.radius + 0.1f);
    // Pressing diagonally into the wall slides along it.
    const float zBefore = character.FootPosition().z;
    for (int i = 0; i < 30; ++i) character.Move({-3.0f, 0.0f, -3.0f}, false, dt);
    ASTRAL_CHECK(character.FootPosition().z < zBefore - 1.0f);
    ASTRAL_CHECK(character.FootPosition().x > -6.0f + settings.radius - 0.02f);

    // Climbs the mesh ramp.
    character.Teleport({0.0f, 0.0f, -2.0f});
    for (int i = 0; i < 150; ++i) character.Move({0.0f, 0.0f, 4.0f}, false, dt);
    ASTRAL_CHECK(character.FootPosition().y > 1.5f);
    ASTRAL_CHECK(character.Grounded());
    // Cannot walk up the 70 degree mesh slope.
    character.Teleport({25.0f, 0.0f, 3.0f});
    for (int i = 0; i < 120; ++i) character.Move({0.0f, 0.0f, -4.0f}, false, dt);
    ASTRAL_CHECK(character.FootPosition().y < 0.6f);
    // Dash stops at the mesh wall.
    character.Teleport({0.0f, 0.0f, -5.0f});
    const Vec3 reached = character.SweepTo({-12.0f, 0.0f, -5.0f});
    ASTRAL_CHECK(reached.x > -6.0f + settings.radius - 0.02f);
    ASTRAL_CHECK(reached.x < -6.0f + settings.radius + 0.1f);
    // Spawned half inside the wall: pushed back out to the front.
    CharacterController stuck(world, settings, {-6.0f + 0.1f, 0.0f, -5.0f});
    ASTRAL_CHECK(stuck.FootPosition().x > -6.0f + settings.radius - 0.02f);
    // Scene queries see the mesh like any other body.
    RaycastHit hit;
    ASTRAL_CHECK(world.Raycast({{0.0f, 5.0f, 5.0f}, {0, -1, 0}}, 10.0f, hit));
    ASTRAL_CHECK_NEAR(hit.point.y, 3.0f * rise, 1e-3);
    ASTRAL_CHECK_NEAR(hit.normal.y, std::cos(Math::Radians(25.0f)), 1e-3);
}

ASTRAL_TEST(CapsuleBesideATriangleMeasuresItsNearestVertex) {
    // Regression for an MSVC x64 Release-only failure found by the fuzz test:
    // the capsule core passes beside the triangle (not through it) and the
    // nearest feature is vertex C, reached through the edge candidates. Only
    // the endpoint candidates survived there, so casts sank into the triangle.
    const Vec3 a{-0x1.476064p+1f, 0x1.67eabp+1f, -0x1.2cffdp+1f};
    const Vec3 b{-0x1.1f6aecp+1f, 0x1.e3219ap+0f, -0x1.149af4p+1f};
    const Vec3 c{-0x1.7ccdd8p+0f, 0x1.52029ap+1f, -0x1.839b28p-1f};
    std::string error;
    const auto triangle = TriangleMesh::Create({a, b, c}, {0, 1, 2}, error);
    ASTRAL_CHECK(triangle && triangle->TriangleCount() == 1);
    if (!triangle) return;
    const Shape mesh = Shape::Mesh(triangle);
    Shape capsule;
    capsule.type = ShapeType::Capsule;
    capsule.radius = 0x1.8f984cp-3f;
    capsule.halfHeight = 0x1.f05792p-2f;
    const Pose meshPose{{0.5f, -0.2f, 0.1f}, Math::Quat{0.0f, 0x1.320ca0p-3f, 0.0f, 0x1.fa4034p-1f}};
    const Pose pose{{-0x1.def4p-1f, 0x1.35233p+1f, -0x1.3808p-8f},
        Math::Quat{-0x1.63ed42p-2f, 0x1.93d7b2p-1f, 0x1.05295ep-2f, 0x1.c0b33p-2f}};

    // Reference: the smallest of the endpoint and per-edge distances from the
    // core segment (in mesh space), computed here independently.
    const Vec3 axis = pose.TransformVector({0.0f, capsule.halfHeight, 0.0f});
    const Vec3 p0 = meshPose.InverseTransformPoint(pose.position - axis);
    const Vec3 p1 = meshPose.InverseTransformPoint(pose.position + axis);
    float reference = std::min(Math::Length(Math::ClosestPointOnTriangle(p0, a, b, c) - p0),
        Math::Length(Math::ClosestPointOnTriangle(p1, a, b, c) - p1));
    float edgeDistances[3] = {}, edgeS[3] = {}, edgeT[3] = {};
    auto edge = [&](int index, Vec3 from, Vec3 to) {
        Vec3 onSegment, onEdge;
        Math::ClosestPointsSegmentSegment(p0, p1, from, to, edgeS[index], edgeT[index], onSegment, onEdge);
        edgeDistances[index] = Math::Length(onSegment - onEdge);
    };
    edge(0, a, b);
    edge(1, b, c);
    edge(2, c, a);
    reference = std::min(reference, std::min(edgeDistances[0], std::min(edgeDistances[1], edgeDistances[2])));
    if (!(std::fabs(reference - 0.2254428f) <= 1.0e-4f)) {
        for (int e = 0; e < 3; ++e) {
            std::fprintf(stderr, "  edge %d s %.9g t %.9g distance %.9g\n", e, static_cast<double>(edgeS[e]),
                static_cast<double>(edgeT[e]), static_cast<double>(edgeDistances[e]));
        }
    }
    ASTRAL_CHECK_NEAR(reference, 0.2254428f, 1.0e-4);

    Vec3 normal, point;
    const float distance = RoundedDistance(capsule, pose, mesh, meshPose, normal, point);
    if (!(std::fabs(distance - (reference - capsule.radius)) <= 1.0e-4f)) {
        std::fprintf(stderr, "  rounded distance %.9g, expected %.9g (edges %.9g %.9g %.9g)\n", distance,
            reference - capsule.radius, edgeDistances[0], edgeDistances[1], edgeDistances[2]);
    }
    ASTRAL_CHECK_NEAR(distance, reference - capsule.radius, 1.0e-4);
    // The nearest point is vertex C.
    ASTRAL_CHECK_NEAR(Math::Length(point - meshPose.TransformPoint(c)), 0.0f, 1.0e-4);
    Proximity nearest;
    ASTRAL_CHECK(RoundedProximities(capsule, pose, mesh, meshPose, 100.0f, &nearest, 1) == 1);
    ASTRAL_CHECK_NEAR(nearest.distance, distance, 1.0e-6);

    // The fuzz case's cast stops before the vertex instead of sinking past it.
    const Vec3 motion{-0x1.cf824cp+1f, 0x1.12994p+0f, -0x1.f2d264p+1f};
    ShapeCastResult cast;
    ASTRAL_CHECK(ShapeCast(capsule, pose, motion, mesh, meshPose, cast));
    ASTRAL_CHECK(cast.fraction < 0.02f);
    for (int k = 0; k <= 20; ++k) {
        const Pose along{pose.position + motion * (cast.fraction * static_cast<float>(k) / 20.0f), pose.rotation};
        ASTRAL_CHECK(RoundedDistance(capsule, along, mesh, meshPose, normal, point) >= -1.0e-3f - 1.0e-5f);
    }
}

ASTRAL_TEST(MeshCollisionFuzzStaysFinite) {
    Core::Random random(1234);
    MeshBuilder soup;
    // Random draws are sequenced into locals (never function arguments, whose
    // evaluation order differs between compilers) so every platform fuzzes the same cases.
    auto point = [&] {
        const float x = random.Range(-3.0f, 3.0f);
        const float y = random.Range(-3.0f, 3.0f);
        const float z = random.Range(-3.0f, 3.0f);
        return Vec3{x, y, z};
    };
    for (int i = 0; i < 200; ++i) {
        const Vec3 a = point();
        const Vec3 b = a + (point() - a) * 0.3f;
        const Vec3 c = a + (point() - a) * 0.3f;
        const Vec3 outward = point();
        soup.Triangle(a, b, c, outward);
    }
    const Shape mesh = Shape::Mesh(soup.Build());
    const Pose meshPose{{0.5f, -0.2f, 0.1f}, Math::QuatFromAxisAngle({0, 1, 0}, 0.3f)};
    std::vector<Proximity> all(256);
    for (int i = 0; i < 300; ++i) {
        const float kind = random.NextFloat();
        Shape shape;
        if (kind < 0.33f) {
            shape = Shape::Sphere(random.Range(0.05f, 1.0f));
        } else if (kind < 0.66f) {
            const float radius = random.Range(0.05f, 0.6f);
            shape = Shape::CapsuleFromHeight(radius, random.Range(1.3f, 3.0f));
        } else {
            shape = Shape::Box({random.Range(0.05f, 1.0f), random.Range(0.05f, 1.0f), random.Range(0.05f, 1.0f)});
        }
        const Vec3 position{random.Range(-4.0f, 4.0f), random.Range(-4.0f, 4.0f), random.Range(-4.0f, 4.0f)};
        const Vec3 axis = Math::Normalize(Vec3{random.Range(-1.0f, 1.0f), random.Range(-1.0f, 1.0f), 0.3f});
        const Pose pose{position, Math::QuatFromAxisAngle(axis, random.Range(-3.0f, 3.0f))};
        Manifold manifolds[kMaxManifolds];
        const bool meshFirst = random.NextFloat() < 0.5f;
        const int count = meshFirst ? CollideAll(mesh, meshPose, shape, pose, manifolds, kMaxManifolds)
                                    : CollideAll(shape, pose, mesh, meshPose, manifolds, kMaxManifolds);
        ASTRAL_CHECK(count >= 0 && count <= kMaxManifolds);
        for (int m = 0; m < count; ++m) {
            ASTRAL_CHECK(manifolds[m].count >= 1 && manifolds[m].count <= 4);
            ASTRAL_CHECK_NEAR(Math::Length(manifolds[m].normal), 1.0f, 1e-3);
            for (int p = 0; p < manifolds[m].count; ++p) {
                ASTRAL_CHECK(Math::IsFinite(manifolds[m].points[p].position));
                ASTRAL_CHECK(std::isfinite(manifolds[m].points[p].penetration) && manifolds[m].points[p].penetration >= 0.0f);
            }
        }
        Manifold single;
        ASTRAL_CHECK(Collide(shape, pose, mesh, meshPose, single) == (count > 0));
        if (shape.type == ShapeType::Box) continue;
        // The BVH nearest search agrees with the exhaustive per-triangle scan.
        Vec3 normal, point;
        const float nearest = RoundedDistance(shape, pose, mesh, meshPose, normal, point);
        const int found = RoundedProximities(shape, pose, mesh, meshPose, 100.0f, all.data(), static_cast<int>(all.size()));
        ASTRAL_CHECK(found == static_cast<int>(mesh.mesh->TriangleCount()));
        ASTRAL_CHECK_NEAR(nearest, all[0].distance, 1e-5);
        ASTRAL_CHECK(Math::IsFinite(normal) && Math::IsFinite(point));
        ASTRAL_CHECK((nearest < 0.0f) == (count > 0));
        ShapeCastResult cast;
        const Vec3 motion{random.Range(-4.0f, 4.0f), random.Range(-4.0f, 4.0f), random.Range(-4.0f, 4.0f)};
        if (ShapeCast(shape, pose, motion, mesh, meshPose, cast)) {
            ASTRAL_CHECK(cast.fraction >= 0.0f && cast.fraction <= 1.0f);
            ASTRAL_CHECK(Math::IsFinite(cast.normal) && Math::IsFinite(cast.point));
            // Nothing is penetrated (beyond the cast tolerance) before the reported fraction.
            const Pose before{pose.position + motion * std::max(0.0f, cast.fraction - 0.02f), pose.rotation};
            if (!cast.startPenetrating && cast.fraction > 0.02f) {
                const float depth = RoundedDistance(shape, before, mesh, meshPose, normal, point);
                if (!(depth >= -1.0e-3f - 1.0e-5f)) {
                    // Bit-exact inputs (hex floats), so a platform-specific failure can be replayed.
                    std::fprintf(stderr, "cast failure at iteration %d: type %d radius %a halfHeight %a\n", i,
                        static_cast<int>(shape.type), shape.radius, shape.halfHeight);
                    std::fprintf(stderr, "  pose %a %a %a | %a %a %a %a\n", pose.position.x, pose.position.y, pose.position.z,
                        pose.rotation.x, pose.rotation.y, pose.rotation.z, pose.rotation.w);
                    std::fprintf(stderr, "  mesh rotation %a %a %a %a\n", meshPose.rotation.x, meshPose.rotation.y,
                        meshPose.rotation.z, meshPose.rotation.w);
                    std::fprintf(stderr, "  motion %a %a %a fraction %.9g depth %.9g normal %.6g %.6g %.6g\n", motion.x,
                        motion.y, motion.z, cast.fraction, depth, cast.normal.x, cast.normal.y, cast.normal.z);
                    double checksum = 0.0;
                    for (std::size_t t = 0; t < mesh.mesh->TriangleCount(); ++t) {
                        const TriangleMesh::Triangle& tri = mesh.mesh->GetTriangle(t);
                        checksum += static_cast<double>(tri.a.x) + tri.b.y * 2.0 + tri.c.z * 3.0;
                        if (std::fabs(tri.a.x + 2.55761f) < 0.01f || std::fabs(tri.b.x + 2.55761f) < 0.01f || std::fabs(tri.c.x + 2.55761f) < 0.01f) {
                            std::fprintf(stderr, "  triangle %zu a %a %a %a b %a %a %a c %a %a %a edges %d\n", t, tri.a.x, tri.a.y,
                                tri.a.z, tri.b.x, tri.b.y, tri.b.z, tri.c.x, tri.c.y, tri.c.z, tri.activeEdges);
                        }
                    }
                    std::fprintf(stderr, "  triangles %zu checksum %.17g\n", mesh.mesh->TriangleCount(), checksum);
                    const int near = RoundedProximities(shape, pose, mesh, meshPose, 100.0f, all.data(), static_cast<int>(all.size()));
                    for (int k = 0; k < 3 && k < near; ++k) {
                        std::fprintf(stderr, "  nearest %d distance %.9g point %.6g %.6g %.6g\n", k, all[static_cast<std::size_t>(k)].distance,
                            all[static_cast<std::size_t>(k)].pointOnB.x, all[static_cast<std::size_t>(k)].pointOnB.y,
                            all[static_cast<std::size_t>(k)].pointOnB.z);
                    }
                    for (int k = 0; k <= 20; ++k) {
                        const float f = cast.fraction * static_cast<float>(k) / 20.0f;
                        const Pose along{pose.position + motion * f, pose.rotation};
                        std::fprintf(stderr, "  f %.6f distance %.9g\n", f, RoundedDistance(shape, along, mesh, meshPose, normal, point));
                    }
                }
                ASTRAL_CHECK(depth >= -1.0e-3f - 1.0e-5f);
            }
        }
    }
}

ASTRAL_TEST(ShapeCastsNeverSinkPastTheTolerance) {
    // Starting in resting contact (within the tolerance) and sliding a long way
    // with a slight downward slope: either the cast reports a hit, or no pose
    // along the motion penetrates deeper than the tolerance.
    constexpr float kTolerance = 1.0e-3f;
    MeshBuilder floorMesh;
    floorMesh.Grid(-20.0f, -20.0f, 4, 4, 10.0f, Flat);
    const Shape targets[] = {Shape::Box({20.0f, 0.5f, 20.0f}), Shape::Mesh(floorMesh.Build())};
    const Pose targetPoses[] = {{{0.0f, -0.5f, 0.0f}, {}}, {{}, {}}};
    for (int t = 0; t < 2; ++t) {
        for (const float start : {-0.0009f, -0.0005f, 0.0f, 0.0005f, 0.0009f}) {
            for (const float drop : {0.0f, 0.0003f, 0.00063f, 0.0015f}) {
                const Shape probe = Shape::Sphere(0.5f);
                const Pose from{{-3.5f, 0.5f + start, 0.3f}, {}};
                const Vec3 motion{7.0f, -drop, 0.0f};
                ShapeCastResult result;
                const bool hit = ShapeCast(probe, from, motion, targets[t], targetPoses[t], result, kTolerance);
                const float reached = hit ? result.fraction : 1.0f;
                Vec3 normal, point;
                for (int k = 0; k <= 20; ++k) {
                    const Pose along{from.position + motion * (reached * static_cast<float>(k) / 20.0f), {}};
                    ASTRAL_CHECK(RoundedDistance(probe, along, targets[t], targetPoses[t], normal, point) >= -kTolerance - 1.0e-5f);
                }
            }
        }
    }
}

ASTRAL_TEST_MAIN("EnginePhysicsTests")
