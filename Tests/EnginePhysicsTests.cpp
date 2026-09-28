#include "Engine/Core/Random.h"
#include "Engine/Physics/BroadPhase.h"
#include "Engine/Physics/CharacterController.h"
#include "Engine/Physics/Collision.h"
#include "Engine/Physics/Destruction.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Tests/EngineTestSupport.h"

#include <cstdio>
#include <set>

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

ASTRAL_TEST_MAIN("EnginePhysicsTests")
