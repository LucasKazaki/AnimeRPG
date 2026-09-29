#include "Engine/Animation/Skinning.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/Gltf.h"
#include "Engine/Audio/AudioMixer.h"
#include "Engine/Core/Json.h"
#include "Engine/Framework/GameWorld.h"
#include "Engine/Framework/Scene.h"
#include "Engine/Graphics/SceneRenderer.h"
#include "Tests/EngineTestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace Astral;
using namespace Astral::Framework;
using Core::JsonValue;
using Math::Quat;
using Math::TRS;
using Math::Vec3;

namespace {

std::string TempDir(const char* name) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(path);
    std::filesystem::create_directories(path);
    return path.string();
}

void WriteText(const std::string& path, const std::string& text) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << text;
}

JsonValue ParseText(const std::string& text) {
    JsonValue json;
    std::string error;
    const bool ok = Core::ParseJson(text, json, error);
    if (!ok) std::fprintf(stderr, "json: %s\n", error.c_str());
    ASTRAL_CHECK(ok);
    return json;
}

bool Near(Vec3 a, Vec3 b, float tolerance) { return Math::Length(a - b) <= tolerance; }

void Run(GameWorld& world, float seconds, float dt = 1.0f / 60.0f) {
    const int frames = static_cast<int>(seconds / dt + 0.5f);
    for (int i = 0; i < frames; ++i) world.Tick(dt);
}

// Records its callbacks into a shared log.
std::vector<std::string>& Log() {
    static std::vector<std::string> log;
    return log;
}

struct Recorder : Behaviour {
    std::string tag{"r"};
    void OnCreate() override { Log().push_back(tag + ":create"); }
    void OnStart() override { Log().push_back(tag + ":start"); }
    void OnFixedUpdate(float) override { Log().push_back(tag + ":fixed"); }
    void OnUpdate(float) override { Log().push_back(tag + ":update"); }
    void OnLateUpdate(float) override { Log().push_back(tag + ":late"); }
    void OnDestroy() override { Log().push_back(tag + ":destroy"); }
};

struct ContactRecorder : Behaviour {
    std::vector<CollisionInfo> enters, exits, triggerEnters, triggerExits;
    void OnCollisionEnter(const CollisionInfo& info) override { enters.push_back(info); }
    void OnCollisionExit(const CollisionInfo& info) override { exits.push_back(info); }
    void OnTriggerEnter(const CollisionInfo& info) override { triggerEnters.push_back(info); }
    void OnTriggerExit(const CollisionInfo& info) override { triggerExits.push_back(info); }
};

struct Spinner : Behaviour {
    float degreesPerSecond{90.0f};
    Vec3 axis{0.0f, 1.0f, 0.0f};
    void OnUpdate(float dt) override {
        const TRS local = World().GetLocal(Self());
        TRS next = local;
        next.rotation = Math::Normalize(Math::QuatFromAxisAngle(Math::Normalize(axis, {0, 1, 0}), Math::Radians(degreesPerSecond * dt)) * local.rotation);
        World().SetLocal(Self(), next);
    }
};

struct Walker : Behaviour {
    Vec3 velocity{3.0f, 0.0f, 0.0f};
    bool jumpNext{};
    void OnFixedUpdate(float) override {
        if (CharacterMover* mover = World().Get<CharacterMover>(Self())) {
            mover->desiredVelocity = velocity;
            if (jumpNext) {
                mover->jump = true;
                jumpNext = false;
            }
        }
    }
};

void RegisterTestBehaviours() {
    BehaviourRegistry& registry = BehaviourRegistry::Global();
    registry.Register<Spinner>("Spinner").Field("degreesPerSecond", &Spinner::degreesPerSecond).Field("axis", &Spinner::axis);
    registry.Register<Recorder>("Recorder").Field("tag", &Recorder::tag);
    registry.Register<Walker>("Walker").Field("velocity", &Walker::velocity);
}

Entity AddBox(GameWorld& world, const std::string& name, Vec3 position, Vec3 half, bool dynamic, float mass = 1.0f) {
    const Entity entity = world.CreateEntity(name, {position, {}, {1, 1, 1}});
    Collider collider;
    collider.shape = ColliderShape::Box;
    collider.halfExtents = half;
    world.Add<Collider>(entity, collider);
    if (dynamic) {
        RigidBody body;
        body.mass = mass;
        world.Add<RigidBody>(entity, body);
    }
    return entity;
}

} // namespace

// ------------------------------------------------------------------ timers

ASTRAL_TEST(TimersFireInOrderAndRespectOwners) {
    TimerManager timers;
    std::vector<std::string> fired;
    const TimerHandle late = timers.Set(0.5f, [&] { fired.push_back("late"); });
    const TimerHandle early = timers.Set(0.2f, [&] { fired.push_back("early"); });
    const TimerHandle loop = timers.Set(0.1f, [&] { fired.push_back("loop"); }, 0.25f);
    ASTRAL_CHECK(!late.IsNull() && !early.IsNull() && !loop.IsNull() && late != early);
    ASTRAL_CHECK(timers.Set(std::nanf(""), [] {}).IsNull() && timers.Set(1.0f, nullptr).IsNull());
    ASTRAL_CHECK_NEAR(timers.Remaining(late), 0.5f, 1e-6);
    ASTRAL_CHECK(timers.Tick(0.05f) == 0);
    // 0.6 s: loop@0.1, early@0.2, loop@0.35, late@0.5, loop@0.6 in deadline order.
    ASTRAL_CHECK(timers.Tick(0.55f) == 5);
    ASTRAL_CHECK((fired == std::vector<std::string>{"loop", "early", "loop", "late", "loop"}));
    ASTRAL_CHECK(!timers.IsActive(early) && timers.IsActive(loop));
    ASTRAL_CHECK(timers.SetPaused(loop, true));
    fired.clear();
    timers.Tick(2.0f);
    ASTRAL_CHECK(fired.empty());
    ASTRAL_CHECK_NEAR(timers.Remaining(loop), 0.25f, 1e-5);
    ASTRAL_CHECK(timers.SetPaused(loop, false));
    timers.Tick(0.25f);
    ASTRAL_CHECK(fired.size() == 1);
    // A callback may clear its own timer and set new ones.
    TimerHandle self{};
    int selfCount = 0;
    self = timers.Set(0.0f, [&] {
        ++selfCount;
        timers.Clear(self);
        timers.Set(0.0f, [&] { fired.push_back("spawned"); });
    }, 0.01f);
    timers.Tick(1.0f);
    ASTRAL_CHECK(selfCount == 1 && fired.back() == "spawned");
    // Owners cancel their timers; runaway loops are bounded per tick.
    const World::Entity owner{3, 1};
    timers.Set(1.0f, [&] { fired.push_back("owned"); }, 0.0f, owner);
    timers.Set(1.0f, [&] { fired.push_back("owned2"); }, 0.5f, owner);
    ASTRAL_CHECK(timers.ClearOwner(owner) == 2);
    timers.Clear(loop);
    timers.maxFiresPerTick = 100;
    int spins = 0;
    timers.Set(0.0f, [&] { ++spins; }, 1.0e-6f);
    ASTRAL_CHECK(timers.Tick(10.0f) == 100 && spins == 100);
    timers.ClearAll();
    ASTRAL_CHECK(timers.Count() == 0);
}

// ------------------------------------------------------------------ lifecycle

ASTRAL_TEST(BehaviourLifecycleRunsInTickOrder) {
    Log().clear();
    GameWorld world;
    const Entity a = world.CreateEntity("A");
    Recorder& recorder = world.AddBehaviour<Recorder>(a);
    ASTRAL_CHECK((Log() == std::vector<std::string>{"r:create"}));
    ASTRAL_CHECK(!recorder.Started() && recorder.Self() == a && &recorder.World() == &world);
    Log().clear();
    world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK((Log() == std::vector<std::string>{"r:start", "r:fixed", "r:update", "r:late"}));
    // A 30 Hz frame runs two 60 Hz fixed steps; a short frame runs none.
    Log().clear();
    world.Tick(1.0f / 30.0f);
    ASTRAL_CHECK((Log() == std::vector<std::string>{"r:fixed", "r:fixed", "r:update", "r:late"}));
    ASTRAL_CHECK(world.Stats().fixedStepsLastFrame == 2);
    Log().clear();
    world.Tick(1.0f / 240.0f);
    ASTRAL_CHECK((Log() == std::vector<std::string>{"r:update", "r:late"}));
    // A huge hitch is clamped and the step count bounded.
    world.Tick(10.0f);
    ASTRAL_CHECK(world.Stats().fixedStepsLastFrame <= world.Settings().maxFixedSteps);
    // Disabled behaviours get no ticks.
    recorder.enabled = false;
    Log().clear();
    world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK(Log().empty());
    recorder.enabled = true;
    // Time scale: paused worlds do not step or advance game time.
    world.Settings().timeScale = 0.0f;
    const float before = world.Time();
    Log().clear();
    world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK(world.Time() == before && world.Stats().fixedStepsLastFrame == 0);
    world.Settings().timeScale = 1.0f;

    // Destroy is deferred to the end of the frame; OnDestroy then runs once.
    struct SelfDestruct : Behaviour {
        void OnUpdate(float) override {
            World().Destroy(Self());
            Log().push_back(World().IsAlive(Self()) ? "alive" : "pending");
        }
    };
    const Entity b = world.CreateEntity("B", {}, a);
    world.AddBehaviour<SelfDestruct>(b);
    auto& childRecorder = world.AddBehaviour<Recorder>(b);
    childRecorder.tag = "c";
    Log().clear();
    world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK(!world.Registry().Valid(b));
    ASTRAL_CHECK(std::count(Log().begin(), Log().end(), "pending") == 1);
    ASTRAL_CHECK(std::count(Log().begin(), Log().end(), "r:destroy") == 0);
    ASTRAL_CHECK(Log().back() == "c:destroy");
    ASTRAL_CHECK(world.Stats().destroyedLastFrame == 1);

    // Timers owned by a behaviour's entity fire on game time and die with it.
    int ticks = 0;
    recorder.InvokeRepeating(0.05f, 0.1f, [&] { ++ticks; });
    Run(world, 0.5f);
    ASTRAL_CHECK(ticks >= 4 && ticks <= 5);
    ASTRAL_CHECK(world.Timers().Count() == 1);
    world.Destroy(a);
    world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK(world.Timers().Count() == 0 && !world.Registry().Valid(a));

    // Create batches defer OnCreate until every entity of the batch exists.
    struct Finder : Behaviour {
        Entity found{};
        void OnCreate() override { found = World().Find("Later"); }
    };
    world.BeginCreateBatch();
    const Entity first = world.CreateEntity("First");
    Finder& finder = world.AddBehaviour<Finder>(first);
    const Entity later = world.CreateEntity("Later");
    ASTRAL_CHECK(finder.found.IsNull());
    world.EndCreateBatch();
    ASTRAL_CHECK(finder.found == later);

    // Behaviours by registered name with reflected properties.
    RegisterTestBehaviours();
    std::string error;
    JsonValue properties = JsonValue::MakeObject();
    properties.Set("degreesPerSecond", 30.0);
    Behaviour* spinner = world.AddBehaviour(later, "Spinner", properties, error);
    ASTRAL_CHECK(spinner && spinner->TypeName() == "Spinner" && dynamic_cast<Spinner*>(spinner)->degreesPerSecond == 30.0f);
    ASTRAL_CHECK(world.GetBehaviour<Spinner>(later) == spinner);
    properties.Set("degreesPerSecond", "fast");
    ASTRAL_CHECK(!world.AddBehaviour(later, "Spinner", properties, error) && error.find("degreesPerSecond") != std::string::npos);
    ASTRAL_CHECK(!world.AddBehaviour(later, "Nope", {}, error) && error.find("unknown behaviour") != std::string::npos);
    JsonValue unknownField = JsonValue::MakeObject();
    unknownField.Set("spin", 1.0);
    ASTRAL_CHECK(!world.AddBehaviour(later, "Spinner", unknownField, error));
}

// ------------------------------------------------------------------ physics

ASTRAL_TEST(PhysicsSyncMovesEntitiesAndReportsContacts) {
    GameWorld world;
    const Entity floor = AddBox(world, "Floor", {0, -0.5f, 0}, {20, 0.5f, 20}, false);
    const Entity ball = world.CreateEntity("Ball", {{0, 3, 0}, {}, {1, 1, 1}});
    Collider sphere;
    sphere.shape = ColliderShape::Sphere;
    sphere.radius = 0.5f;
    world.Add<Collider>(ball, sphere);
    world.Add<RigidBody>(ball);
    ContactRecorder& contacts = world.AddBehaviour<ContactRecorder>(ball);
    // A child rides along with its simulated parent.
    const Entity marker = world.CreateEntity("Marker", {{0, 1, 0}, {}, {1, 1, 1}}, ball);
    Run(world, 2.0f);
    ASTRAL_CHECK_NEAR(world.GetWorld(ball).translation.y, 0.5f, 0.03f);
    ASTRAL_CHECK_NEAR(world.GetWorld(marker).translation.y, 1.5f, 0.05f);
    // (A landing bounce may end and restart the contact; every report names the floor.)
    ASTRAL_CHECK(!contacts.enters.empty() && contacts.enters[0].other == floor && !contacts.enters[0].trigger);
    ASTRAL_CHECK(contacts.enters[0].normal.y < -0.9f); // from the ball toward the floor
    ASTRAL_CHECK(world.Stats().bodies == 2);

    // Moving a simulated entity from code teleports its body.
    world.SetWorldPosition(ball, {5, 3, 0});
    Run(world, 0.1f);
    ASTRAL_CHECK(world.GetWorld(ball).translation.x > 4.9f);
    ASTRAL_CHECK(!contacts.exits.empty() && contacts.exits.back().other == floor);
    Run(world, 2.0f);
    ASTRAL_CHECK_NEAR(world.GetWorld(ball).translation.y, 0.5f, 0.03f);

    // Triggers report overlaps without blocking.
    const Entity zone = AddBox(world, "Zone", {-4, 1, 0}, {1, 1, 1}, false);
    world.Get<Collider>(zone)->isTrigger = true;
    ContactRecorder& zoneContacts = world.AddBehaviour<ContactRecorder>(zone);
    const Entity crate = AddBox(world, "Crate", {-4, 4, 0}, {0.3f, 0.3f, 0.3f}, true, 2.0f);
    Run(world, 2.0f);
    ASTRAL_CHECK_NEAR(world.GetWorld(crate).translation.y, 0.3f, 0.03f); // fell through the trigger
    ASTRAL_CHECK(zoneContacts.triggerEnters.size() == 1 && zoneContacts.triggerEnters[0].other == crate);
    ASTRAL_CHECK(zoneContacts.enters.empty());

    // A kinematic paddle driven by its transform pushes the ball.
    const Entity paddle = AddBox(world, "Paddle", {3, 0.5f, 0}, {0.2f, 0.5f, 1}, true);
    world.Get<RigidBody>(paddle)->type = Physics::BodyType::Kinematic;
    for (int i = 0; i < 60; ++i) {
        world.SetWorldPosition(paddle, {3.0f + 0.05f * static_cast<float>(i), 0.5f, 0});
        world.Tick(1.0f / 60.0f);
    }
    ASTRAL_CHECK(world.GetWorld(ball).translation.x > 5.8f);

    // Queries map back to entities; destruction removes bodies.
    EntityRaycastHit hit;
    ASTRAL_CHECK(world.Raycast({{world.GetWorld(ball).translation.x, 5, 0}, {0, -1, 0}}, 10.0f, hit));
    ASTRAL_CHECK(hit.entity == ball);
    ASTRAL_CHECK(world.Raycast({{world.GetWorld(ball).translation.x, 5, 0}, {0, -1, 0}}, 10.0f, hit, 0xFFFFFFFFu, ball));
    ASTRAL_CHECK(hit.entity == floor);
    const std::size_t bodies = world.Physics().BodyCount();
    world.Destroy(crate);
    world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK(world.Physics().BodyCount() == bodies - 1);
    world.Remove<Collider>(paddle);
    ASTRAL_CHECK(world.Physics().BodyCount() == bodies - 2 && !world.Get<Collider>(paddle));
    ASTRAL_CHECK(world.EntityFromBody({}).IsNull());
}

ASTRAL_TEST(CharacterMoverWalksJumpsAndPushes) {
    RegisterTestBehaviours();
    GameWorld world;
    AddBox(world, "Floor", {0, -0.5f, 0}, {30, 0.5f, 30}, false);
    const Entity hero = world.CreateEntity("Hero");
    world.Add<CharacterMover>(hero);
    Walker& walker = world.AddBehaviour<Walker>(hero);
    const Entity gate = AddBox(world, "Gate", {6, 1, 0}, {0.5f, 1, 2}, false);
    world.Get<Collider>(gate)->isTrigger = true;
    ContactRecorder& gateContacts = world.AddBehaviour<ContactRecorder>(gate);
    Run(world, 1.0f);
    const CharacterMover* mover = world.Get<CharacterMover>(hero);
    ASTRAL_CHECK(mover->Grounded());
    ASTRAL_CHECK_NEAR(world.GetWorld(hero).translation.y, 0.0f, 0.03f);
    ASTRAL_CHECK(world.GetWorld(hero).translation.x > 2.5f);
    // Jump and land.
    walker.jumpNext = true;
    float peak = 0.0f;
    for (int i = 0; i < 90; ++i) {
        world.Tick(1.0f / 60.0f);
        peak = std::max(peak, world.GetWorld(hero).translation.y);
    }
    ASTRAL_CHECK(peak > 1.0f);
    ASTRAL_CHECK(mover->Grounded());
    ASTRAL_CHECK(gateContacts.triggerEnters.size() == 1 && gateContacts.triggerEnters[0].other == hero);
    // The character's capsule pushes dynamic bodies.
    const float crateX = world.GetWorld(hero).translation.x + 2.0f;
    const Entity crate = AddBox(world, "Crate", {crateX, 0.3f, 0}, {0.3f, 0.3f, 0.3f}, true, 1.0f);
    Run(world, 1.5f);
    ASTRAL_CHECK(world.GetWorld(crate).translation.x > crateX + 1.0f);
    // Teleporting the entity moves the controller.
    world.SetWorldPosition(hero, {-10, 0, 5});
    walker.velocity = {};
    Run(world, 0.2f);
    ASTRAL_CHECK(Near(world.GetWorld(hero).translation, {-10, 0, 5}, 0.05f));
}

// ------------------------------------------------------------------ rendering, audio, particles, animation

ASTRAL_TEST(RenderExtractionFollowsTheHierarchy) {
    GameWorld world;
    auto cube = MakePrimitiveMesh("cube");
    auto material = std::make_shared<Graphics::Material>();
    material->shading = Graphics::ShadingModel::Unlit;
    material->baseColor = {1, 0, 0};
    const Entity parent = world.CreateEntity("Parent", {{0, 0, 5}, Math::QuatFromAxisAngle({0, 1, 0}, Math::kHalfPi), {2, 2, 2}});
    MeshRenderer renderer;
    renderer.mesh = cube;
    renderer.material = material;
    renderer.objectId = 42;
    world.Add<MeshRenderer>(parent, renderer);
    const Entity child = world.CreateEntity("Child", {{1, 0, 0}, {}, {1, 1, 1}}, parent);
    renderer.objectId = 0;
    renderer.visible = true;
    world.Add<MeshRenderer>(child, renderer);
    const Entity hidden = world.CreateEntity("Hidden");
    renderer.visible = false;
    world.Add<MeshRenderer>(hidden, renderer);
    const Entity sun = world.CreateEntity("Sun", {{}, Math::QuatFromAxisAngle({1, 0, 0}, Math::kHalfPi), {1, 1, 1}});
    Light light;
    light.type = LightType::Directional;
    light.intensity = 3.0f;
    world.Add<Light>(sun, light);
    const Entity lamp = world.CreateEntity("Lamp", {{1, 2, 3}, {}, {1, 1, 1}});
    world.Add<Light>(lamp);
    Graphics::RenderScene scene;
    Graphics::RenderView view;
    ASTRAL_CHECK(!world.BuildRenderScene(scene, view, 64, 48)); // no camera yet
    const Entity eye = world.CreateEntity("Eye", {{0, 0, 0}, {}, {1, 1, 1}});
    world.Add<Camera>(eye);
    const Entity inactive = world.CreateEntity("Backup", {{0, 50, 0}, {}, {1, 1, 1}});
    Camera backup;
    backup.priority = 10;
    backup.active = false;
    world.Add<Camera>(inactive, backup);
    world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK(world.ActiveCamera() == eye);
    ASTRAL_CHECK(world.BuildRenderScene(scene, view, 64, 48));
    ASTRAL_CHECK(scene.draws.size() == 2);
    ASTRAL_CHECK(scene.draws[0].objectId == 42 && scene.draws[1].objectId == child.index + 1);
    // Child world = parent (rotated 90 degrees about Y, scale 2) * child offset (1, 0, 0) -> (0, 0, 3).
    const Vec3 childOrigin = Math::TransformPoint(scene.draws[1].world, {0, 0, 0});
    ASTRAL_CHECK(Near(childOrigin, {0, 0, 3}, 1e-4f));
    ASTRAL_CHECK(Near(scene.sun.direction, {0, -1, 0}, 1e-4f) && scene.sun.intensity == 3.0f);
    ASTRAL_CHECK(scene.pointLights.size() == 1 && Near(scene.pointLights[0].position, {1, 2, 3}, 1e-6f));
    ASTRAL_CHECK(Near(view.eye, {0, 0, 0}, 1e-6f) && view.width == 64);
    // The renderer sees the child cube (in front of its parent) in the middle of the frame.
    Graphics::RenderTarget target;
    target.Resize(64, 48);
    Graphics::SceneRenderer sceneRenderer;
    sceneRenderer.Render(scene, view, target);
    ASTRAL_CHECK(target.objectId[target.Index(32, 24)] == child.index + 1);
    // Cameras switch by priority when activated.
    world.Get<Camera>(inactive)->active = true;
    ASTRAL_CHECK(world.ActiveCamera() == inactive);
}

ASTRAL_TEST(AudioAndParticlesFollowTheirEntities) {
    Audio::AudioMixer mixer(48000, 8);
    GameWorld world({}, &mixer);
    const Entity speaker = world.CreateEntity("Speaker", {{10, 0, 0}, {}, {1, 1, 1}});
    AudioSource source;
    source.clip = std::make_shared<Audio::AudioClip>(Audio::Synth::Tone(440.0f, 0.5f));
    source.loop = true;
    world.Add<AudioSource>(speaker, source);
    world.Add<Camera>(world.CreateEntity("Ears"));
    ParticleSystem sparks;
    sparks.rate = 200.0f;
    sparks.speedMin = sparks.speedMax = 0.0f;
    sparks.lifetimeMin = sparks.lifetimeMax = 5.0f;
    world.Add<ParticleSystem>(speaker, sparks);
    world.Tick(1.0f / 60.0f);
    const Audio::VoiceId voice = world.Get<AudioSource>(speaker)->voice;
    ASTRAL_CHECK(!voice.IsNull() && mixer.IsPlaying(voice));
    std::vector<float> stereo(2 * 256);
    mixer.Mix(stereo.data(), 256); // stats are measured by the mix
    ASTRAL_CHECK(mixer.Stats().activeVoices == 1 && mixer.Stats().peak > 0.0f);
    Run(world, 0.2f);
    ASTRAL_CHECK(world.Particles().ParticleCount() > 20);
    // Emitters follow the entity; new particles spawn at its new position.
    world.SetWorldPosition(speaker, {-10, 0, 0});
    Run(world, 0.2f);
    Graphics::RenderScene scene;
    Graphics::RenderView view;
    ASTRAL_CHECK(world.BuildRenderScene(scene, view, 32, 32));
    const bool anyNearNew = std::any_of(scene.billboards.begin(), scene.billboards.end(),
        [](const Graphics::Billboard& b) { return Near(b.position, {-10, 0, 0}, 0.01f); });
    ASTRAL_CHECK(anyNearNew);
    // Destruction stops the voice and the emitter.
    world.Destroy(speaker);
    world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK(!mixer.IsPlaying(voice));
    mixer.Mix(stereo.data(), 256);
    ASTRAL_CHECK(mixer.Stats().activeVoices == 0);
    ASTRAL_CHECK(world.Particles().EmitterCount() == 1); // still fading out
    ASTRAL_CHECK(!world.PlaySoundAt(source.clip.get(), {1, 2, 3}).IsNull());
}

ASTRAL_TEST(AnimatorComponentSkinsAndPublishesNotifies) {
    auto skeleton = std::make_shared<Animation::Skeleton>();
    skeleton->AddJoint("root", -1, {});
    skeleton->AddJoint("arm", 0, {{0, 1, 0}, {}, {1, 1, 1}});
    skeleton->ComputeBindMatrices();
    auto clip = std::make_shared<Animation::AnimationClip>();
    clip->name = "Raise";
    clip->duration = 1.0f;
    clip->AddKey(1, 0.0f, {{0, 1, 0}, {}, {1, 1, 1}});
    clip->AddKey(1, 1.0f, {{0, 3, 0}, {}, {1, 1, 1}});
    clip->notifies.push_back({"Clang", 0.25f, 0.0f});
    auto machine = std::make_shared<Animation::AnimStateMachine>();
    Animation::AnimState state;
    state.name = "Raise";
    state.clip = clip.get();
    machine->AddState(state);
    auto bind = std::make_shared<Graphics::MeshData>();
    Graphics::MeshBuilder builder(*bind);
    builder.SetSkinJoint(1);
    builder.AddBox({0, 1, 0}, {0.1f, 0.1f, 0.1f});
    bind->ComputeBounds();

    GameWorld world;
    const Entity arm = world.CreateEntity("Arm");
    MeshRenderer renderer;
    renderer.mesh = bind;
    world.Add<MeshRenderer>(arm, renderer);
    AnimatorComponent animator;
    animator.skeleton = skeleton;
    animator.machine = machine;
    animator.bindMesh = bind;
    world.Add<AnimatorComponent>(arm, animator);
    int notifies = 0;
    auto subscription = world.Events().SubscribeScoped<AnimationNotification>([&](const AnimationNotification& n) {
        if (n.entity == arm && n.name == "Clang") ++notifies;
    });
    Run(world, 0.5f);
    ASTRAL_CHECK(notifies == 1);
    const MeshRenderer* skinned = world.Get<MeshRenderer>(arm);
    ASTRAL_CHECK(skinned->mesh != bind && skinned->mesh == world.Get<AnimatorComponent>(arm)->skinned);
    // Halfway through the clip the arm joint has risen by one metre.
    float averageY = 0.0f;
    for (const Graphics::Vertex& v : skinned->mesh->vertices) averageY += v.position.y;
    averageY /= static_cast<float>(skinned->mesh->vertices.size());
    ASTRAL_CHECK_NEAR(averageY, 2.0f, 0.1f);
}

// ------------------------------------------------------------------ scenes

namespace {
const char* kScene = R"({
  "format": "astral-scene", "version": 1,
  "settings": {"gravity": [0, -12, 0], "timeScale": 1},
  "environment": {"fogDensity": 0.002, "exposure": 1.2},
  "materials": {
    "stone": {"shading": "Toon", "baseColor": [0.6, 0.6, 0.65]},
    "wood": {"shading": "Lit", "baseColor": [0.5, 0.3, 0.1], "roughness": 0.9}
  },
  "prefabs": {
    "Crate": {
      "name": "Crate",
      "components": {
        "MeshRenderer": {"mesh": "primitive:cube", "material": "wood"},
        "Collider": {"shape": "Box", "halfExtents": [0.5, 0.5, 0.5]},
        "RigidBody": {"mass": 4}
      },
      "children": [{"name": "Lid", "transform": {"position": [0, 0.55, 0], "scale": [0.9, 0.1, 0.9]},
                    "components": {"MeshRenderer": {"mesh": "primitive:cube", "material": "wood"}}}]
    },
    "Tower": {"prefab": "Crate", "name": "Tower", "components": {"RigidBody": {"type": "Static"}}}
  },
  "entities": [
    {"name": "Floor", "transform": {"scale": [40, 1, 40]},
     "components": {"MeshRenderer": {"mesh": "primitive:plane", "material": "stone"}, "Collider": {"shape": "Mesh"}}},
    {"prefab": "Crate", "name": "Crate A", "transform": {"position": [0, 2, 0]}},
    {"prefab": "Crate", "name": "Crate B", "transform": {"position": [3, 2, 0], "rotation": {"euler": [0, 90, 0]}},
     "components": {"RigidBody": {"mass": 8}}},
    {"prefab": "Tower", "transform": {"position": [-3, 0.5, 0]}},
    {"name": "Sun", "transform": {"rotation": {"euler": [60, 30, 0]}},
     "components": {"Light": {"type": "Directional", "intensity": 2.5}}},
    {"name": "Camera", "transform": {"position": [0, 3, -10]}, "components": {"Camera": {"fieldOfView": 50}}},
    {"name": "Pivot", "transform": {"position": [0, 4, 4], "scale": 2},
     "behaviours": [{"type": "Spinner", "degreesPerSecond": 45, "axis": [0, 1, 0]},
                    {"type": "Recorder", "tag": "pivot", "enabled": false}],
     "components": {"ParticleSystem": {"rate": 30, "colorStart": [1, 0.8, 0.2, 1]}},
     "children": [{"name": "Orb", "transform": {"position": [1, 0, 0]},
                   "components": {"MeshRenderer": {"mesh": "primitive:sphere", "material": "stone"},
                                  "Light": {"color": [0.4, 0.6, 1.0], "radius": 6}}}]}
  ]
})";
} // namespace

ASTRAL_TEST(ScenesLoadPrefabsOverridesAndRoundTrip) {
    RegisterTestBehaviours();
    Log().clear();
    SceneSerializer serializer;
    SceneDocument document;
    std::string error;
    const bool parsed = serializer.Parse(ParseText(kScene), document, error);
    if (!parsed) std::fprintf(stderr, "%s\n", error.c_str());
    ASTRAL_CHECK(parsed);
    ASTRAL_CHECK(document.entities.size() == 7 && document.prefabs.size() == 2);
    GameWorld world;
    const std::vector<Entity> roots = serializer.Instantiate(document, world);
    ASTRAL_CHECK(roots.size() == 7);
    ASTRAL_CHECK_NEAR(world.Physics().Settings().gravity.y, -12.0f, 1e-6);
    ASTRAL_CHECK_NEAR(world.Env().exposure, 1.2f, 1e-6);
    const Entity crateA = world.Find("Crate A"), crateB = world.Find("Crate B"), tower = world.Find("Tower");
    ASTRAL_CHECK(!crateA.IsNull() && !crateB.IsNull() && !tower.IsNull());
    ASTRAL_CHECK(world.Get<RigidBody>(crateA)->mass == 4.0f && world.Get<RigidBody>(crateB)->mass == 8.0f);
    ASTRAL_CHECK(world.Get<RigidBody>(tower)->type == Physics::BodyType::Static && world.Get<RigidBody>(tower)->mass == 4.0f);
    ASTRAL_CHECK(World::GetChildren(world.Registry(), crateB).size() == 1);
    // Prefab instances share resources.
    ASTRAL_CHECK(world.Get<MeshRenderer>(crateA)->material == world.Get<MeshRenderer>(crateB)->material);
    ASTRAL_CHECK(world.Get<MeshRenderer>(crateA)->material->shading == Graphics::ShadingModel::Lit);
    ASTRAL_CHECK(world.Get<MeshRenderer>(crateA)->mesh == world.Get<MeshRenderer>(crateB)->mesh);
    const Quat yaw = world.GetLocal(crateB).rotation;
    ASTRAL_CHECK(Near(Math::Rotate(yaw, {0, 0, 1}), {1, 0, 0}, 1e-4f));
    const Entity pivot = world.Find("Pivot");
    ASTRAL_CHECK(world.GetLocal(pivot).scale.y == 2.0f);
    const Spinner* spinner = world.GetBehaviour<Spinner>(pivot);
    ASTRAL_CHECK(spinner && spinner->degreesPerSecond == 45.0f);
    const Recorder* recorder = world.GetBehaviour<Recorder>(pivot);
    ASTRAL_CHECK(recorder && !recorder->enabled && recorder->tag == "pivot");
    // Simulate: crates land on the scaled plane mesh collider; the pivot spins.
    Run(world, 2.0f);
    ASTRAL_CHECK_NEAR(world.GetWorld(crateA).translation.y, 0.5f, 0.05f);
    ASTRAL_CHECK_NEAR(world.GetWorld(crateB).translation.y, 0.5f, 0.05f);
    ASTRAL_CHECK_NEAR(world.GetWorld(tower).translation.y, 0.5f, 1e-5); // static
    const Vec3 orb = world.GetWorld(world.Find("Orb")).translation;
    ASTRAL_CHECK(Near(orb, {0, 4, 4}, 2.1f) && !Near(orb, {2, 4, 4}, 0.1f)); // rotated away from +X
    ASTRAL_CHECK(std::find(Log().begin(), Log().end(), "pivot:update") == Log().end()); // disabled
    Graphics::RenderScene scene;
    Graphics::RenderView view;
    ASTRAL_CHECK(world.BuildRenderScene(scene, view, 64, 64));
    ASTRAL_CHECK(scene.draws.size() == 8 && scene.pointLights.size() == 1 && scene.sun.intensity == 2.5f);

    // Save -> parse -> instantiate -> save is a fixed point.
    const std::string saved = Core::WriteJson(serializer.Save(world), {true, 2});
    SceneDocument reloaded;
    ASTRAL_CHECK(serializer.Parse(ParseText(saved), reloaded, error));
    GameWorld copy;
    serializer.Instantiate(reloaded, copy);
    ASTRAL_CHECK(Core::WriteJson(serializer.Save(copy), {true, 2}) == saved);
    ASTRAL_CHECK(copy.Registry().AliveCount() == world.Registry().AliveCount());

    // Runtime prefab spawning.
    const Entity spawned = serializer.InstantiatePrefab(document, "Crate", world, {{8, 3, 0}, {}, {1, 1, 1}});
    ASTRAL_CHECK(!spawned.IsNull() && world.Get<World::Name>(spawned)->value == "Crate");
    ASTRAL_CHECK(serializer.InstantiatePrefab(document, "Missing", world, {}).IsNull());
    Run(world, 2.0f);
    ASTRAL_CHECK_NEAR(world.GetWorld(spawned).translation.y, 0.5f, 0.05f);
}

ASTRAL_TEST(SceneErrorsNameTheirPathAndChangeNothing) {
    RegisterTestBehaviours();
    SceneSerializer serializer;
    struct Case {
        const char* json;
        const char* expected;
    };
    const Case cases[] = {
        {R"({"entities": [{"components": {"Rigidbody": {}}}]})", "entities[0].components.Rigidbody: unknown component"},
        {R"({"entities": [{"components": {"RigidBody": {"mass": "heavy"}}}]})", "entities[0].components.RigidBody: field 'mass'"},
        {R"({"entities": [{}, {"children": [{"components": {"Light": {"type": "Spot"}}}]}]})",
            "entities[1].children[0].components.Light"},
        {R"({"entities": [{"prefab": "Ghost"}]})", "unknown prefab 'Ghost'"},
        {R"({"prefabs": {"A": {"prefab": "B"}, "B": {"prefab": "A"}}})", "contains itself"},
        {R"({"entities": [{"behaviours": [{"type": "Teleporter"}]}]})", "unknown behaviour type 'Teleporter'"},
        {R"({"entities": [{"behaviours": [{"type": "Spinner", "rpm": 3}]}]})", "entities[0].behaviours[0]"},
        {R"({"entities": [{"components": {"MeshRenderer": {"mesh": "primitive:teapot"}}}]})", "unknown primitive"},
        {R"({"entities": [{"components": {"MeshRenderer": {"mesh": "primitive:cube", "material": "gold"}}}]})",
            "unknown material 'gold'"},
        {R"({"entities": [{"model": "hero.glb"}]})", "needs an asset manager"},
        {R"({"entities": [{"components": {"Collider": {"shape": "Mesh"}}}]})", "a Mesh collider needs a mesh"},
        {R"({"entities": [{"transform": {"rotation": [0, 0, 0, 0]}}]})", "entities[0].transform: field 'rotation'"},
        {R"({"version": 9})", "unsupported version"},
        {R"({"entitys": []})", "entitys: unknown key"},
        {R"({"settings": {"fixedDelta": 0}})", "settings.fixedDelta"},
        {R"({"materials": {"a#b": {}}})", "material names"},
        {R"({"entities": [{"name": "x", "collider": "mesh"}]})", "only applies to \"model\""},
    };
    GameWorld world;
    world.CreateEntity("Existing");
    for (const Case& test : cases) {
        SceneDocument document;
        std::string error;
        ASTRAL_CHECK(!serializer.Parse(ParseText(test.json), document, error));
        if (error.find(test.expected) == std::string::npos) std::fprintf(stderr, "unexpected error: %s\n", error.c_str());
        ASTRAL_CHECK(error.find(test.expected) != std::string::npos);
    }
    ASTRAL_CHECK(world.Registry().AliveCount() == 1);
    // Depth and entity limits.
    std::string deep = R"({"entities": [)";
    for (int i = 0; i < 80; ++i) deep += R"({"children": [)";
    deep += "{}";
    for (int i = 0; i < 80; ++i) deep += "]}";
    deep += "]}";
    SceneDocument document;
    std::string error;
    ASTRAL_CHECK(!serializer.Parse(ParseText(deep), document, error) && error.find("deeper") != std::string::npos);
    SceneSerializer limited;
    limited.maxEntities = 3;
    ASTRAL_CHECK(!limited.Parse(ParseText(R"({"entities": [{}, {}, {}, {}]})"), document, error));
}

ASTRAL_TEST(ModelsAndPrefabFilesLoadThroughTheAssetManager) {
    const std::string root = TempDir("astral_framework_assets");
    // A two-node glTF: a base with two primitives (two materials) and a raised child.
    Assets::GltfDocument model;
    model.materials.resize(2);
    model.materials[0].name = "Base";
    model.materials[0].baseColorFactor = {0.2f, 0.4f, 0.8f, 1.0f};
    model.materials[1].name = "Trim";
    model.materials[1].unlit = true;
    Assets::GltfMesh baseMesh;
    baseMesh.name = "Base";
    for (int p = 0; p < 2; ++p) {
        Assets::GltfPrimitive primitive;
        Graphics::MeshBuilder builder(primitive.mesh);
        builder.AddBox({0, p == 0 ? 0.5f : 1.1f, 0}, {1.0f, p == 0 ? 0.5f : 0.1f, 1.0f});
        primitive.mesh.ComputeBounds();
        primitive.material = p;
        baseMesh.primitives.push_back(primitive);
    }
    model.meshes.push_back(baseMesh);
    Assets::GltfNode base;
    base.name = "Base";
    base.mesh = 0;
    base.children = {1};
    Assets::GltfNode top;
    top.name = "Top";
    top.mesh = 0;
    top.parent = 0;
    top.local.translation = {0, 2, 0};
    model.nodes = {base, top};
    model.scenes.push_back({"Scene", {0}});
    model.scene = 0;
    std::string error;
    ASTRAL_CHECK(Assets::ExportGltf(model, root + "/tower.glb", Assets::GltfContainer::Binary, error));
    WriteText(root + "/prefabs/barrel.json", R"({
      "materials": {"steel": {"shading": "Lit", "metallic": 1}},
      "entity": {"name": "Barrel", "components": {
          "MeshRenderer": {"mesh": "primitive:cylinder", "material": "steel"},
          "Collider": {"shape": "Capsule", "radius": 0.5, "height": 2},
          "RigidBody": {"mass": 3, "lockRotation": true}}}})");
    std::vector<float> tone(4800, 0.25f);
    std::vector<float> stereo;
    for (float s : tone) {
        stereo.push_back(s);
        stereo.push_back(s);
    }
    ASTRAL_CHECK(Audio::WriteWav(root + "/hum.wav", stereo, 48000, error));

    Assets::AssetManager assets(root, nullptr);
    assets.RegisterDefaultLoaders();
    SceneSerializer serializer(&assets);
    const char* sceneText = R"({
      "entities": [
        {"model": "tower.glb", "collider": "mesh", "transform": {"position": [0, 0, 10]}},
        {"prefab": "prefabs/barrel.json", "transform": {"position": [0, 6, 10]}},
        {"name": "Hum", "components": {"AudioSource": {"clip": "hum.wav", "loop": true}}}
      ]})";
    SceneDocument document;
    const bool parsed = serializer.Parse(ParseText(sceneText), document, error);
    if (!parsed) std::fprintf(stderr, "%s\n", error.c_str());
    ASTRAL_CHECK(parsed);
    GameWorld world;
    serializer.Instantiate(document, world);
    const Entity tower = world.Find("tower");
    ASTRAL_CHECK(!tower.IsNull() && World::GetChildren(world.Registry(), tower).size() == 1);
    const Entity baseEntity = world.Find("Base"), topEntity = world.Find("Top");
    ASTRAL_CHECK(!baseEntity.IsNull() && !topEntity.IsNull() && World::GetParent(world.Registry(), topEntity) == baseEntity);
    ASTRAL_CHECK(!world.Find("Base.primitive1").IsNull());
    ASTRAL_CHECK(world.Get<MeshRenderer>(baseEntity)->material->shading == Graphics::ShadingModel::Toon);
    ASTRAL_CHECK(world.Get<MeshRenderer>(world.Find("Base.primitive1"))->material->shading == Graphics::ShadingModel::Unlit);
    ASTRAL_CHECK(world.Get<AudioSource>(world.Find("Hum"))->clip->samples.size() == 4800);
    // The barrel lands on the model's mesh colliders: the Top node sits at y = 2 and
    // its trim primitive spans 1.0..1.2 above it, so the top surface is y = 3.2.
    Run(world, 3.0f);
    const Entity barrel = world.Find("Barrel");
    ASTRAL_CHECK(!barrel.IsNull() && world.Get<MeshRenderer>(barrel)->material->metallic == 1.0f);
    ASTRAL_CHECK_NEAR(world.GetWorld(barrel).translation.y, 3.2f + 1.0f, 0.05f); // capsule centre, 1 m above the top
    EntityRaycastHit hit;
    ASTRAL_CHECK(world.Raycast({{0.5f, 20, 10.5f}, {0, -1, 0}}, 40.0f, hit, 0xFFFFFFFFu, barrel));
    ASTRAL_CHECK(hit.entity == topEntity || hit.entity == world.Find("Top.primitive1"));
    ASTRAL_CHECK_NEAR(hit.point.y, 3.2f, 1e-3);

    // Saved scenes keep asset references and reload identically.
    const std::string saved = Core::WriteJson(serializer.Save(world), {true, 2});
    ASTRAL_CHECK(saved.find("tower.glb#0.1") != std::string::npos && saved.find("tower.glb#material1") != std::string::npos);
    SceneDocument reloaded;
    ASTRAL_CHECK(serializer.Parse(ParseText(saved), reloaded, error));
    GameWorld copy;
    serializer.Instantiate(reloaded, copy);
    ASTRAL_CHECK(Core::WriteJson(serializer.Save(copy), {true, 2}) == saved);

    // Scene files parse through the asset manager too; missing files report their path.
    WriteText(root + "/level.json", sceneText);
    ASTRAL_CHECK(serializer.ParseFile("level.json", document, error));
    ASTRAL_CHECK(!serializer.ParseFile("missing.json", document, error) && error.find("missing.json") != std::string::npos);
    ASTRAL_CHECK(!serializer.Parse(ParseText(R"({"entities": [{"model": "nope.glb"}]})"), document, error));
    ASTRAL_CHECK(error.find("nope.glb") != std::string::npos);
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(IdenticalWorldsStayIdentical) {
    RegisterTestBehaviours();
    SceneSerializer serializer;
    SceneDocument document;
    std::string error;
    ASTRAL_CHECK(serializer.Parse(ParseText(kScene), document, error));
    GameWorld a, b;
    serializer.Instantiate(document, a);
    serializer.Instantiate(document, b);
    const float frames[] = {1.0f / 60.0f, 1.0f / 30.0f, 1.0f / 144.0f, 0.05f};
    for (int i = 0; i < 240; ++i) {
        a.Tick(frames[i % 4]);
        b.Tick(frames[i % 4]);
    }
    ASTRAL_CHECK(Core::WriteJson(serializer.Save(a)) == Core::WriteJson(serializer.Save(b)));
}

ASTRAL_TEST_MAIN("EngineFrameworkTests")
