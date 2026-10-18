#include "Engine/Animation/Skinning.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/Gltf.h"
#include "Engine/Audio/AudioMixer.h"
#include "Engine/Core/Json.h"
#include "Engine/Framework/GameWorld.h"
#include "Engine/Framework/Scene.h"
#include "Engine/Graphics/SceneRenderer.h"
#include "Engine/World/Components.h"
#include "Tests/EngineTestSupport.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
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

ASTRAL_TEST(RenderExtractionPicksLevelsOfDetail) {
    GameWorld world;
    const Entity eye = world.CreateEntity("Eye");
    world.Add<Camera>(eye);
    const Entity rock = world.CreateEntity("Rock", {{0, 0, 3}, {}, {1, 1, 1}});
    auto sphere = std::make_shared<Graphics::MeshData>();
    Graphics::MeshBuilder(*sphere).AddSphere({0, 0, 0}, 1.0f, 24, 32);
    sphere->ComputeBounds();
    MeshRenderer renderer;
    renderer.mesh = sphere;
    renderer.lods = std::make_shared<const Graphics::LodGroup>(Graphics::BuildLodGroup(sphere));
    world.Add<MeshRenderer>(rock, renderer);
    const auto& levels = renderer.lods->levels;
    auto drawnMesh = [&] {
        Graphics::RenderScene scene;
        Graphics::RenderView view;
        ASTRAL_CHECK(world.BuildRenderScene(scene, view, 64, 64));
        ASTRAL_CHECK(scene.draws.size() == 1);
        return scene.draws[0].mesh;
    };
    world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK(drawnMesh() == levels[0].mesh.get()); // close: full detail
    world.SetWorldPosition(rock, {0, 0, 400});
    world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK(drawnMesh() == levels.back().mesh.get()); // far: coarsest
    world.Env().lodBias = 1000.0f; // bias keeps detail
    ASTRAL_CHECK(drawnMesh() == levels[0].mesh.get());
    world.Env().lodBias = 1.0f;
    world.Get<MeshRenderer>(rock)->forcedLod = 1;
    ASTRAL_CHECK(drawnMesh() == levels[1].mesh.get());
    // Scenes generate one shared LOD chain per mesh with autoLod.
    SceneSerializer serializer;
    SceneDocument document;
    std::string error;
    const bool parsed = serializer.Parse(ParseText(R"({"entities": [
        {"name": "A", "components": {"MeshRenderer": {"mesh": "primitive:sphere", "autoLod": true}}},
        {"name": "B", "components": {"MeshRenderer": {"mesh": "primitive:sphere", "autoLod": true}}},
        {"name": "C", "components": {"MeshRenderer": {"mesh": "primitive:sphere"}}}]})"), document, error);
    ASTRAL_CHECK(parsed && document.lodGroups.size() == 1);
    GameWorld loaded;
    serializer.Instantiate(document, loaded);
    const MeshRenderer* a = loaded.Get<MeshRenderer>(loaded.Find("A"));
    const MeshRenderer* b = loaded.Get<MeshRenderer>(loaded.Find("B"));
    ASTRAL_CHECK(a->lods && a->lods == b->lods && a->lods->Count() > 1);
    ASTRAL_CHECK(!loaded.Get<MeshRenderer>(loaded.Find("C"))->lods);
}

ASTRAL_TEST(SceneDependencyCollectorRetainsFailedNestedCandidate) {
    const std::string root = TempDir("astral_scene_dependency_collector");
    std::string error;
    Graphics::ImageRgba8 image;
    image.Resize(2, 2, {255, 255, 255, 255});
    ASTRAL_CHECK(Graphics::WritePng(root + "/material.png", image, error));
    WriteText(root + "/outer.json", R"({"materials":{"M":{"texture":"material.png"}},
        "entity":{"prefab":"inner.json","components":{"MeshRenderer":{"mesh":"primitive:cube","material":"M"}}}})");
    Assets::AssetManager assets(root, nullptr);
    assets.RegisterDefaultLoaders();
    SceneSerializer serializer(&assets);
    SceneDocument document;
    ASTRAL_CHECK(serializer.Parse(ParseText(R"({"entities":[{"name":"LastGood"}]})"), document, error));
    std::vector<Assets::AssetDependency> attempted;
    const auto json = ParseText(R"({"entities":[{"prefab":"outer.json"}]})");
    ASTRAL_CHECK(!serializer.Parse(json, document, error, &attempted, true));
    ASTRAL_CHECK(error.find("inner.json") != std::string::npos && attempted.size() == 3);
    ASTRAL_CHECK(document.entities.size() == 1 && document.entities[0].name == "LastGood");
    assets.Collect(); // failed watch identities keep the missing slot alive
    WriteText(root + "/inner.json", R"({"entity":{"name":"Recovered"}})");
    ASTRAL_CHECK(assets.PollChanges() == 1);
    ASTRAL_CHECK(std::any_of(attempted.begin(), attempted.end(), [](const auto& watch) { return watch.Changed(); }));
    attempted.clear();
    ASTRAL_CHECK(serializer.Parse(json, document, error, &attempted, true));
    ASTRAL_CHECK(document.entities[0].name == "Recovered" && document.dependencies.size() == 3);
    assets.Load<Core::JsonValue>("unrelated.json");
    attempted.clear();
    ASTRAL_CHECK(serializer.Parse(json, document, error, &attempted, true));
    ASTRAL_CHECK(attempted.size() == 3); // unrelated cache errors are not dependencies
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(SceneStrictReloadRejectsNewDegradedImageWithoutChangingInitialImport) {
    const std::string root = TempDir("astral_scene_strict_image_reload");
    std::string error;
    WriteText(root + "/model.gltf", R"({"asset":{"version":"2.0"},"images":[{"uri":"later.png"}],
        "textures":[{"source":0}],"materials":[{"pbrMetallicRoughness":{"baseColorTexture":{"index":0}}}]})");
    Assets::AssetManager assets(root, nullptr);
    assets.RegisterDefaultLoaders();
    SceneSerializer serializer(&assets);
    SceneDocument document;
    const auto json = ParseText(R"({"entities":[{"name":"Surface","components":{
        "MeshRenderer":{"mesh":"primitive:cube","material":"model.gltf#material0"}}}]})");
    ASTRAL_CHECK(serializer.Parse(json, document, error));
    ASTRAL_CHECK(document.entities[0].name == "Surface");
    std::vector<Assets::AssetDependency> attempted;
    ASTRAL_CHECK(!serializer.Parse(json, document, error, &attempted, true));
    ASTRAL_CHECK(document.entities[0].name == "Surface" && attempted.size() == 1 && error.find("image 0") != std::string::npos);
    Graphics::ImageRgba8 image;
    image.Resize(2, 2, {255, 255, 255, 255});
    ASTRAL_CHECK(Graphics::WritePng(root + "/later.png", image, error));
    ASTRAL_CHECK(assets.PollChanges() == 1);
    attempted.clear();
    ASTRAL_CHECK(serializer.Parse(json, document, error, &attempted, true));
    ASTRAL_CHECK(document.materials.at("model.gltf#material0")->baseTexture != nullptr);
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(PrefabCopiesRespectExpandedEntityBudget) {
    SceneSerializer serializer;
    serializer.maxEntities = 7;
    const auto json = ParseText(R"({"prefabs":{"Tree":{"name":"Tree","children":[{},{}]}},
        "entities":[{"prefab":"Tree"},{"prefab":"Tree"},{"prefab":"Tree"}]})");
    SceneDocument document;
    document.entities.emplace_back();
    document.entities.front().name = "Retained";
    std::string error;
    const bool parsed = serializer.Parse(json, document, error);
    if (parsed) {
        GameWorld world;
        serializer.Instantiate(document, world);
        std::printf("expanded copies: runtime entities=%zu, configured limit=%zu\n", world.Registry().AliveCount(), serializer.maxEntities);
        std::fflush(stdout);
    }
    ASTRAL_CHECK(!parsed);
    ASTRAL_CHECK(error.find("entities[2]") != std::string::npos && error.find("7") != std::string::npos);
    ASTRAL_CHECK(document.entities.size() == 1 && document.entities.front().name == "Retained");
}

ASTRAL_TEST(PrefabCopiesComposeInsertionDepth) {
    SceneSerializer serializer;
    serializer.maxDepth = 1;
    const auto json = ParseText(R"({"prefabs":{"Branch":{"children":[{}]}},
        "entities":[{"children":[{"prefab":"Branch"}]}]})");
    SceneDocument document;
    document.entities.emplace_back();
    document.entities.front().name = "Retained";
    std::string error;
    const bool parsed = serializer.Parse(json, document, error);
    if (parsed) {
        GameWorld world;
        serializer.Instantiate(document, world);
        std::printf("expanded depth: accepted three-level runtime hierarchy with maxDepth=%zu, entities=%zu\n",
            serializer.maxDepth, world.Registry().AliveCount());
        std::fflush(stdout);
    }
    ASTRAL_CHECK(!parsed);
    ASTRAL_CHECK(error.find("entities[0].children[0]") != std::string::npos && error.find("deeper") != std::string::npos);
    ASTRAL_CHECK(document.entities.size() == 1 && document.entities.front().name == "Retained");
}

ASTRAL_TEST(AcyclicPrefabDoublingRespectsExpandedCacheBudget) {
    JsonValue prefabs = JsonValue::MakeObject();
    prefabs.Set("P0", JsonValue::MakeObject());
    for (int i = 1; i != 9; ++i) {
        JsonValue definition = JsonValue::MakeObject(), children = JsonValue::MakeArray();
        JsonValue child = JsonValue::MakeObject();
        child.Set("prefab", "P" + std::to_string(i - 1));
        children.Append(child);
        children.Append(child);
        definition.Set("children", children);
        prefabs.Set("P" + std::to_string(i), definition);
    }
    JsonValue json = JsonValue::MakeObject(), entities = JsonValue::MakeArray(), instance = JsonValue::MakeObject();
    instance.Set("prefab", "P8");
    entities.Append(instance);
    json.Set("prefabs", prefabs);
    json.Set("entities", entities);
    SceneSerializer serializer;
    serializer.maxEntities = 64;
    SceneDocument document;
    document.entities.emplace_back();
    document.entities.front().name = "Retained";
    std::string error;
    const bool parsed = serializer.Parse(json, document, error);
    if (parsed) {
        GameWorld world;
        serializer.Instantiate(document, world);
        std::printf("acyclic doubling: definitions=%zu, runtime entities=%zu, configured limit=%zu\n",
            document.prefabs.size(), world.Registry().AliveCount(), serializer.maxEntities);
        std::fflush(stdout);
    }
    ASTRAL_CHECK(!parsed);
    ASTRAL_CHECK(error.find("prefabs.") != std::string::npos && error.find("64") != std::string::npos);
    ASTRAL_CHECK(document.entities.size() == 1 && document.entities.front().name == "Retained");
}

ASTRAL_TEST(PrefabBudgetsAcceptSeparateExactZeroAndOneLimits) {
    SceneSerializer serializer;
    serializer.maxEntities = serializer.maxDepth = 0;
    SceneDocument document;
    std::string error;
    ASTRAL_CHECK(serializer.Parse(ParseText("{}"), document, error));
    ASTRAL_CHECK(!serializer.Parse(ParseText(R"({"entities":[{}]})"), document, error));
    ASTRAL_CHECK(!serializer.Parse(ParseText(R"({"prefabs":{"Unit":{}}})"), document, error));
    ASTRAL_CHECK(document.entities.empty() && document.prefabs.empty());

    serializer.maxEntities = 1;
    const auto one = ParseText(R"({"prefabs":{"Unit":{"name":"Unit"}},"entities":[{"prefab":"Unit"}]})");
    ASTRAL_CHECK(serializer.Parse(one, document, error)); // one cache node and one output node use independent limits
    GameWorld single;
    serializer.Instantiate(document, single);
    ASTRAL_CHECK(single.Registry().AliveCount() == 1 && document.prefabs.size() == 1);
    ASTRAL_CHECK(!serializer.Parse(ParseText(R"({"prefabs":{"A":{},"B":{}}})"), document, error));
    ASTRAL_CHECK(error.find("cached prefab") != std::string::npos && document.entities.front().name == "Unit");

    serializer.maxEntities = 7;
    serializer.maxDepth = 1;
    ASTRAL_CHECK(serializer.Parse(ParseText(R"({"prefabs":{"Tree":{"children":[{},{}]}},
        "entities":[{"prefab":"Tree"},{"prefab":"Tree"},{}]})"), document, error));
    GameWorld exact;
    serializer.Instantiate(document, exact);
    ASTRAL_CHECK(exact.Registry().AliveCount() == 7 && document.prefabs.size() == 1);

    serializer.maxEntities = serializer.maxDepth = std::numeric_limits<std::size_t>::max();
    ASTRAL_CHECK(serializer.Parse(one, document, error)); // huge limits must not wrap during admission
    ASTRAL_CHECK(document.entities.size() == 1 && document.entities.front().name == "Unit");
}

ASTRAL_TEST(PrefabResolutionIndirectionHasAnIndependentDepthGuard) {
    const auto aliases = ParseText(R"({"prefabs":{"A":{"prefab":"B"},"B":{"prefab":"C"},"C":{"name":"Leaf"}},
        "entities":[{"prefab":"A"}]})");
    SceneSerializer serializer;
    serializer.maxEntities = 3;
    serializer.maxDepth = 1;
    SceneDocument document;
    document.entities.emplace_back();
    document.entities.front().name = "Retained";
    std::string error;
    ASTRAL_CHECK(!serializer.Parse(aliases, document, error));
    ASTRAL_CHECK(error.find("prefab resolution deeper than 1") != std::string::npos);
    ASTRAL_CHECK(document.entities.front().name == "Retained");
    serializer.maxDepth = 2;
    ASTRAL_CHECK(serializer.Parse(aliases, document, error)); // three cached alias roots, one flat output root
    ASTRAL_CHECK(document.prefabs.size() == 3 && document.entities.front().children.empty());
    ASTRAL_CHECK(document.entities.front().name == "Leaf");
    ASTRAL_CHECK(!serializer.Parse(ParseText(R"({"prefabs":{"A":{"prefab":"A"}}})"), document, error));
    ASTRAL_CHECK(error.find("contains itself") != std::string::npos && document.entities.front().name == "Leaf");
}

ASTRAL_TEST(ModelBudgetCountsGeneratedPrimitivesAndComposesDepth) {
    const std::string root = TempDir("astral_framework_model_budget");
    Assets::GltfDocument model;
    Assets::GltfMesh mesh;
    for (int i = 0; i != 3; ++i) {
        Assets::GltfPrimitive primitive;
        Graphics::MeshBuilder(primitive.mesh).AddBox({static_cast<float>(i), 0, 0}, {0.5f, 0.5f, 0.5f});
        primitive.mesh.ComputeBounds();
        mesh.primitives.push_back(std::move(primitive));
    }
    model.meshes.push_back(std::move(mesh));
    Assets::GltfNode node;
    node.name = "ModelNode";
    node.mesh = 0;
    model.nodes.push_back(node);
    model.scenes.push_back({"Budget", {0}});
    model.scene = 0;
    std::string error;
    ASTRAL_CHECK(Assets::ExportGltf(model, root + "/budget.glb", Assets::GltfContainer::Binary, error));
    Assets::AssetManager assets(root, nullptr);
    assets.RegisterDefaultLoaders();
    SceneSerializer serializer(&assets);
    serializer.maxEntities = 3;
    const auto json = ParseText(R"({"entities":[{"model":"budget.glb"}]})");
    SceneDocument document;
    document.entities.emplace_back();
    document.entities.front().name = "Retained";
    std::vector<Assets::AssetDependency> attempted;
    ASTRAL_CHECK(!serializer.Parse(json, document, error, &attempted)); // wrapper + node + two primitive children = four
    ASTRAL_CHECK(attempted.size() == 1 && document.entities.front().name == "Retained");
    serializer.maxEntities = 4;
    serializer.maxDepth = 2;
    ASTRAL_CHECK(serializer.Parse(json, document, error));
    GameWorld exact;
    serializer.Instantiate(document, exact);
    ASTRAL_CHECK(exact.Registry().AliveCount() == 4);
    serializer.maxDepth = 1;
    ASTRAL_CHECK(!serializer.Parse(json, document, error) && error.find("deeper") != std::string::npos);
    ASTRAL_CHECK(document.entities.front().children.front().name == "ModelNode");

    serializer.maxEntities = 5;
    serializer.maxDepth = 2;
    const auto nested = ParseText(R"({"entities":[{"children":[{"model":"budget.glb"}]}]})");
    ASTRAL_CHECK(!serializer.Parse(nested, document, error) && error.find("entities[0].children[0].model") != std::string::npos);
    serializer.maxDepth = 3;
    ASTRAL_CHECK(serializer.Parse(nested, document, error));
    GameWorld nestedWorld;
    serializer.Instantiate(document, nestedWorld);
    ASTRAL_CHECK(nestedWorld.Registry().AliveCount() == 5);

    serializer.maxEntities = 4;
    serializer.maxDepth = 2;
    const auto prefab = ParseText(R"({"prefabs":{"Model":{"model":"budget.glb"}},"entities":[{"prefab":"Model"}]})");
    ASTRAL_CHECK(serializer.Parse(prefab, document, error)); // model-derived cache extent includes both primitive children
    serializer.maxEntities = 3;
    ASTRAL_CHECK(!serializer.Parse(prefab, document, error) && error.find("cached prefab") != std::string::npos);
    ASTRAL_CHECK(document.prefabs.size() == 1 && document.entities.front().children.front().children.size() == 2);
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(PrefabBudgetFailureRetainsNestedAttemptedWatchesAndOutput) {
    const std::string root = TempDir("astral_framework_prefab_budget_watches");
    WriteText(root + "/outer.json", R"({"entity":{"prefab":"inner.json","children":[{}]}})");
    WriteText(root + "/inner.json", R"({"entity":{"name":"Inner","children":[{}]}})");
    Assets::AssetManager assets(root, nullptr);
    assets.RegisterDefaultLoaders();
    SceneSerializer serializer(&assets);
    serializer.maxEntities = 4;
    const auto json = ParseText(R"({"entities":[{"prefab":"outer.json"}]})");
    SceneDocument document;
    document.entities.emplace_back();
    document.entities.front().name = "Retained";
    std::vector<Assets::AssetDependency> attempted;
    std::string error;
    ASTRAL_CHECK(!serializer.Parse(json, document, error, &attempted, true)); // cache would retain two + three nodes
    ASTRAL_CHECK(attempted.size() == 2 && error.find("cached prefab") != std::string::npos);
    ASTRAL_CHECK(document.entities.size() == 1 && document.entities.front().name == "Retained");
    assets.Collect();
    WriteText(root + "/inner.json", R"({"entity":{"name":"Inner"}})");
    ASTRAL_CHECK(assets.PollChanges() == 1);
    ASTRAL_CHECK(std::any_of(attempted.begin(), attempted.end(), [](const auto& watch) { return watch.Changed(); }));
    attempted.clear();
    ASTRAL_CHECK(serializer.Parse(json, document, error, &attempted, true));
    ASTRAL_CHECK(document.dependencies.size() == 2 && document.entities.front().name == "Inner");
    GameWorld world;
    serializer.Instantiate(document, world);
    ASTRAL_CHECK(world.Registry().AliveCount() == 2);
    std::filesystem::remove_all(root);
}

namespace {

// Original verification fixture. File coordinates are glTF right-handed;
// expected coordinates below are literal engine-space values, not values
// generated with BuildSkeleton/BuildClip or the future scene-binding helper.
const char* kImportedAnimationGltf = R"({
"asset":{"version":"2.0","generator":"Astral original two-joint verification fixture"},
"extensionsUsed":["KHR_materials_unlit"],
"buffers":[{"uri":"TwoJointRibbon.bin","byteLength":492}],
"bufferViews":[
 {"buffer":0,"byteOffset":0,"byteLength":36,"target":34962},
 {"buffer":0,"byteOffset":36,"byteLength":36,"target":34962},
 {"buffer":0,"byteOffset":72,"byteLength":12,"target":34962},
 {"buffer":0,"byteOffset":84,"byteLength":48,"target":34962},
 {"buffer":0,"byteOffset":132,"byteLength":6,"target":34963},
 {"buffer":0,"byteOffset":140,"byteLength":36,"target":34962},
 {"buffer":0,"byteOffset":176,"byteLength":36,"target":34962},
 {"buffer":0,"byteOffset":212,"byteLength":12,"target":34962},
 {"buffer":0,"byteOffset":224,"byteLength":48,"target":34962},
 {"buffer":0,"byteOffset":272,"byteLength":6,"target":34963},
 {"buffer":0,"byteOffset":280,"byteLength":128},
 {"buffer":0,"byteOffset":408,"byteLength":12},
 {"buffer":0,"byteOffset":420,"byteLength":36},
 {"buffer":0,"byteOffset":456,"byteLength":36}],
"accessors":[
 {"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[-2,0,0],"max":[0,1,0]},
 {"bufferView":1,"componentType":5126,"count":3,"type":"VEC3"},
 {"bufferView":2,"componentType":5121,"count":3,"type":"VEC4"},
 {"bufferView":3,"componentType":5126,"count":3,"type":"VEC4"},
 {"bufferView":4,"componentType":5123,"count":3,"type":"SCALAR"},
 {"bufferView":5,"componentType":5126,"count":3,"type":"VEC3","min":[-2,1,0],"max":[0,2,0]},
 {"bufferView":6,"componentType":5126,"count":3,"type":"VEC3"},
 {"bufferView":7,"componentType":5121,"count":3,"type":"VEC4"},
 {"bufferView":8,"componentType":5126,"count":3,"type":"VEC4"},
 {"bufferView":9,"componentType":5123,"count":3,"type":"SCALAR"},
 {"bufferView":10,"componentType":5126,"count":2,"type":"MAT4"},
 {"bufferView":11,"componentType":5126,"count":3,"type":"SCALAR","min":[0],"max":[1]},
 {"bufferView":12,"componentType":5126,"count":3,"type":"VEC3"},
 {"bufferView":13,"componentType":5126,"count":3,"type":"VEC3"}],
"materials":[
 {"name":"Blue","pbrMetallicRoughness":{"baseColorFactor":[0.1,0.4,0.9,1]},"extensions":{"KHR_materials_unlit":{}}},
 {"name":"Orange","pbrMetallicRoughness":{"baseColorFactor":[1,0.4,0.1,1]},"extensions":{"KHR_materials_unlit":{}}}],
"meshes":[
 {"name":"Ribbon","primitives":[
   {"attributes":{"POSITION":0,"NORMAL":1,"JOINTS_0":2,"WEIGHTS_0":3},"indices":4,"material":0},
   {"attributes":{"POSITION":5,"NORMAL":6,"JOINTS_0":7,"WEIGHTS_0":8},"indices":9,"material":1}]},
 {"name":"Rigid","primitives":[{"attributes":{"POSITION":0,"NORMAL":1},"indices":4,"material":0}]}],
"nodes":[
 {"name":"RigSpace","translation":[0,3,0],"children":[1,3,4,5]},
 {"name":"RootJoint","children":[2]},
 {"name":"TipJoint","translation":[0,1,0]},
 {"name":"BodyOne","mesh":0,"skin":0,"translation":[-2,0,0],"rotation":[0,0,-0.7071067811865476,0.7071067811865476]},
 {"name":"BodyTwo","mesh":0,"skin":0,"translation":[-5,0,0]},
 {"name":"RigidControl","mesh":1,"translation":[-8,0,0]}],
"skins":[{"name":"ReverseSlots","joints":[2,1],"inverseBindMatrices":10}],
"animations":[
 {"name":"Raise","samplers":[{"input":11,"output":12,"interpolation":"LINEAR"}],"channels":[{"sampler":0,"target":{"node":2,"path":"translation"}}]},
 {"name":"Bend","samplers":[{"input":11,"output":13,"interpolation":"LINEAR"}],"channels":[{"sampler":0,"target":{"node":2,"path":"translation"}}]}],
"scenes":[{"name":"Verification","nodes":[0]}],"scene":0
})";

const char* kImportedAnimationScene = R"({"format":"astral-scene","version":1,
"settings":{"gravity":[0,0,0]},"entities":[
 {"name":"InstanceA","model":"TwoJointRibbon.gltf","components":{"Animator":{"speed":1}}},
 {"name":"InstanceB","model":"TwoJointRibbon.gltf","transform":{"position":[10,0,0]},"components":{"Animator":{"speed":0.5}}}
]})";

void AppendImportedFloat(std::vector<std::uint8_t>& out, float value) {
    static_assert(sizeof(float) == sizeof(std::uint32_t), "fixture requires Float32");
    std::uint32_t bits{};
    std::memcpy(&bits, &value, sizeof(bits));
    for (int shift = 0; shift < 32; shift += 8)
        out.push_back(static_cast<std::uint8_t>((bits >> shift) & 255u));
}

std::vector<std::uint8_t> ImportedAnimationBuffer() {
    std::vector<std::uint8_t> out;
    const float positions[2][9] = {{0,0,0, -2,0,0, 0,1,0}, {0,1,0, -2,1,0, 0,2,0}};
    for (int primitive = 0; primitive < 2; ++primitive) {
        ASTRAL_CHECK(out.size() == static_cast<std::size_t>(primitive * 140));
        for (float v : positions[primitive]) AppendImportedFloat(out, v);
        for (int vertex = 0; vertex < 3; ++vertex) {
            AppendImportedFloat(out, 0); AppendImportedFloat(out, 0); AppendImportedFloat(out, 1);
        }
        for (int vertex = 0; vertex < 3; ++vertex) {
            const int slot = primitive == 0 ? (vertex == 2 ? 0 : 1) : (vertex == 1 ? 1 : 0);
            out.insert(out.end(), {static_cast<std::uint8_t>(slot), 0, 0, 0});
        }
        for (int vertex = 0; vertex < 3; ++vertex) {
            AppendImportedFloat(out, 1); AppendImportedFloat(out, 0);
            AppendImportedFloat(out, 0); AppendImportedFloat(out, 0);
        }
        out.insert(out.end(), {0,0, 2,0, 1,0, 0,0}); // ushort indices, then two alignment bytes
    }
    // Column-major glTF inverse binds, in skin-slot order: TipJoint, RootJoint.
    // Their engine matrices are J_bind^-1 * M_BodyOne. Explicit constants keep
    // the numeric witness independent from the engine's inverse implementation.
    const float inverseBind[32] = {
        0,-1,0,0, 1,0,0,0, 0,0,1,0, -2,-1,0,1,
        0,-1,0,0, 1,0,0,0, 0,0,1,0, -2, 0,0,1};
    for (float v : inverseBind) AppendImportedFloat(out, v);
    const float times[3] = {0, 0.5f, 1};
    const float raise[9] = {0,1,0, 0,2,0, 0,3,0};
    const float bend[9] = {0,1,0, -1,1,0, -2,1,0};
    for (float v : times) AppendImportedFloat(out, v);
    for (float v : raise) AppendImportedFloat(out, v);
    for (float v : bend) AppendImportedFloat(out, v);
    ASTRAL_CHECK(out.size() == 492);
    return out;
}

struct ImportedAnimationFixture {
    std::filesystem::path root;
    std::filesystem::path physicalRoot;
    std::vector<std::filesystem::path> ownedFiles;

    ImportedAnimationFixture() {
        static unsigned sequence{};
        const auto temp = std::filesystem::canonical(std::filesystem::temp_directory_path());
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned attempt = 0; attempt < 100; ++attempt) {
            root = temp / ("astral-imported-animation-" + std::to_string(stamp) + "-" + std::to_string(++sequence));
            std::error_code error;
            if (std::filesystem::create_directory(root, error)) break;
            ASTRAL_CHECK(!error || error == std::errc::file_exists);
            root.clear();
        }
        ASTRAL_CHECK(!root.empty());
        physicalRoot = std::filesystem::canonical(root);
        ASTRAL_CHECK(physicalRoot.parent_path() == temp);
        std::printf("[ IMPORTED_ANIMATION_FIXTURE ] %s\n", root.string().c_str());
        std::fflush(stdout);
        const auto binary = ImportedAnimationBuffer();
        const std::string model(kImportedAnimationGltf), scene(kImportedAnimationScene);
        Write("TwoJointRibbon.gltf", {model.begin(), model.end()});
        Write("TwoJointRibbon.bin", binary);
        Write("two-joint.scene.json", {scene.begin(), scene.end()});
        Write("TwoJointRibbon.gltf.initial", {model.begin(), model.end()});
        Write("TwoJointRibbon.bin.initial", binary);
        Write("two-joint.scene.json.initial", {scene.begin(), scene.end()});
    }

    void Write(const char* filename, const std::vector<std::uint8_t>& bytes) {
        const auto target = root / filename;
        ASTRAL_CHECK(target.parent_path() == root && !std::filesystem::exists(target));
        std::ofstream file(target, std::ios::binary | std::ios::trunc);
        ASTRAL_CHECK(file && file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())));
        file.close();
        ASTRAL_CHECK(file.good());
        ownedFiles.push_back(target);
    }

    ~ImportedAnimationFixture() {
        // abort() on an OLD assertion retains the exact owned fixture. Normal
        // completion removes only this ledger, never recursively unknown files.
        ASTRAL_CHECK(std::filesystem::canonical(root) == physicalRoot);
        for (const auto& file : ownedFiles) {
            ASTRAL_CHECK(file.parent_path() == root);
            ASTRAL_CHECK(std::filesystem::symlink_status(file).type() == std::filesystem::file_type::regular);
            ASTRAL_CHECK(std::filesystem::remove(file));
        }
        ASTRAL_CHECK(std::filesystem::remove(root));
    }
};

void CheckImportedAnimationSource(const Assets::GltfDocument& model) {
    ASTRAL_CHECK(model.nodes.size() == 6 && model.meshes.size() == 2);
    ASTRAL_CHECK(model.skins.size() == 1 && model.animations.size() == 2);
    ASTRAL_CHECK(model.skins[0].joints == std::vector<int>({2, 1}));
    ASTRAL_CHECK(model.skins[0].inverseBind.size() == 2);
    ASTRAL_CHECK(model.meshes[0].primitives.size() == 2 && model.meshes[1].primitives.size() == 1);
    ASTRAL_CHECK(model.nodes[3].skin == 0 && model.nodes[4].skin == 0);
    ASTRAL_CHECK(model.nodes[3].mesh == 0 && model.nodes[4].mesh == 0);
    ASTRAL_CHECK(model.animations[0].name == "Raise" && model.animations[1].name == "Bend");
    ASTRAL_CHECK(model.animations[0].channels.size() == 1 && model.animations[0].channels[0].node == 2);
    for (const auto& primitive : model.meshes[0].primitives) {
        ASTRAL_CHECK(primitive.mesh.vertices.size() == 3 && primitive.mesh.skin.size() == 3);
        ASTRAL_CHECK(primitive.mesh.indices.size() == 3);
    }
    ASTRAL_CHECK(model.meshes[0].primitives[0].mesh.skin[0].joints[0] == 1);
    ASTRAL_CHECK(model.meshes[0].primitives[0].mesh.skin[2].joints[0] == 0);
    ASTRAL_CHECK(model.meshes[1].primitives[0].mesh.skin.empty());
}

Vec3 ImportedBindVertex(int primitive, std::size_t vertex) {
    const Vec3 expected[2][3] = {{{0,0,0}, {2,0,0}, {0,1,0}}, {{0,1,0}, {2,1,0}, {0,2,0}}};
    ASTRAL_CHECK(primitive >= 0 && primitive < 2 && vertex < 3);
    return expected[primitive][vertex];
}

bool ImportedTipVertex(int primitive, std::size_t vertex) {
    return primitive == 0 ? vertex == 2 : vertex != 1;
}

Vec3 ImportedExpectedWorld(int primitive, std::size_t vertex, float time, float wrapperX = 0) {
    const Vec3 bind = ImportedBindVertex(primitive, vertex);
    // BodyOne bind M is T(2,3,0) * Rz(+90deg). Raise adds world +Y
    // 2*time to tip-weighted vertices; BodyTwo has a different local inverse
    // but the same skin/inverse-bind pair, hence the same document-space result.
    return {wrapperX + 2 - bind.y, 3 + bind.x + (ImportedTipVertex(primitive, vertex) ? 2 * time : 0), 0};
}

Entity ImportedDescendant(const GameWorld& world, Entity parent, const std::string& name) {
    for (Entity child : World::GetChildren(world.Registry(), parent)) {
        const auto* label = world.Get<World::Name>(child);
        if (label && label->value == name) return child;
        const Entity found = ImportedDescendant(world, child, name);
        if (!found.IsNull()) return found;
    }
    return {};
}

void CheckImportedRenderedVertices(const GameWorld& world, Entity entity, int primitive, float time, float wrapperX) {
    const auto* renderer = world.Get<MeshRenderer>(entity);
    ASTRAL_CHECK(renderer && renderer->mesh && renderer->mesh->vertices.size() == 3);
    const TRS transform = world.GetWorld(entity);
    for (std::size_t vertex = 0; vertex < 3; ++vertex) {
        const Vec3 actual = Math::TransformPoint(transform, renderer->mesh->vertices[vertex].position);
        ASTRAL_CHECK(Near(actual, ImportedExpectedWorld(primitive, vertex, time, wrapperX), 2e-4f));
    }
}

} // namespace

ASTRAL_TEST(ImportedAnimationFixtureConvertsWithIndependentCoordinates) {
    ImportedAnimationFixture fixture;
    Assets::AssetManager assets(fixture.root.string(), nullptr);
    assets.RegisterDefaultLoaders();
    const auto handle = assets.Load<Assets::GltfDocument>("TwoJointRibbon.gltf");
    ASTRAL_CHECK(handle.Ready() && handle.Error().empty());
    CheckImportedAnimationSource(*handle.Get());
    Assets::GltfSkeletonBinding binding;
    std::string error;
    ASTRAL_CHECK(Assets::BuildSkeleton(*handle.Get(), 0, binding, error));
    ASTRAL_CHECK(binding.nodeToJoint[1] == 0 && binding.nodeToJoint[2] == 1);
    ASTRAL_CHECK(binding.skeleton.JointCount() == 2 && binding.skeleton.GetJoint(1).parent == 0);
    Animation::AnimationClip raise = Assets::BuildClip(*handle.Get(), 0, binding);
    Animation::AnimationClip bend = Assets::BuildClip(*handle.Get(), 1, binding);
    ASTRAL_CHECK(raise.Validate(binding.skeleton, error) && bend.Validate(binding.skeleton, error));
    Animation::Pose pose = Animation::Pose::Bind(binding.skeleton);
    raise.Sample(0.5f, pose);
    ASTRAL_CHECK(Near(pose.local[0].translation, {0,3,0}, 1e-5f));
    ASTRAL_CHECK(Near(pose.local[1].translation, {0,2,0}, 1e-5f));
    std::vector<Math::Mat4> matrices;
    Animation::ComputeSkinMatrices(binding.skeleton, pose, matrices);
    Math::Mat4 inverseOne, inverseTwo;
    ASTRAL_CHECK(Math::Inverse(handle->WorldMatrix(3), inverseOne));
    ASTRAL_CHECK(Math::Inverse(handle->WorldMatrix(4), inverseTwo));
    for (int primitive = 0; primitive < 2; ++primitive) {
        Graphics::MeshData remapped = handle->meshes[0].primitives[primitive].mesh;
        // Test-only explicit swap for this exact reversed two-joint fixture.
        // Do not call the future production binding/remapping helper.
        for (auto& influence : remapped.skin) {
            ASTRAL_CHECK(influence.joints[0] <= 1);
            influence.joints[0] = static_cast<std::uint16_t>(1 - influence.joints[0]);
        }
        Graphics::MeshData result;
        Animation::SkinMesh(remapped, matrices, result);
        for (std::size_t vertex = 0; vertex < 3; ++vertex) {
            const Vec3 world = ImportedExpectedWorld(primitive, vertex, 0.5f);
            ASTRAL_CHECK(Near(result.vertices[vertex].position, world, 1e-5f));
            // Literal inverse BodyOne: (world.y - 3, 2 - world.x, world.z).
            const Vec3 localOne = Math::TransformPoint(inverseOne, result.vertices[vertex].position);
            Vec3 expectedOne = ImportedBindVertex(primitive, vertex);
            if (ImportedTipVertex(primitive, vertex)) expectedOne.x += 1;
            ASTRAL_CHECK(Near(localOne, expectedOne, 1e-5f));
            // BodyTwo is T(5,3,0), which must not reuse BodyOne's inverse.
            const Vec3 localTwo = Math::TransformPoint(inverseTwo, result.vertices[vertex].position);
            const Vec3 bind = ImportedBindVertex(primitive, vertex);
            const Vec3 expectedTwo{-3 - bind.y, bind.x + (ImportedTipVertex(primitive, vertex) ? 1.0f : 0), 0};
            ASTRAL_CHECK(Near(localTwo, expectedTwo, 1e-5f));
        }
    }
    pose = Animation::Pose::Bind(binding.skeleton);
    bend.Sample(0.5f, pose);
    ASTRAL_CHECK(Near(pose.local[1].translation, {1,1,0}, 1e-5f));
}

ASTRAL_TEST(ImportedModelSceneBindsEveryPrimitiveAndOwnsPlayback) {
    ImportedAnimationFixture fixture;
    Assets::AssetManager assets(fixture.root.string(), nullptr);
    assets.RegisterDefaultLoaders();
    auto handle = assets.Load<Assets::GltfDocument>("TwoJointRibbon.gltf");
    ASTRAL_CHECK(handle.Ready() && handle.Error().empty());
    CheckImportedAnimationSource(*handle.Get());
    const auto original = *handle.Get();
    SceneSerializer serializer(&assets);
    SceneDocument document;
    std::string error;
    const bool parsed = serializer.ParseFile("two-joint.scene.json", document, error);
    if (!parsed) std::fprintf(stderr, "imported scene: %s\n", error.c_str());
    ASTRAL_CHECK(parsed && document.entities.size() == 2);
    GameWorld world;
    serializer.Instantiate(document, world);
    const Entity instances[] = {world.Find("InstanceA"), world.Find("InstanceB")};
    std::vector<Entity> parts;
    for (Entity instance : instances) {
        ASTRAL_CHECK(!instance.IsNull());
        for (const char* name : {"BodyOne", "BodyOne.primitive1", "BodyTwo", "BodyTwo.primitive1"}) {
            const Entity entity = ImportedDescendant(world, instance, name);
            ASTRAL_CHECK(!entity.IsNull());
            const auto* renderer = world.Get<MeshRenderer>(entity);
            ASTRAL_CHECK(renderer && renderer->mesh && renderer->mesh->vertices.size() == 3);
            parts.push_back(entity);
        }
        const Entity rigid = ImportedDescendant(world, instance, "RigidControl");
        ASTRAL_CHECK(!rigid.IsNull() && world.Get<MeshRenderer>(rigid));
        ASTRAL_CHECK(!world.Get<AnimatorComponent>(rigid));
    }
    ASTRAL_CHECK(parts.size() == 8);
    // Check the still-cached source before Collect could reload clean bytes
    // and hide an in-place mutation during scene binding.
    for (std::size_t primitive = 0; primitive < 2; ++primitive) {
        const auto& before = original.meshes[0].primitives[primitive].mesh;
        const auto& cached = handle->meshes[0].primitives[primitive].mesh;
        for (std::size_t vertex = 0; vertex < 3; ++vertex) {
            ASTRAL_CHECK(Near(before.vertices[vertex].position, cached.vertices[vertex].position, 0));
            ASTRAL_CHECK(before.skin[vertex].joints == cached.skin[vertex].joints);
            ASTRAL_CHECK(before.skin[vertex].weights == cached.skin[vertex].weights);
        }
    }
    std::vector<Graphics::MeshData> bindSnapshots;
    for (Entity entity : parts) {
        const auto* bound = world.Get<AnimatorComponent>(entity);
        // OLD must stop here: never dereference the absent imported binding.
        ASTRAL_CHECK(bound != nullptr);
        ASTRAL_CHECK(bound->skeleton && bound->machine && bound->bindMesh);
        bindSnapshots.push_back(*bound->bindMesh);
    }
    document = {}; // instantiated bindings must own all conversion resources
    handle = {};
    assets.Collect();
    world.Tick(0);
    for (std::size_t part = 0; part < parts.size(); ++part)
        CheckImportedRenderedVertices(world, parts[part], static_cast<int>(part % 2), 0, part < 4 ? 0.0f : 10.0f);
    Run(world, 0.5f, 0.25f);
    for (std::size_t part = 0; part < parts.size(); ++part) {
        CheckImportedRenderedVertices(world, parts[part], static_cast<int>(part % 2), part < 4 ? 0.5f : 0.25f,
            part < 4 ? 0.0f : 10.0f);
        const auto* animator = world.Get<AnimatorComponent>(parts[part]);
        ASTRAL_CHECK(animator && animator->animator && animator->skinned);
        ASTRAL_CHECK(animator->bindMesh && animator->bindMesh->vertices.size() == bindSnapshots[part].vertices.size());
        for (std::size_t vertex = 0; vertex < animator->bindMesh->vertices.size(); ++vertex)
            ASTRAL_CHECK(Near(animator->bindMesh->vertices[vertex].position, bindSnapshots[part].vertices[vertex].position, 0));
        for (std::size_t prior = 0; prior < part; ++prior) {
            const auto* other = world.Get<AnimatorComponent>(parts[prior]);
            ASTRAL_CHECK(animator->animator != other->animator && animator->skinned != other->skinned);
        }
    }
    const auto current = assets.Load<Assets::GltfDocument>("TwoJointRibbon.gltf");
    ASTRAL_CHECK(current.Ready() && current.Error().empty());
    for (std::size_t primitive = 0; primitive < 2; ++primitive) {
        const auto& before = original.meshes[0].primitives[primitive].mesh;
        const auto& after = current->meshes[0].primitives[primitive].mesh;
        for (std::size_t vertex = 0; vertex < 3; ++vertex) {
            ASTRAL_CHECK(Near(before.vertices[vertex].position, after.vertices[vertex].position, 0));
            ASTRAL_CHECK(before.skin[vertex].joints == after.skin[vertex].joints);
            ASTRAL_CHECK(before.skin[vertex].weights == after.skin[vertex].weights);
        }
    }
}

namespace {
void ReplaceImportedFile(const ImportedAnimationFixture& fixture, const char* name, const std::vector<std::uint8_t>& bytes) {
    const auto path = fixture.root / name;
    ASTRAL_CHECK(std::filesystem::canonical(fixture.root) == fixture.physicalRoot);
    ASTRAL_CHECK(std::find(fixture.ownedFiles.begin(), fixture.ownedFiles.end(), path) != fixture.ownedFiles.end());
    ASTRAL_CHECK(std::filesystem::symlink_status(path).type() == std::filesystem::file_type::regular);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    ASTRAL_CHECK(file && file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())));
    file.close(); ASTRAL_CHECK(file.good());
}
void ReplaceImportedModel(const ImportedAnimationFixture& fixture, const JsonValue& model) {
    const auto text = Core::WriteJson(model);
    ReplaceImportedFile(fixture, "TwoJointRibbon.gltf", {text.begin(), text.end()});
}
JsonValue* ImportedJsonNode(JsonValue& node, const std::string& name) {
    if (node.String("name") == name) return &node;
    for (const char* key : {"entities", "children"}) if (auto* array = node.Find(key))
        for (auto& child : array->Items()) if (auto* found = ImportedJsonNode(child, name)) return found;
    return nullptr;
}
JsonValue& ImportedAnimatorJson(JsonValue& scene, const char* name) {
    auto* node = ImportedJsonNode(scene, name);
    ASTRAL_CHECK(node && node->Find("components") && node->Find("components")->Find("Animator"));
    return *node->Find("components")->Find("Animator");
}
SceneDocument ImportedParse(SceneSerializer& serializer, const JsonValue& json) {
    SceneDocument document; std::string error;
    const bool parsed = serializer.Parse(json, document, error);
    if (!parsed) std::fprintf(stderr, "imported variant: %s\n", error.c_str());
    ASTRAL_CHECK(parsed); return document;
}
}

ASTRAL_TEST(ImportedAnimationInitialPosePreservesBehaviourLifecycle) {
    ImportedAnimationFixture fixture;
    Assets::AssetManager assets(fixture.root.string(), nullptr); assets.RegisterDefaultLoaders();
    RegisterTestBehaviours(); Log().clear();
    JsonValue json = ParseText(kImportedAnimationScene);
    auto* root = ImportedJsonNode(json, "InstanceA");
    JsonValue behaviours = JsonValue::MakeArray();
    auto recorder = JsonValue::MakeObject(); recorder.Set("type", "Recorder"); recorder.Set("tag", "initial");
    behaviours.Append(recorder); root->Set("behaviours", behaviours);
    root->Find("components")->Set("ParticleSystem", ParseText(R"({"playOnStart":true})"));
    root->Find("components")->Set("AudioSource", ParseText(R"({"playOnStart":true})"));
    SceneSerializer serializer(&assets); const auto document = ImportedParse(serializer, json);
    GameWorld world; serializer.Instantiate(document, world);
    ASTRAL_CHECK(world.Time() == 0 && world.Frame() == 0);
    ASTRAL_CHECK(Log() == std::vector<std::string>({"initial:create"}));
    const Entity instance = world.Find("InstanceA");
    ASTRAL_CHECK(!world.Get<ParticleSystem>(instance)->started && !world.Get<AudioSource>(instance)->started);
    for (const char* name : {"BodyOne", "BodyOne.primitive1", "BodyTwo", "BodyTwo.primitive1"})
        CheckImportedRenderedVertices(world, ImportedDescendant(world, instance, name), std::string(name).find("primitive1") != std::string::npos, 0, 0);
    world.Tick(0);
    ASTRAL_CHECK(std::find(Log().begin(), Log().end(), "initial:start") != Log().end());
}

ASTRAL_TEST(ImportedAnimationFlattenedControllerRoundTrips) {
    ImportedAnimationFixture fixture;
    Assets::AssetManager assets(fixture.root.string(), nullptr); assets.RegisterDefaultLoaders();
    SceneSerializer serializer(&assets); GameWorld original;
    serializer.Instantiate(ImportedParse(serializer, ParseText(kImportedAnimationScene)), original);
    auto flat = serializer.Save(original);
    auto* root = ImportedJsonNode(flat, "InstanceA");
    ASTRAL_CHECK(root && !root->Has("model"));
    ASTRAL_CHECK(ImportedAnimatorJson(flat, "InstanceA")["binding"].String("role") == "controller");
    auto& part = ImportedAnimatorJson(flat, "BodyOne");
    ASTRAL_CHECK(part.Size() == 1 && part["binding"].Int("node") == 3 && part["binding"].Int("skin") == 0);
    ASTRAL_CHECK(part["binding"]["meshBind"][12].AsNumber() == 0 && part["binding"]["meshBind"][15].AsNumber() == 1);
    GameWorld restored; serializer.Instantiate(ImportedParse(serializer, flat), restored);
    const Entity instance = restored.Find("InstanceA");
    CheckImportedRenderedVertices(restored, ImportedDescendant(restored, instance, "BodyTwo"), 0, 0, 0);
    ImportedAnimatorJson(flat, "InstanceA").Set("clip", "Bend");
    ImportedAnimatorJson(flat, "InstanceA").Set("speed", 0.5);
    GameWorld edited; serializer.Instantiate(ImportedParse(serializer, flat), edited);
    Run(edited, 0.5f, 0.25f);
    const Entity body = ImportedDescendant(edited, edited.Find("InstanceA"), "BodyOne");
    const auto* renderer = edited.Get<MeshRenderer>(body);
    const Vec3 tip = Math::TransformPoint(edited.GetWorld(body), renderer->mesh->vertices[2].position);
    ASTRAL_CHECK(Near(tip, {1.5f,3,0}, 2e-4f)); // Bend +X0.5 at local time .25
    CheckImportedRenderedVertices(edited, ImportedDescendant(edited, edited.Find("InstanceB"), "BodyTwo"), 0, 0.25f, 10);
}

ASTRAL_TEST(ImportedAnimationBindingValidationIsTransactional) {
    ImportedAnimationFixture fixture;
    Assets::AssetManager assets(fixture.root.string(), nullptr); assets.RegisterDefaultLoaders();
    SceneSerializer serializer(&assets); GameWorld world;
    serializer.Instantiate(ImportedParse(serializer, ParseText(kImportedAnimationScene)), world);
    const JsonValue flat = serializer.Save(world);
    const std::vector<std::function<void(JsonValue&)>> mutations = {
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Find("binding")->Set("node",99); },
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Find("binding")->Set("node",3.5); },
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Find("binding")->Set("role","unknown"); },
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Find("binding")->Set("skin",99); },
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Find("binding")->Set("mesh",1); },
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Find("binding")->Set("primitive",99); },
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Find("binding")->Set("model","missing.gltf"); },
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Find("binding")->Set("unknown",true); },
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Set("speed",2); },
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Find("binding")->Find("meshBind")->Items()[12] = 0.125; },
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Find("binding")->Find("meshBind")->Items().pop_back(); },
        [](auto& j) { for (int i=0;i<4;++i) ImportedAnimatorJson(j,"BodyOne").Find("binding")->Find("meshBind")->Items()[i] = 0; },
        [](auto& j) { ImportedAnimatorJson(j,"BodyOne").Find("binding")->Find("meshBind")->Items()[0] = std::numeric_limits<double>::max(); },
        [](auto& j) { ImportedAnimatorJson(j,"InstanceA").Set("applyRootMotion",true); },
        [](auto& j) { ImportedJsonNode(j,"BodyOne")->Find("components")->Find("MeshRenderer")->Set("autoLod",true); },
        [](auto& j) { ImportedJsonNode(j,"BodyOne")->Find("components")->Find("MeshRenderer")->Set("mesh","TwoJointRibbon.gltf#0.1"); },
        [](auto& j) { ImportedJsonNode(j,"BodyOne")->Find("components")->Set("Collider",ParseText(R"({"shape":"Mesh"})")); },
        [](auto& j) { ImportedJsonNode(j,"InstanceA")->Find("components")->Erase("Animator"); },
    };
    for (const auto& mutate : mutations) {
        auto bad = flat; mutate(bad);
        SceneDocument retained; retained.entities.emplace_back(); retained.entities[0].name = "Retained";
        std::string error;
        ASTRAL_CHECK(!serializer.Parse(bad, retained, error) && !error.empty());
        ASTRAL_CHECK(retained.entities.size() == 1 && retained.entities[0].name == "Retained");
    }
}

ASTRAL_TEST(ImportedAnimationNamedAndUnnamedClipSelection) {
    ImportedAnimationFixture fixture;
    Assets::AssetManager assets(fixture.root.string(), nullptr); assets.RegisterDefaultLoaders();
    SceneSerializer serializer(&assets);
    auto named = ParseText(kImportedAnimationScene); ImportedAnimatorJson(named,"InstanceA").Set("clip","Raise");
    auto source = ParseText(kImportedAnimationGltf);
    std::swap(source.Find("animations")->Items()[0], source.Find("animations")->Items()[1]);
    ReplaceImportedModel(fixture, source);
    auto document = ImportedParse(serializer, named);
    ASTRAL_CHECK(document.entities[0].importedAnimator->owner->animation == 1);
    source.Find("animations")->Items().pop_back(); ReplaceImportedModel(fixture,source); ASTRAL_CHECK(assets.Reload("TwoJointRibbon.gltf"));
    std::string error;
    const auto retained = document.entities[0].importedAnimator;
    ASTRAL_CHECK(!serializer.Parse(named,document,error));
    ASTRAL_CHECK(document.entities[0].importedAnimator == retained);
    source = ParseText(kImportedAnimationGltf); source.Find("animations")->Items()[0].Erase("name");
    ReplaceImportedModel(fixture,source); ASTRAL_CHECK(assets.Reload("TwoJointRibbon.gltf"));
    GameWorld automatic;
    serializer.Instantiate(ImportedParse(serializer,ParseText(kImportedAnimationScene)),automatic);
    auto flat = serializer.Save(automatic);
    ASTRAL_CHECK(ImportedAnimatorJson(flat,"InstanceA").String("clip").empty());
    GameWorld restored; serializer.Instantiate(ImportedParse(serializer,flat),restored);
    Run(restored,0.5f,0.25f);
    CheckImportedRenderedVertices(restored,ImportedDescendant(restored,restored.Find("InstanceA"),"BodyOne"),0,0.5f,0);
    source = ParseText(kImportedAnimationGltf); source.Find("animations")->Items()[1].Set("name","Raise");
    ReplaceImportedModel(fixture,source); ASTRAL_CHECK(assets.Reload("TwoJointRibbon.gltf"));
    ASTRAL_CHECK(!serializer.Parse(named,document,error));
    ASTRAL_CHECK(serializer.Parse(ParseText(kImportedAnimationScene),document,error)); // automatic duplicate name remains unpinned
    source = ParseText(kImportedAnimationGltf);
    source.Find("animations")->Items()[0].Find("channels")->Items()[0].Find("target")->Set("node",0);
    ReplaceImportedModel(fixture,source); ASTRAL_CHECK(assets.Reload("TwoJointRibbon.gltf"));
    ASTRAL_CHECK(!serializer.Parse(ParseText(kImportedAnimationScene),document,error));
}

ASTRAL_TEST(ImportedAnimationTwoSkinsStayIndependent) {
    ImportedAnimationFixture fixture;
    auto source = ParseText(kImportedAnimationGltf);
    auto binary = ImportedAnimationBuffer();
    const float extraInverse[32] = {0,-1,0,0, 1,0,0,0, 0,0,1,0, -4,-1,0,1,
                                  0,-1,0,0, 1,0,0,0, 0,0,1,0, -4,0,0,1};
    for (float value : extraInverse) AppendImportedFloat(binary,value);
    source.Find("buffers")->Items()[0].Set("byteLength",620);
    source.Find("bufferViews")->Append(ParseText(R"({"buffer":0,"byteOffset":492,"byteLength":128})"));
    source.Find("accessors")->Append(ParseText(R"({"bufferView":14,"componentType":5126,"count":2,"type":"MAT4"})"));
    JsonValue skin = source["skins"][0]; skin.Set("inverseBindMatrices",14); source.Find("skins")->Append(skin);
    source.Find("skins")->Items()[1].Set("joints",ParseText("[7,6]"));
    source.Find("nodes")->Append(ParseText(R"({"name":"IndependentRoot","children":[7]})"));
    source.Find("nodes")->Append(ParseText(R"({"name":"IndependentTip","translation":[0,1,0]})"));
    source.Find("nodes")->Items()[0].Find("children")->Append(6);
    source.Find("nodes")->Items()[4].Set("skin",1);
    ReplaceImportedFile(fixture,"TwoJointRibbon.bin",binary); ReplaceImportedModel(fixture,source);
    Assets::AssetManager assets(fixture.root.string(),nullptr); assets.RegisterDefaultLoaders();
    SceneSerializer serializer(&assets); GameWorld world;
    serializer.Instantiate(ImportedParse(serializer,ParseText(kImportedAnimationScene)),world);
    Run(world,0.5f,0.25f);
    const Entity a=world.Find("InstanceA");
    const auto* first=world.Get<AnimatorComponent>(ImportedDescendant(world,a,"BodyOne"));
    const auto* second=world.Get<AnimatorComponent>(ImportedDescendant(world,a,"BodyTwo"));
    ASTRAL_CHECK(first && second && first->skeleton != second->skeleton);
    CheckImportedRenderedVertices(world,ImportedDescendant(world,a,"BodyOne"),0,0.5f,0);
    const Entity body=ImportedDescendant(world,a,"BodyTwo");
    const auto* renderer=world.Get<MeshRenderer>(body);
    for (std::size_t vertex=0;vertex<3;++vertex) {
        Vec3 expected=ImportedExpectedWorld(0,vertex,0); expected.x+=2; // untargeted second skin stays bound
        ASTRAL_CHECK(Near(Math::TransformPoint(world.GetWorld(body),renderer->mesh->vertices[vertex].position),expected,2e-4f));
    }
}

ASTRAL_TEST(ImportedAnimationNestedModelsUseNearestController) {
    ImportedAnimationFixture fixture;
    Assets::AssetManager assets(fixture.root.string(),nullptr); assets.RegisterDefaultLoaders();
    auto scene=ParseText(kImportedAnimationScene);
    auto nested=scene["entities"][1]; nested.Set("name","NestedB");
    nested.Find("components")->Find("Animator")->Set("clip","Bend");
    auto rigid=nested; rigid.Set("name","StaticNested"); rigid.Erase("components");
    auto children=JsonValue::MakeArray(); children.Append(nested); children.Append(rigid);
    scene.Find("entities")->Items()[0].Set("children",children);
    SceneSerializer serializer(&assets); GameWorld world;
    serializer.Instantiate(ImportedParse(serializer,scene),world);
    Run(world,0.5f,0.25f);
    const Entity parent=world.Find("InstanceA");
    const Entity child=ImportedDescendant(world,parent,"NestedB");
    const Entity body=ImportedDescendant(world,child,"BodyOne");
    const auto* animator=world.Get<AnimatorComponent>(body);
    ASTRAL_CHECK(animator && animator->importedController==child);
    ASTRAL_CHECK(animator->importedOwner!=world.Get<AnimatorComponent>(parent)->importedOwner);
    const auto* renderer=world.Get<MeshRenderer>(body);
    ASTRAL_CHECK(Near(Math::TransformPoint(world.GetWorld(body),renderer->mesh->vertices[2].position),{11.5f,3,0},2e-4f));
    const Entity staticRoot=ImportedDescendant(world,parent,"StaticNested");
    ASTRAL_CHECK(!world.Get<AnimatorComponent>(ImportedDescendant(world,staticRoot,"BodyOne")));
    CheckImportedRenderedVertices(world,ImportedDescendant(world,parent,"BodyOne"),0,0.5f,0);
}

namespace {
std::string ImportedNonuniformNormalModelText() {
    std::string model(kImportedAnimationGltf);
    const std::string marker = R"({"name":"BodyOne","mesh":0,"skin":0,)";
    const auto position = model.find(marker);
    ASTRAL_CHECK(position != std::string::npos && model.find(marker, position + 1) == std::string::npos);
    model.insert(position + marker.size(), R"("scale":[2,3,4],)");
    return model;
}
std::vector<std::uint8_t> ImportedNonuniformNormalBuffer() {
    auto bytes = ImportedAnimationBuffer();
    const float rawNormal[3] = {-2.0f / 7, 3.0f / 7, 6.0f / 7};
    for (std::size_t offset : {std::size_t(36), std::size_t(176)}) {
        for (std::size_t vertex = 0; vertex < 3; ++vertex) for (std::size_t axis = 0; axis < 3; ++axis) {
            std::uint32_t bits{};
            std::memcpy(&bits, &rawNormal[axis], sizeof(bits));
            for (std::size_t byte = 0; byte < 4; ++byte)
                bytes[offset + vertex * 12 + axis * 4 + byte] = static_cast<std::uint8_t>((bits >> (byte * 8)) & 0xff);
        }
    }
    return bytes;
}
}

ASTRAL_TEST(ImportedAnimationNonuniformMeshNormalMatchesWorld) {
    ImportedAnimationFixture fixture;
    const auto model = ImportedNonuniformNormalModelText();
    ReplaceImportedFile(fixture, "TwoJointRibbon.bin", ImportedNonuniformNormalBuffer());
    ReplaceImportedFile(fixture, "TwoJointRibbon.gltf", {model.begin(), model.end()});
    Assets::AssetManager assets(fixture.root.string(), nullptr); assets.RegisterDefaultLoaders();
    SceneSerializer serializer(&assets); GameWorld world;
    serializer.Instantiate(ImportedParse(serializer, ParseText(kImportedAnimationScene)), world);
    ASTRAL_CHECK(world.Time() == 0 && world.Frame() == 0);
    const Entity body = ImportedDescendant(world, world.Find("InstanceA"), "BodyOne");
    const auto* renderer = world.Get<MeshRenderer>(body);
    const auto* bound = world.Get<AnimatorComponent>(body);
    ASTRAL_CHECK(bound && bound->animator && renderer && renderer->mesh && renderer->mesh->vertices.size() == 3);
    CheckImportedRenderedVertices(world, body, 0, 0, 0); // the mesh frame must still cancel for positions
    Math::Mat4 inverseWorld;
    ASTRAL_CHECK(Math::Inverse(world.Get<World::WorldTransform>(body)->matrix, inverseWorld));
    const Vec3 localNormal = renderer->mesh->vertices[0].normal;
    ASTRAL_CHECK(Math::IsFinite(localNormal) && Math::Length(localNormal) > 0.5f);
    const Vec3 worldNormal = Math::Normalize(Math::TransformVector(Math::Transpose(inverseWorld), localNormal));
    const Vec3 expectedWorldNormal{-3.0f / 7, 2.0f / 7, 6.0f / 7};
    std::printf("[ IMPORTED_NORMAL_WITNESS ] world=(%.9g,%.9g,%.9g) expected=(-3/7,2/7,6/7) local=(%.9g,%.9g,%.9g)\n",
        worldNormal.x, worldNormal.y, worldNormal.z, localNormal.x, localNormal.y, localNormal.z);
    std::fflush(stdout);
    ASTRAL_CHECK(Near(worldNormal, expectedWorldNormal, 2e-4f)); // first safe OLD-normal numeric rejection
    ASTRAL_CHECK(Near(localNormal, Math::Normalize(Vec3{4,9,24}), 2e-4f));
    const Entity part = ImportedDescendant(world, world.Find("InstanceA"), "BodyOne.primitive1");
    CheckImportedRenderedVertices(world, part, 1, 0, 0);
    for (Entity entity : {body, part}) for (const auto& vertex : world.Get<MeshRenderer>(entity)->mesh->vertices)
        ASTRAL_CHECK(Near(vertex.normal, Math::Normalize(Vec3{4,9,24}), 2e-4f));
}

ASTRAL_TEST(ImportedAnimationHelperNormalBlendHasDocumentFallback) {
    for (int mode = 0; mode < 3; ++mode) {
        auto skeleton = std::make_shared<Animation::Skeleton>();
        skeleton->AddJoint("first", -1, {}); skeleton->AddJoint("second", -1, {});
        skeleton->ComputeBindMatrices();
        Animation::AnimationClip clip;
        const Vec3 firstScale = mode == 0 ? Vec3{1,1,1} : mode == 1 ? Vec3{1,2,1} : Vec3{0,1,1};
        const Vec3 secondScale = mode == 0 ? Vec3{1,1,1} : mode == 1 ? Vec3{2,1,1} : Vec3{0,1,1};
        clip.AddKey(0, 0, {{}, {}, firstScale}); clip.AddKey(1, 0, {{}, {}, secondScale});
        auto machine = std::make_shared<Animation::AnimStateMachine>();
        machine->AddState({"normal-control", &clip});
        auto bind = std::make_shared<Graphics::MeshData>();
        for (Vec3 position : {Vec3{1,1,1}, Vec3{2,1,1}, Vec3{1,2,1}}) {
            Graphics::Vertex vertex; vertex.position = position; vertex.normal = {2.0f/7,3.0f/7,6.0f/7};
            bind->vertices.push_back(vertex);
            Graphics::SkinInfluence influence; influence.joints = {0,1,0,0}; influence.weights = {0.25f,0.75f,0,0};
            bind->skin.push_back(influence);
        }
        bind->indices = {0,1,2}; bind->ComputeBounds();
        AnimatorComponent component; component.skeleton = skeleton; component.machine = machine; component.bindMesh = bind;
        component.binding.role = ImportedAnimatorRole::Primitive;
        component.binding.meshBind = Math::Scaling({2,3,4});
        component.modelToMesh = Math::Scaling({0.5f,1.0f/3,0.25f});
        component.animator = std::make_shared<Animation::Animator>(*skeleton, *machine);
        MeshRenderer renderer; PublishImportedAnimationMesh(component, renderer);
        ASTRAL_CHECK(renderer.mesh && renderer.mesh->vertices.size() == 3);
        const Vec3 actualLocal = renderer.mesh->vertices[0].normal;
        ASTRAL_CHECK(Math::IsFinite(actualLocal) && Math::Length(actualLocal) > 0.5f);
        const Vec3 actualWorld = Math::Normalize(Math::TransformVector(component.modelToMesh, actualLocal)); // diagonal B^-T
        const Vec3 expectedWorld = Math::Normalize(mode == 1 ? Vec3{10,21,48} : Vec3{2,3,6});
        const Vec3 expectedLocal = Math::Normalize(mode == 1 ? Vec3{20,63,192} : Vec3{4,9,24});
        ASTRAL_CHECK(Near(actualWorld, expectedWorld, 2e-4f) && Near(actualLocal, expectedLocal, 2e-4f));
        const Vec3 position = Math::TransformPoint(component.binding.meshBind, renderer.mesh->vertices[0].position);
        const Vec3 expectedPosition = mode == 0 ? Vec3{1,1,1} : mode == 1 ? Vec3{1.75f,1.25f,1} : Vec3{0,1,1};
        ASTRAL_CHECK(Near(position, expectedPosition, 2e-4f));
    }
}

ASTRAL_TEST_MAIN("EngineFrameworkTests")
