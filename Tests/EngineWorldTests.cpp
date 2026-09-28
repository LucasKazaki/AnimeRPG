#include "Engine/World/Components.h"
#include "Engine/World/Registry.h"
#include "Engine/World/Serialization.h"
#include "Tests/EngineTestSupport.h"

#include <stdexcept>
#include <string>

using namespace Astral;
using namespace Astral::World;

namespace {
struct Health {
    int value{};
};
struct Velocity {
    Math::Vec3 value{};
};
struct Tag {};
} // namespace

ASTRAL_TEST(EntitiesAreGenerational) {
    Registry registry;
    const Entity a = registry.Create();
    const Entity b = registry.Create();
    ASTRAL_CHECK(registry.Valid(a) && registry.Valid(b) && registry.AliveCount() == 2);
    registry.Add<Health>(a, Health{10});
    registry.Destroy(a);
    ASTRAL_CHECK(!registry.Valid(a));
    ASTRAL_CHECK(registry.Get<Health>(a) == nullptr);
    const Entity c = registry.Create();
    ASTRAL_CHECK(c.index == a.index && c.generation != a.generation);
    ASTRAL_CHECK(registry.Get<Health>(c) == nullptr); // no component leaks into the reused slot
    bool threw = false;
    try {
        registry.Add<Health>(a, Health{1});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    ASTRAL_CHECK(threw);
    ASTRAL_CHECK(!registry.Valid(kNullEntity));
    registry.Destroy(a); // double destroy is a no-op
    ASTRAL_CHECK(registry.AliveCount() == 2);
}

ASTRAL_TEST(ComponentPoolsStayPackedAfterRemoval) {
    Registry registry;
    std::vector<Entity> entities;
    for (int i = 0; i < 100; ++i) {
        const Entity e = registry.Create();
        entities.push_back(e);
        registry.Add<Health>(e, Health{i});
        if (i % 3 == 0) registry.Add<Velocity>(e, Velocity{{static_cast<float>(i), 0, 0}});
    }
    for (int i = 0; i < 100; i += 2) registry.Remove<Health>(entities[static_cast<std::size_t>(i)]);
    ASTRAL_CHECK(registry.Count<Health>() == 50);
    for (int i = 1; i < 100; i += 2) ASTRAL_CHECK(registry.Get<Health>(entities[static_cast<std::size_t>(i)])->value == i);
    int visited = 0;
    registry.Each<Health, Velocity>([&](Entity, Health& h, Velocity& v) {
        ASTRAL_CHECK(static_cast<int>(v.value.x) == h.value);
        ASTRAL_CHECK(h.value % 2 == 1 && h.value % 3 == 0);
        h.value += 1000;
        ++visited;
    });
    ASTRAL_CHECK(visited == 17); // odd multiples of 3 below 100
    int missing = 0;
    registry.Each<Health, Tag>([&](Entity, Health&, Tag&) { ++missing; });
    ASTRAL_CHECK(missing == 0);
}

ASTRAL_TEST(CommandBufferDefersStructuralChanges) {
    Registry registry;
    for (int i = 0; i < 10; ++i) registry.Add<Health>(registry.Create(), Health{i});
    CommandBuffer commands;
    registry.Each<Health>([&](Entity e, Health& h) {
        if (h.value < 5) commands.Destroy(e);
        else commands.Add<Tag>(e, Tag{});
    });
    ASTRAL_CHECK(registry.AliveCount() == 10);
    commands.Flush(registry);
    ASTRAL_CHECK(registry.AliveCount() == 5);
    ASTRAL_CHECK(registry.Count<Tag>() == 5);
}

ASTRAL_TEST(HierarchyRejectsCyclesAndPropagatesTransforms) {
    Registry registry;
    const Entity root = registry.Create();
    const Entity arm = registry.Create();
    const Entity hand = registry.Create();
    registry.Add<LocalTransform>(root, LocalTransform{{{10, 0, 0}, Math::QuatFromYaw(Math::kHalfPi), {1, 1, 1}}});
    registry.Add<LocalTransform>(arm, LocalTransform{{{0, 0, 2}, {}, {1, 1, 1}}});
    registry.Add<LocalTransform>(hand, LocalTransform{{{0, 1, 0}, {}, {1, 1, 1}}});
    ASTRAL_CHECK(SetParent(registry, arm, root));
    ASTRAL_CHECK(SetParent(registry, hand, arm));
    ASTRAL_CHECK(!SetParent(registry, root, hand)); // cycle
    ASTRAL_CHECK(!SetParent(registry, root, root)); // self
    ASTRAL_CHECK(IsAncestor(registry, root, hand));
    const TransformUpdateStats stats = UpdateTransforms(registry);
    ASTRAL_CHECK(stats.updated == 3 && stats.maxDepth == 2);
    // Root yaw of +90 deg turns the arm's +Z offset into +X.
    const Math::Vec3 handWorld = registry.Get<WorldTransform>(hand)->trs.translation;
    ASTRAL_CHECK_NEAR(handWorld.x, 12.0f, 1e-4);
    ASTRAL_CHECK_NEAR(handWorld.y, 1.0f, 1e-4);
    ASTRAL_CHECK_NEAR(handWorld.z, 0.0f, 1e-4);
    const Math::Vec3 fromMatrix = Math::GetTranslation(registry.Get<WorldTransform>(hand)->matrix);
    ASTRAL_CHECK_NEAR(fromMatrix.x, 12.0f, 1e-4);
    // Re-parenting moves the subtree; detaching makes it a root.
    ASTRAL_CHECK(SetParent(registry, hand, root));
    ASTRAL_CHECK(GetChildren(registry, root).size() == 2);
    ASTRAL_CHECK(GetChildren(registry, arm).empty());
    ASTRAL_CHECK(SetParent(registry, hand, kNullEntity));
    ASTRAL_CHECK(GetParent(registry, hand).IsNull());
    DestroyRecursive(registry, root);
    ASTRAL_CHECK(!registry.Valid(root) && !registry.Valid(arm) && registry.Valid(hand));
}

ASTRAL_TEST(DeepHierarchyIsIterative) {
    Registry registry;
    Entity previous = registry.Create();
    registry.Add<LocalTransform>(previous, LocalTransform{{{0, 1, 0}, {}, {1, 1, 1}}});
    for (int i = 0; i < 20000; ++i) {
        const Entity e = registry.Create();
        registry.Add<LocalTransform>(e, LocalTransform{{{0, 1, 0}, {}, {1, 1, 1}}});
        ASTRAL_CHECK(SetParent(registry, e, previous));
        previous = e;
    }
    const TransformUpdateStats stats = UpdateTransforms(registry);
    ASTRAL_CHECK(stats.maxDepth == 20000);
    ASTRAL_CHECK_NEAR(registry.Get<WorldTransform>(previous)->trs.translation.y, 20001.0f, 1e-1);
}

ASTRAL_TEST(WorldRoundTripsThroughText) {
    Registry registry;
    const Entity lincoln = registry.Create();
    registry.Add<Name>(lincoln, Name{"Lincoln \"Memorial\""});
    registry.Add<LocalTransform>(lincoln, LocalTransform{{{-8, 0, 18}, Math::QuatFromYaw(0.25f), {1, 2, 1}}});
    const Entity statue = registry.Create();
    registry.Add<Name>(statue, Name{"Statue"});
    registry.Add<LocalTransform>(statue, LocalTransform{{{0, 3, 1}, {}, {1, 1, 1}}});
    SetParent(registry, statue, lincoln);
    registry.Add<Health>(statue, Health{77});
    WorldSerializer serializer;
    serializer.Register("Health",
        [](const Registry& r, Entity e, std::string& out) {
            const Health* h = r.Get<Health>(e);
            if (!h) return false;
            out = std::to_string(h->value);
            return true;
        },
        [](Registry& r, Entity e, const std::vector<std::string>& args, std::string& error) {
            if (args.size() != 1) return error = "Health expects 1 value", false;
            r.Add<Health>(e, Health{std::stoi(args[0])});
            return true;
        });
    const std::string text = serializer.Save(registry);
    Registry loaded;
    std::string error;
    ASTRAL_CHECK(serializer.Load(text, loaded, error));
    ASTRAL_CHECK(loaded.AliveCount() == 2);
    ASTRAL_CHECK(serializer.Save(loaded) == text); // stable round trip
    int statues = 0;
    loaded.Each<Name>([&](Entity e, Name& name) {
        if (name.value == "Statue") {
            ++statues;
            ASTRAL_CHECK(loaded.Get<Health>(e)->value == 77);
            const Entity parent = GetParent(loaded, e);
            ASTRAL_CHECK(loaded.Get<Name>(parent)->value == "Lincoln \"Memorial\"");
        }
    });
    ASTRAL_CHECK(statues == 1);
}

ASTRAL_TEST(CorruptWorldsAreRejectedWithoutSideEffects) {
    WorldSerializer serializer;
    Registry target;
    const Entity keep = target.Create();
    target.Add<Name>(keep, Name{"keep"});
    const char* bad[] = {
        "",
        "ASTRAL_WORLD 2\n",
        "NOT_A_WORLD 1\n",
        "ASTRAL_WORLD 1\nentity 0\nName \"unterminated\nend\n",
        "ASTRAL_WORLD 1\nentity 0\nend\nentity 0\nend\n",
        "ASTRAL_WORLD 1\nentity 0\nParent 5\nend\n",
        "ASTRAL_WORLD 1\nentity 0\nParent 1\nend\nentity 1\nParent 0\nend\n",
        "ASTRAL_WORLD 1\nentity 0\nParent 0\nend\n",
        "ASTRAL_WORLD 1\nentity 0\nLocalTransform 1 2 3 0 0 0 1 1 1 nan\nend\n",
        "ASTRAL_WORLD 1\nentity 0\nLocalTransform 1 2 3\nend\n",
        "ASTRAL_WORLD 1\nentity 0\nMystery 1\nend\n",
        "ASTRAL_WORLD 1\nentity 0\n",
        "ASTRAL_WORLD 1\nName \"orphan\"\n",
        "ASTRAL_WORLD 1\nentity -3\nend\n",
        "ASTRAL_WORLD 1\nentity 0\nLocalTransform 1e999 0 0 0 0 0 1 1 1 1\nend\n",
    };
    for (const char* document : bad) {
        std::string error;
        ASTRAL_CHECK(!serializer.Load(document, target, error));
        ASTRAL_CHECK(!error.empty());
        ASTRAL_CHECK(target.AliveCount() == 1 && target.Get<Name>(keep)->value == "keep");
    }
    WorldLoadLimits limits;
    limits.maxEntities = 2;
    std::string error;
    ASTRAL_CHECK(!serializer.Load("ASTRAL_WORLD 1\nentity 0\nend\nentity 1\nend\nentity 2\nend\n", target, error, limits));
    serializer.strictUnknownComponents = false;
    ASTRAL_CHECK(serializer.Load("ASTRAL_WORLD 1\n# comment\nentity 0\nMystery 1\nend\n", target, error));
    ASTRAL_CHECK(target.AliveCount() == 1 && target.Get<Name>(keep) == nullptr);
}

ASTRAL_TEST_MAIN("EngineWorldTests")
