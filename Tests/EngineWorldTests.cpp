#include "Engine/World/Components.h"
#include "Engine/World/Registry.h"
#include "Engine/World/Serialization.h"
#include "Engine/Framework/GameWorld.h"
#include "Tests/EngineTestSupport.h"

#include <cstddef>
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

struct LifecycleCounts {
    int destroyed{};
    int disposed{};
    int fired{};
    int updated{};
};

struct LifecycleProbe final : Framework::Behaviour {
    explicit LifecycleProbe(std::shared_ptr<LifecycleCounts> counts) : counts_(std::move(counts)) {}
    ~LifecycleProbe() override { ++counts_->disposed; }
    void OnCreate() override {
        const auto counts = counts_;
        Invoke(0.01f, [counts] { ++counts->fired; });
    }
    void OnUpdate(float) override { ++counts_->updated; }
    void OnDestroy() override { ++counts_->destroyed; }
    std::shared_ptr<LifecycleCounts> counts_;
};

struct RemoveOtherBehaviours final : Framework::Behaviour {
    explicit RemoveOtherBehaviours(Entity target) : target_(target) {}
    void OnUpdate(float) override { World().Remove<Framework::Behaviours>(target_); }
    Entity target_;
};

struct ReentrantLifecycleProbe final : Framework::Behaviour {
    explicit ReentrantLifecycleProbe(std::shared_ptr<LifecycleCounts> counts) : counts_(std::move(counts)) {}
    void OnDestroy() override {
        ++counts_->destroyed;
        World().Remove<Framework::Behaviours>(Self());
        bool typedRejected = false, componentRejected = false;
        try { World().AddBehaviour<LifecycleProbe>(Self(), counts_); }
        catch (const std::invalid_argument&) { typedRejected = true; }
        try { World().Add<Framework::Behaviours>(Self(), Framework::Behaviours{}); }
        catch (const std::invalid_argument&) { componentRejected = true; }
        Framework::BehaviourRegistry types;
        types.Register<Framework::Behaviour>("Plain");
        std::string error;
        ASTRAL_CHECK(!World().AddBehaviour(Self(), "Plain", {}, error, types));
        ASTRAL_CHECK(error.find("teardown") != std::string::npos);
        ASTRAL_CHECK(typedRejected && componentRejected);
        ASTRAL_CHECK(Invoke(0, [] {}).IsNull());
        ASTRAL_CHECK(InvokeRepeating(0, 0.01f, [] {}).IsNull());
        // Teardown is scoped to this entity, not a global attachment ban.
        const Entity other = World().CreateEntity("CreatedDuringOtherTeardown");
        World().AddBehaviour<Framework::Behaviour>(other);
    }
    std::shared_ptr<LifecycleCounts> counts_;
};

struct DestroyDuringTeardown final : Framework::Behaviour {
    explicit DestroyDuringTeardown(std::shared_ptr<LifecycleCounts> counts) : counts_(std::move(counts)) {}
    void OnDestroy() override {
        ++counts_->destroyed;
        World().DestroyImmediate(Self());
    }
    std::shared_ptr<LifecycleCounts> counts_;
};

struct OneShotWitness {
    Audio::VoiceId voice;
    std::weak_ptr<const Audio::AudioClip> clip;
};

struct OneShotOnCreate final : Framework::Behaviour {
    explicit OneShotOnCreate(OneShotWitness& witness, bool owned = false, bool onDestroy = false) : witness_(witness),
        clip_(std::make_shared<Audio::AudioClip>(Audio::Synth::Tone(440, 0.25f))), owned_(owned), onDestroy_(onDestroy) {}
    void OnCreate() override { if (!onDestroy_) Play(); }
    void OnDestroy() override { if (onDestroy_) Play(); }
    void Play() {
        witness_.clip = clip_;
        witness_.voice = owned_ ? World().PlaySoundAt(clip_, {}) : World().PlaySoundAt(clip_.get(), {});
    }
    OneShotWitness& witness_;
    std::shared_ptr<Audio::AudioClip> clip_;
    bool owned_, onDestroy_;
};

Framework::GameWorldSettings RigidSyncSettings() {
    Framework::GameWorldSettings settings;
    settings.physics.gravity = {};
    settings.physics.allowSleep = false;
    return settings;
}

Framework::RigidBody MovingBody(float mass, Math::Vec3 velocity = {}) {
    Framework::RigidBody body;
    body.mass = mass;
    body.velocity = velocity;
    body.linearDamping = 0;
    body.angularDamping = 0;
    return body;
}

void CheckParentedDynamicPoses(bool childCreatedFirst, bool unbodiedIntermediate) {
    auto settings = RigidSyncSettings();
    settings.fixedDelta = 1.0f / 64.0f; // exact binary steps, including the multi-step frame
    Framework::GameWorld world(settings);
    const Math::Vec3 childStart{20, 4, 0}, parentStart{-20, -4, 0};
    const auto create = [&](const char* name, Math::Vec3 position) {
        return world.CreateEntity(name, {position, {}, {1, 1, 1}});
    };
    Entity child, parent;
    if (childCreatedFirst) {
        child = create("simulated child", childStart);
        parent = create("simulated parent", parentStart);
    } else {
        parent = create("simulated parent", parentStart);
        child = create("simulated child", childStart);
    }
    ASTRAL_CHECK((child.index < parent.index) == childCreatedFirst);
    Entity immediateParent = parent;
    Entity bridge{};
    if (unbodiedIntermediate) {
        bridge = create("unbodied intermediate", {3, 7, 0});
        ASTRAL_CHECK(world.SetParent(bridge, parent));
        immediateParent = bridge;
    }
    ASTRAL_CHECK(world.SetParent(child, immediateParent));
    ASTRAL_CHECK(Math::DistanceSquared(world.GetWorld(child).translation, childStart) < 1e-10f);
    ASTRAL_CHECK(World::GetParent(world.Registry(), child) == immediateParent);
    Framework::Collider tinyBox;
    tinyBox.halfExtents = {0.05f, 0.05f, 0.05f}; // independent bodies, far apart: no contact response
    world.Add<Framework::Collider>(child, tinyBox);
    world.Add<Framework::RigidBody>(child, MovingBody(1, {2, 0, 0}));
    world.Add<Framework::Collider>(parent, tinyBox);
    world.Add<Framework::RigidBody>(parent, MovingBody(1, {1, 0, 0}));
    const Entity follower = world.CreateEntity("unbodied child follower", {{0, 3, 0}, {}, {1, 1, 1}}, child);
    const Math::TRS followerLocal = world.GetLocal(follower);
    const Math::TRS bridgeLocal = bridge.IsNull() ? Math::TRS{} : world.GetLocal(bridge);
    world.Tick(0);
    ASTRAL_CHECK(world.Stats().fixedStepsLastFrame == 0 && world.Physics().BodyCount() == 0);
    Physics::BodyId childBody{}, parentBody{};
    int simulatedSteps = 0;
    for (const int steps : {1, 1, 3}) {
        world.Tick(settings.fixedDelta * steps);
        ASTRAL_CHECK(world.Stats().fixedStepsLastFrame == steps);
        simulatedSteps += steps;
        const auto currentChild = world.Get<Framework::Collider>(child)->body;
        const auto currentParent = world.Get<Framework::Collider>(parent)->body;
        if (childBody.IsNull()) { childBody = currentChild; parentBody = currentParent; }
        ASTRAL_CHECK(currentChild == childBody && currentParent == parentBody);
        const auto* childPhysics = world.Physics().GetBody(childBody);
        const auto* parentPhysics = world.Physics().GetBody(parentBody);
        ASTRAL_CHECK(childPhysics && parentPhysics && world.Physics().BodyCount() == 2);
        ASTRAL_CHECK(childPhysics->type == Physics::BodyType::Dynamic && parentPhysics->type == Physics::BodyType::Dynamic);
        ASTRAL_CHECK_NEAR(childPhysics->linearVelocity.x, 2, 1e-6f);
        ASTRAL_CHECK_NEAR(parentPhysics->linearVelocity.x, 1, 1e-6f);
        ASTRAL_CHECK_NEAR(childPhysics->pose.position.x, childStart.x + 2 * settings.fixedDelta * simulatedSteps, 1e-5f);
        ASTRAL_CHECK_NEAR(parentPhysics->pose.position.x, parentStart.x + settings.fixedDelta * simulatedSteps, 1e-5f);
        const auto childWorld = world.GetWorld(child);
        const auto parentWorld = world.GetWorld(parent);
        std::printf("parented child-first=%d bridge=%d fixed=%d: cached child x=%.9f body x=%.9f\n",
            childCreatedFirst, unbodiedIntermediate, simulatedSteps, childWorld.translation.x, childPhysics->pose.position.x);
        ASTRAL_CHECK(Math::DistanceSquared(childWorld.translation, childPhysics->pose.position) < 1e-10f);
        ASTRAL_CHECK(Math::DistanceSquared(parentWorld.translation, parentPhysics->pose.position) < 1e-10f);
        for (const Math::Vec3 axis : {Math::Vec3{1, 0, 0}, Math::Vec3{0, 1, 0}, Math::Vec3{0, 0, 1}}) {
            ASTRAL_CHECK(Math::DistanceSquared(Math::Rotate(childWorld.rotation, axis),
                Math::Rotate(childPhysics->pose.rotation, axis)) < 1e-10f);
        }
        // The renderer and non-bodied descendants use the actual hierarchy cache.
        const auto* cached = world.Get<World::WorldTransform>(child);
        ASTRAL_CHECK(cached && Math::DistanceSquared(Math::TransformPoint(cached->matrix, {}), childPhysics->pose.position) < 1e-10f);
        ASTRAL_CHECK(Math::DistanceSquared(world.GetWorld(follower).translation,
            Math::Combine(childWorld, followerLocal).translation) < 1e-10f);
        if (!bridge.IsNull()) {
            ASTRAL_CHECK(!world.Get<Framework::Collider>(bridge) && !world.Get<Framework::RigidBody>(bridge));
            ASTRAL_CHECK(Math::DistanceSquared(world.GetWorld(bridge).translation,
                Math::Combine(parentWorld, bridgeLocal).translation) < 1e-10f);
        }
    }
}
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

ASTRAL_TEST(DestroyingAnEntityRepairsHierarchyLinks) {
    Registry registry;
    const Entity parent = registry.Create();
    Entity children[4];
    for (Entity& child : children) {
        child = registry.Create();
        registry.Add<LocalTransform>(child, LocalTransform{{{1, 0, 0}, {}, {1, 1, 1}}});
        ASTRAL_CHECK(SetParent(registry, child, parent));
    }
    const Entity grandchild = registry.Create();
    ASTRAL_CHECK(SetParent(registry, grandchild, children[3]));
    registry.Add<LocalTransform>(parent, LocalTransform{{{0, 5, 0}, {}, {1, 1, 1}}});

    // Middle sibling, directly and through a command buffer.
    registry.Destroy(children[1]);
    CommandBuffer commands;
    commands.Destroy(children[2]);
    commands.Flush(registry);
    const std::vector<Entity> remaining = GetChildren(registry, parent);
    ASTRAL_CHECK(remaining.size() == 2 && remaining[0] == children[0] && remaining[1] == children[3]);
    ASTRAL_CHECK(registry.Get<Hierarchy>(parent)->childCount == 2);
    UpdateTransforms(registry);
    ASTRAL_CHECK_NEAR(registry.Get<WorldTransform>(children[3])->trs.translation.y, 5.0f, 1e-5);

    // Destroying a parent orphans its children as roots that still update.
    registry.Destroy(children[3]);
    ASTRAL_CHECK(GetParent(registry, grandchild).IsNull());
    ASTRAL_CHECK(GetChildren(registry, parent).size() == 1);
    const TransformUpdateStats stats = UpdateTransforms(registry);
    ASTRAL_CHECK(stats.updated == 3); // parent, children[0], grandchild
    // Recycled slots never inherit stale links.
    const Entity reused = registry.Create();
    ASTRAL_CHECK(!registry.Has<Hierarchy>(reused));
    ASTRAL_CHECK(GetChildren(registry, parent).size() == 1);
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

ASTRAL_TEST(RemovingBehaviourComponentReleasesLifecycleAndTimers) {
    const auto counts = std::make_shared<LifecycleCounts>();
    Framework::GameWorld world;
    const Entity entity = world.CreateEntity("LifecycleProbe");
    world.AddBehaviour<LifecycleProbe>(entity, counts);
    world.Remove<Framework::Behaviours>(entity);
    world.Tick(0.02f);
    std::printf("remove: destroyed=%d disposed=%d fired=%d\n", counts->destroyed, counts->disposed, counts->fired);
    ASTRAL_CHECK(world.IsAlive(entity));
    ASTRAL_CHECK(counts->fired == 0);
    ASTRAL_CHECK(counts->destroyed == 1 && counts->disposed == 1);
    ASTRAL_CHECK(world.Get<Framework::Behaviours>(entity) == nullptr);
}

ASTRAL_TEST(ReplacingBehaviourComponentReleasesLifecycleAndTimers) {
    const auto counts = std::make_shared<LifecycleCounts>();
    Framework::GameWorld world;
    const Entity entity = world.CreateEntity("LifecycleProbe");
    world.AddBehaviour<LifecycleProbe>(entity, counts);
    world.Add<Framework::Behaviours>(entity, Framework::Behaviours{});
    world.Tick(0.02f);
    std::printf("replace: destroyed=%d disposed=%d fired=%d\n", counts->destroyed, counts->disposed, counts->fired);
    ASTRAL_CHECK(world.IsAlive(entity));
    ASTRAL_CHECK(counts->fired == 0);
    ASTRAL_CHECK(counts->destroyed == 1 && counts->disposed == 1);
    ASTRAL_CHECK(world.Get<Framework::Behaviours>(entity)->list.empty());
}

ASTRAL_TEST(RemovedBehavioursCannotRunFromAnEarlierFrameSnapshot) {
    const auto counts = std::make_shared<LifecycleCounts>();
    Framework::GameWorld world;
    const Entity remover = world.CreateEntity("Remover");
    const Entity removed = world.CreateEntity("Removed");
    world.AddBehaviour<RemoveOtherBehaviours>(remover, removed);
    world.AddBehaviour<LifecycleProbe>(removed, counts);
    world.Tick(0.0f);
    std::printf("snapshot: destroyed=%d disposed=%d updated=%d\n", counts->destroyed, counts->disposed, counts->updated);
    ASTRAL_CHECK(counts->updated == 0);
    ASTRAL_CHECK(counts->destroyed == 1 && counts->disposed == 1);
    ASTRAL_CHECK(world.IsAlive(removed));
}

ASTRAL_TEST(BehaviourTeardownRejectsSameEntityReattachmentAndNewTimers) {
    const auto counts = std::make_shared<LifecycleCounts>();
    Framework::GameWorld world;
    const Entity entity = world.CreateEntity("Reentrant");
    world.AddBehaviour<ReentrantLifecycleProbe>(entity, counts);
    world.Remove<Framework::Behaviours>(entity);
    ASTRAL_CHECK(counts->destroyed == 1);
    ASTRAL_CHECK(world.Timers().Count() == 0);
    ASTRAL_CHECK(world.IsAlive(entity) && !world.Get<Framework::Behaviours>(entity));
    const Entity other = world.Find("CreatedDuringOtherTeardown");
    ASTRAL_CHECK(world.GetBehaviour<Framework::Behaviour>(other));
    // The guard is released; ordinary replacement attachment works afterward.
    world.AddBehaviour<Framework::Behaviour>(entity);
    world.Tick(0.0f);
    ASTRAL_CHECK(world.GetBehaviour<Framework::Behaviour>(entity)->Started());
}

ASTRAL_TEST(BehaviourReplacementHandlesRecursiveEntityDestruction) {
    const auto counts = std::make_shared<LifecycleCounts>();
    Framework::GameWorld world;
    const Entity entity = world.CreateEntity("DestroyDuringTeardown");
    world.AddBehaviour<DestroyDuringTeardown>(entity, counts);
    world.AddBehaviour<LifecycleProbe>(entity, counts);
    bool rejected = false;
    try { world.Add<Framework::Behaviours>(entity, Framework::Behaviours{}); }
    catch (const std::invalid_argument&) { rejected = true; }
    ASTRAL_CHECK(rejected && !world.IsAlive(entity));
    ASTRAL_CHECK(counts->destroyed == 2 && counts->disposed == 1);
    world.Tick(0.02f);
    ASTRAL_CHECK(counts->fired == 0);
}

ASTRAL_TEST(RemovedPendingBehavioursNeverCreateOrStart) {
    const auto counts = std::make_shared<LifecycleCounts>();
    Framework::GameWorld world;
    const Entity entity = world.CreateEntity("Pending");
    world.BeginCreateBatch();
    world.AddBehaviour<LifecycleProbe>(entity, counts);
    const auto retained = world.Get<Framework::Behaviours>(entity)->list.front();
    world.Remove<Framework::Behaviours>(entity);
    world.EndCreateBatch();
    world.Tick(0.02f);
    ASTRAL_CHECK(!retained->Started());
    ASTRAL_CHECK(counts->destroyed == 0 && counts->updated == 0 && counts->fired == 0);
    ASTRAL_CHECK(world.Timers().Count() == 0);
}

ASTRAL_TEST(AddingRigidBodyRebuildsExistingStaticCollider) {
    Framework::GameWorld world(RigidSyncSettings());
    const Entity entity = world.CreateEntity("neutral box", {{3, 4, 5}, {}, {1, 1, 1}});
    world.Add<Framework::Collider>(entity);
    const float step = world.Settings().fixedDelta;
    world.Tick(step);
    const Physics::BodyId previous = world.Get<Framework::Collider>(entity)->body;
    ASTRAL_CHECK(world.Physics().GetBody(previous)->type == Physics::BodyType::Static);
    world.Add<Framework::RigidBody>(entity, MovingBody(4, {3, 0, 0}));
    world.Tick(step);
    const Physics::BodyId current = world.Get<Framework::Collider>(entity)->body;
    const Physics::Body* body = world.Physics().GetBody(current);
    ASTRAL_CHECK(body);
    std::printf("add: type=%d inverseMass=%.6f\n", static_cast<int>(body->type), body->inverseMass);
    ASTRAL_CHECK(body->type == Physics::BodyType::Dynamic);
    ASTRAL_CHECK_NEAR(body->inverseMass, 0.25f, 1e-6f);
    ASTRAL_CHECK_NEAR(body->linearVelocity.x, 3, 1e-6f);
    ASTRAL_CHECK_NEAR(world.GetWorld(entity).translation.x, 3 + 3 * step, 1e-5f);
    ASTRAL_CHECK(current != previous && !world.Physics().GetBody(previous));
    ASTRAL_CHECK(world.EntityFromBody(previous).IsNull() && world.EntityFromBody(current) == entity);
    ASTRAL_CHECK(world.Physics().BodyCount() == 1);
}

ASTRAL_TEST(RemovingRigidBodyRestoresStaticCollider) {
    Framework::GameWorld world(RigidSyncSettings());
    const Entity entity = world.CreateEntity("neutral box", {{3, 4, 5}, {}, {1, 1, 1}});
    world.Add<Framework::Collider>(entity);
    world.Add<Framework::RigidBody>(entity, MovingBody(2, {2, 0, 0}));
    const float step = world.Settings().fixedDelta;
    world.Tick(step);
    const Physics::BodyId previous = world.Get<Framework::Collider>(entity)->body;
    const Math::Vec3 position = world.GetWorld(entity).translation;
    world.Remove<Framework::RigidBody>(entity);
    world.Tick(step);
    const Physics::BodyId current = world.Get<Framework::Collider>(entity)->body;
    const Physics::Body* body = world.Physics().GetBody(current);
    ASTRAL_CHECK(body);
    std::printf("remove: type=%d inverseMass=%.6f\n", static_cast<int>(body->type), body->inverseMass);
    ASTRAL_CHECK(body->type == Physics::BodyType::Static && body->inverseMass == 0);
    ASTRAL_CHECK_NEAR(Math::Length(body->linearVelocity), 0, 1e-6f);
    ASTRAL_CHECK_NEAR(Math::Distance(world.GetWorld(entity).translation, position), 0, 1e-6f);
    ASTRAL_CHECK(current != previous && !world.Physics().GetBody(previous));
    ASTRAL_CHECK(world.EntityFromBody(current) == entity && world.Physics().BodyCount() == 1);
}

ASTRAL_TEST(ReplacingRigidBodyAppliesNewPhysicsDescription) {
    Framework::GameWorld world(RigidSyncSettings());
    const Entity entity = world.CreateEntity("neutral box", {{3, 4, 5}, {}, {1, 1, 1}});
    world.Add<Framework::Collider>(entity);
    world.Add<Framework::RigidBody>(entity, MovingBody(2));
    const float step = world.Settings().fixedDelta;
    world.Tick(step);
    const Physics::BodyId previous = world.Get<Framework::Collider>(entity)->body;
    Framework::RigidBody replacement = MovingBody(8, {6, -1, 2});
    replacement.linearDamping = 0.3f;
    replacement.angularDamping = 0.4f;
    replacement.gravityScale = 0.2f;
    replacement.lockRotation = true;
    world.Add<Framework::RigidBody>(entity, replacement);
    world.Tick(step);
    const Physics::BodyId current = world.Get<Framework::Collider>(entity)->body;
    const Physics::Body* body = world.Physics().GetBody(current);
    ASTRAL_CHECK(body);
    std::printf("replace: type=%d inverseMass=%.6f\n", static_cast<int>(body->type), body->inverseMass);
    ASTRAL_CHECK_NEAR(body->inverseMass, 0.125f, 1e-6f);
    ASTRAL_CHECK(body->type == Physics::BodyType::Dynamic && body->lockRotation);
    ASTRAL_CHECK_NEAR(body->linearDamping, replacement.linearDamping, 1e-6f);
    ASTRAL_CHECK_NEAR(body->angularDamping, replacement.angularDamping, 1e-6f);
    ASTRAL_CHECK_NEAR(body->gravityScale, replacement.gravityScale, 1e-6f);
    const Math::Vec3 expectedVelocity = replacement.velocity / (1 + step * replacement.linearDamping);
    ASTRAL_CHECK_NEAR(Math::Distance(body->linearVelocity, expectedVelocity), 0, 1e-5f);
    ASTRAL_CHECK_NEAR(Math::Length(body->inverseInertiaLocal), 0, 1e-6f);
    ASTRAL_CHECK(current != previous && !world.Physics().GetBody(previous));
    ASTRAL_CHECK(world.Physics().BodyCount() == 1);
}

ASTRAL_TEST(RigidBodyChangesWaitForFixedSyncAndRespectComponentOrder) {
    Framework::GameWorld world(RigidSyncSettings());
    const Entity entity = world.CreateEntity("ordered components");
    world.Add<Framework::RigidBody>(entity, MovingBody(2));
    const float step = world.Settings().fixedDelta;
    world.Tick(step);
    ASTRAL_CHECK(world.Physics().BodyCount() == 0);
    world.Add<Framework::Collider>(entity);
    world.Tick(0);
    ASTRAL_CHECK(world.Get<Framework::Collider>(entity)->body.IsNull());
    world.Tick(step);
    const Physics::BodyId dynamic = world.Get<Framework::Collider>(entity)->body;
    ASTRAL_CHECK(world.Physics().GetBody(dynamic)->type == Physics::BodyType::Dynamic);
    Framework::RigidBody kinematic = MovingBody(5);
    kinematic.type = Physics::BodyType::Kinematic;
    world.Add<Framework::RigidBody>(entity, kinematic);
    ASTRAL_CHECK(!world.Physics().GetBody(dynamic));
    world.Tick(0);
    ASTRAL_CHECK(world.Physics().BodyCount() == 0);
    world.SetWorldPosition(entity, {9, 2, 3});
    world.Tick(step);
    const Physics::Body* body = world.Physics().GetBody(world.Get<Framework::Collider>(entity)->body);
    ASTRAL_CHECK(body && body->type == Physics::BodyType::Kinematic);
    ASTRAL_CHECK_NEAR(Math::Distance(body->pose.position, {9, 2, 3}), 0, 1e-6f);
    world.Remove<Framework::RigidBody>(entity);
    world.Tick(step);
    const Physics::BodyId staticBody = world.Get<Framework::Collider>(entity)->body;
    world.Remove<Framework::RigidBody>(entity); // absent: no runtime churn
    world.Tick(step);
    ASTRAL_CHECK(world.Get<Framework::Collider>(entity)->body == staticBody);
    world.Remove<Framework::Collider>(entity);
    ASTRAL_CHECK(world.Physics().BodyCount() == 0 && !world.Physics().GetBody(staticBody));
}

ASTRAL_TEST(RigidBodyChangesRetryFailedCreationWithoutAcceptingInvalidShapes) {
    Framework::GameWorld world(RigidSyncSettings());
    const Entity entity = world.CreateEntity("retry physics");
    world.Add<Framework::Collider>(entity);
    world.Add<Framework::RigidBody>(entity, MovingBody(0)); // invalid dynamic mass
    const float step = world.Settings().fixedDelta;
    world.Tick(step);
    ASTRAL_CHECK(world.Get<Framework::Collider>(entity)->failed && world.Physics().BodyCount() == 0);
    world.Add<Framework::RigidBody>(entity, MovingBody(4));
    world.Tick(step);
    ASTRAL_CHECK(!world.Get<Framework::Collider>(entity)->failed && world.Physics().BodyCount() == 1);
    ASTRAL_CHECK_NEAR(world.Physics().GetBody(world.Get<Framework::Collider>(entity)->body)->inverseMass, 0.25f, 1e-6f);
    world.Add<Framework::RigidBody>(entity, MovingBody(0));
    world.Tick(step);
    ASTRAL_CHECK(world.Get<Framework::Collider>(entity)->failed && world.Physics().BodyCount() == 0);
    world.Remove<Framework::RigidBody>(entity);
    world.Tick(step);
    ASTRAL_CHECK(!world.Get<Framework::Collider>(entity)->failed && world.Physics().BodyCount() == 1);
    Framework::Collider invalid;
    invalid.halfExtents = {};
    world.Add<Framework::Collider>(entity, invalid);
    world.Add<Framework::RigidBody>(entity, MovingBody(1));
    world.Tick(step);
    ASTRAL_CHECK(world.Get<Framework::Collider>(entity)->failed && world.Physics().BodyCount() == 0);
}

ASTRAL_TEST(RigidBodyChangesPreserveOtherBodiesMoversAndColliderConfiguration) {
    Framework::GameWorld world(RigidSyncSettings());
    const Entity changed = world.CreateEntity("changed", {{-10, 2, 0}, {}, {1, 1, 1}});
    Framework::Collider collider;
    collider.shape = Framework::ColliderShape::Sphere;
    collider.radius = 0.75f;
    collider.friction = 0.25f;
    collider.restitution = 0.3f;
    collider.isTrigger = true;
    collider.layer = 4;
    collider.mask = 8;
    world.Add<Framework::Collider>(changed, collider);
    const Entity untouched = world.CreateEntity("untouched", {{10, 2, 0}, {}, {1, 1, 1}});
    world.Add<Framework::Collider>(untouched);
    world.Add<Framework::RigidBody>(untouched, MovingBody(2, {1, 0, 0}));
    const Entity character = world.CreateEntity("neutral mover", {{0, 20, 0}, {}, {1, 1, 1}});
    world.Add<Framework::CharacterMover>(character);
    const float step = world.Settings().fixedDelta;
    world.Tick(step);
    const Physics::BodyId unchangedBody = world.Get<Framework::Collider>(untouched)->body;
    const Physics::BodyId moverBody = world.Get<Framework::CharacterMover>(character)->body;
    const auto controller = world.Get<Framework::CharacterMover>(character)->controller;
    world.Physics().ApplyForce(unchangedBody, {4, 0, 0});
    world.Add<Framework::RigidBody>(changed, MovingBody(3));
    const Physics::Body* unaffected = world.Physics().GetBody(unchangedBody);
    ASTRAL_CHECK(unaffected && unaffected->linearVelocity.x == 1 && unaffected->force.x == 4);
    for (int i = 0; i != 8; ++i) {
        Framework::RigidBody replacement = MovingBody(3);
        replacement.type = i % 2 ? Physics::BodyType::Dynamic : Physics::BodyType::Kinematic;
        world.Add<Framework::RigidBody>(changed, replacement);
        world.Tick(step);
        ASTRAL_CHECK(world.Physics().BodyCount() == 3);
        ASTRAL_CHECK(world.Get<Framework::Collider>(untouched)->body == unchangedBody);
        ASTRAL_CHECK(world.Get<Framework::CharacterMover>(character)->body == moverBody);
        ASTRAL_CHECK(world.Get<Framework::CharacterMover>(character)->controller == controller);
        const Physics::Body* body = world.Physics().GetBody(world.Get<Framework::Collider>(changed)->body);
        ASTRAL_CHECK(body && body->type == replacement.type && body->shape.type == Physics::ShapeType::Sphere);
        ASTRAL_CHECK_NEAR(body->shape.radius, collider.radius, 1e-6f);
        ASTRAL_CHECK(body->isTrigger && body->layer == 4 && body->mask == 8);
        ASTRAL_CHECK_NEAR(body->friction, collider.friction, 1e-6f);
        ASTRAL_CHECK_NEAR(body->restitution, collider.restitution, 1e-6f);
    }
    ASTRAL_CHECK(world.Physics().GetBody(unchangedBody)->linearVelocity.x > 1);
    world.DestroyImmediate(changed);
    ASTRAL_CHECK(world.Physics().BodyCount() == 2);
    world.DestroyImmediate(character);
    world.DestroyImmediate(untouched);
    ASTRAL_CHECK(world.Physics().BodyCount() == 0);
}

ASTRAL_TEST(RigidBodyChangesPreserveDynamicMeshRestriction) {
    Framework::GameWorld world(RigidSyncSettings());
    const Entity entity = world.CreateEntity("neutral triangle");
    Framework::Collider collider;
    collider.shape = Framework::ColliderShape::Mesh;
    std::string error;
    collider.triangles = Physics::TriangleMesh::Create({{-1, 0, -1}, {1, 0, -1}, {0, 0, 1}}, {0, 1, 2}, error);
    ASTRAL_CHECK(collider.triangles && error.empty());
    world.Add<Framework::Collider>(entity, collider);
    const float step = world.Settings().fixedDelta;
    world.Tick(step);
    world.Add<Framework::RigidBody>(entity, MovingBody(2));
    world.Tick(step);
    const Physics::Body* body = world.Physics().GetBody(world.Get<Framework::Collider>(entity)->body);
    ASTRAL_CHECK(body && body->type == Physics::BodyType::Kinematic);
    ASTRAL_CHECK(body->shape.mesh == collider.triangles);
    world.Remove<Framework::RigidBody>(entity);
    world.Tick(step);
    body = world.Physics().GetBody(world.Get<Framework::Collider>(entity)->body);
    ASTRAL_CHECK(body && body->type == Physics::BodyType::Static && world.Physics().BodyCount() == 1);
}

ASTRAL_TEST(WorldTeardownStopsOwnedOneShots) {
    Audio::AudioMixer mixer;
    OneShotWitness witness;
    {
        Framework::GameWorld world({}, &mixer);
        world.AddBehaviour<OneShotOnCreate>(world.CreateEntity("SoundOwner"), witness);
        ASTRAL_CHECK(mixer.IsPlaying(witness.voice) && !witness.clip.expired());
    }
    // Never Mix after old teardown: this detects the still-active borrowed
    // pointer safely, without dereferencing storage released with the behavior.
    ASTRAL_CHECK(witness.clip.expired());
    ASTRAL_CHECK(!mixer.IsPlaying(witness.voice));
}

ASTRAL_TEST(WorldSharedOneShotsSurviveBehaviourRemoval) {
    Audio::AudioMixer mixer;
    OneShotWitness witness;
    Framework::GameWorld world({}, &mixer);
    const Entity entity = world.CreateEntity("SharedSoundOwner");
    world.AddBehaviour<OneShotOnCreate>(entity, witness, true);
    world.Remove<Framework::Behaviours>(entity);
    ASTRAL_CHECK(mixer.IsPlaying(witness.voice) && !witness.clip.expired());
    std::vector<float> stereo(256 * 2);
    mixer.Mix(stereo.data(), 256);
    float energy = 0;
    for (float sample : stereo) energy += sample * sample;
    ASTRAL_CHECK(energy > 0 && mixer.IsPlaying(witness.voice));
    mixer.Stop(witness.voice);
    ASTRAL_CHECK(witness.clip.expired());
}

ASTRAL_TEST(WorldLegacyRawOneShotUsesAnIndependentSnapshot) {
    Audio::AudioMixer mixer;
    Framework::GameWorld world({}, &mixer);
    auto source = std::make_shared<Audio::AudioClip>();
    source->samples.assign(32, 0.25f);
    std::weak_ptr<Audio::AudioClip> original = source;
    const Audio::VoiceId voice = world.PlaySoundAt(source.get(), {});
    source->samples.assign(32, -0.25f);
    source.reset();
    ASTRAL_CHECK(original.expired() && mixer.IsPlaying(voice));
    float stereo[16]{};
    mixer.Mix(stereo, 8);
    for (float sample : stereo) ASTRAL_CHECK(sample > 0.1f && sample < 0.3f);
}

ASTRAL_TEST(WorldTeardownStopsCallbackSoundsWithoutStoppingOtherWorlds) {
    Audio::AudioMixer mixer(48000, 2);
    OneShotWitness callback;
    auto otherClip = std::make_shared<Audio::AudioClip>(Audio::Synth::Tone(220, 0.25f));
    std::weak_ptr<Audio::AudioClip> otherWeak = otherClip;
    Audio::VoiceId otherVoice;
    {
        Framework::GameWorld other({}, &mixer);
        otherVoice = other.PlaySoundAt(otherClip, {});
        otherClip.reset();
        {
            Framework::GameWorld departing({}, &mixer);
            departing.AddBehaviour<OneShotOnCreate>(departing.CreateEntity("FinalSound"), callback, true, true);
            ASTRAL_CHECK(callback.voice.IsNull());
        }
        ASTRAL_CHECK(!callback.voice.IsNull() && !mixer.IsPlaying(callback.voice) && callback.clip.expired());
        ASTRAL_CHECK(mixer.IsPlaying(otherVoice) && !otherWeak.expired());
        float stereo[32]{};
        mixer.Mix(stereo, 16);
        ASTRAL_CHECK(mixer.IsPlaying(otherVoice));
    }
    ASTRAL_CHECK(!mixer.IsPlaying(otherVoice) && otherWeak.expired());
}

ASTRAL_TEST(WorldOneShotCleanupIgnoresStolenVoiceGenerations) {
    Audio::AudioMixer mixer(48000, 1);
    auto clip = std::make_shared<Audio::AudioClip>(Audio::Synth::Tone(220, 0.25f));
    Audio::VoiceId replacement;
    {
        Framework::GameWorld departing({}, &mixer);
        const Audio::VoiceId old = departing.PlaySoundAt(clip, {});
        Audio::PlayParams params;
        replacement = mixer.Play(clip, params); // external replacement in the same slot
        ASTRAL_CHECK(old.index == replacement.index && old.generation != replacement.generation);
        ASTRAL_CHECK(!mixer.IsPlaying(old) && mixer.IsPlaying(replacement));
    }
    ASTRAL_CHECK(mixer.IsPlaying(replacement));
    mixer.Stop(replacement);
}

ASTRAL_TEST(WorldAudioSourceRetainsThePlayingClipUntilRelease) {
    Audio::AudioMixer mixer;
    Framework::GameWorld world({}, &mixer);
    const Entity entity = world.CreateEntity("AudioSource");
    Framework::AudioSource source;
    source.clip = std::make_shared<Audio::AudioClip>(Audio::Synth::Tone(220, 0.25f));
    std::weak_ptr<const Audio::AudioClip> weak = source.clip;
    world.Add<Framework::AudioSource>(entity, std::move(source));
    ASTRAL_CHECK(world.PlayAudio(entity));
    auto* component = world.Get<Framework::AudioSource>(entity);
    const Audio::VoiceId voice = component->voice;
    component->clip.reset();
    ASTRAL_CHECK(mixer.IsPlaying(voice) && !weak.expired());
    float stereo[32]{};
    mixer.Mix(stereo, 16);
    world.Remove<Framework::AudioSource>(entity);
    ASTRAL_CHECK(!mixer.IsPlaying(voice) && weak.expired());
}

ASTRAL_TEST(ParentedDynamicChildCreatedBeforeParentMatchesBodyPose) {
    CheckParentedDynamicPoses(true, false);
}

ASTRAL_TEST(ParentedDynamicChildCreatedAfterParentMatchesBodyPose) {
    CheckParentedDynamicPoses(false, false);
}

ASTRAL_TEST(ParentedDynamicChildThroughUnbodiedAncestorMatchesBodyPose) {
    CheckParentedDynamicPoses(false, true);
}

ASTRAL_TEST_MAIN("EngineWorldTests")
