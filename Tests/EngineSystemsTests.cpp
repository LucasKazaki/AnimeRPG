#include "Engine/AI/BehaviorTree.h"
#include "Engine/AI/Navigation.h"
#include "Engine/Audio/AudioMixer.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/VFX/Particles.h"
#include "Tests/EngineTestSupport.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <initializer_list>

using namespace Astral;
using Math::Vec3;

// ------------------------------------------------------------------ VFX

ASTRAL_TEST(ParticleEmittersSpawnSimulateAndExpire) {
    VFX::EmitterSettings settings = VFX::Presets::SlashSparks({0, 0, 1});
    VFX::ParticleEmitter burst(settings, {0, 1, 0});
    ASTRAL_CHECK(burst.Alive() == 36);
    std::vector<Graphics::Billboard> billboards;
    burst.EmitBillboards(billboards);
    ASTRAL_CHECK(billboards.size() == 36);
    for (const auto& b : billboards) ASTRAL_CHECK(b.stretch > 1.0f && b.color.w > 0.0f);
    // Sparks fly forward within the cone.
    burst.Update(0.05f);
    billboards.clear();
    burst.EmitBillboards(billboards);
    float forward = 0.0f;
    for (const auto& b : billboards) forward += b.position.z;
    ASTRAL_CHECK(forward / static_cast<float>(billboards.size()) > 0.1f);
    for (int i = 0; i < 40; ++i) burst.Update(0.02f);
    ASTRAL_CHECK(burst.Finished());
    // Rate emitters honour their budget and deterministic seeds.
    VFX::EmitterSettings motes = VFX::Presets::ManaMotes(2.0f);
    VFX::ParticleEmitter a(motes, {}), b(motes, {});
    for (int i = 0; i < 300; ++i) {
        a.Update(1.0f / 60.0f);
        b.Update(1.0f / 60.0f);
    }
    ASTRAL_CHECK(a.Alive() == b.Alive());
    ASTRAL_CHECK(a.Alive() > 20 && a.Alive() <= 128);
    VFX::ParticleWorld world;
    world.Spawn(VFX::Presets::HitBurst(), {});
    const VFX::EmitterId smoke = world.Spawn(VFX::Presets::ShadowSmoke(), {});
    ASTRAL_CHECK(world.EmitterCount() == 2 && world.ParticleCount() == 44);
    world.Stop(smoke);
    for (int i = 0; i < 120; ++i) world.Update(1.0f / 60.0f);
    ASTRAL_CHECK(world.EmitterCount() == 0);
}

ASTRAL_TEST(RibbonTrailBuildsFadingStrip) {
    VFX::RibbonTrail trail(0.2f);
    for (int i = 0; i < 10; ++i) {
        const float angle = static_cast<float>(i) * 0.2f;
        trail.AddSample({0, 1, 0}, {std::sin(angle), 1.0f + std::cos(angle), 0}, static_cast<float>(i) * 0.01f);
    }
    Graphics::MeshData mesh;
    trail.BuildMesh(0.09f, {2, 1, 4, 1}, {0.5f, 0.2f, 1, 1}, mesh);
    std::string error;
    ASTRAL_CHECK(mesh.Validate(error));
    ASTRAL_CHECK(mesh.vertices.size() == 20 && mesh.indices.size() == 54);
    ASTRAL_CHECK(mesh.vertices.back().color.w > mesh.vertices[1].color.w); // newest sample is brightest
    trail.Prune(1.0f);
    ASTRAL_CHECK(trail.Empty());
}

// ------------------------------------------------------------------ Input

namespace {
Input::InputSystem MakeCombatInput() {
    using namespace Input;
    InputContext gameplay;
    gameplay.name = "Gameplay";
    gameplay.priority = 0;
    ActionDesc move{"Move", ActionType::Axis2D,
        {{Keys::Letter('W'), {0, 1}}, {Keys::Letter('S'), {0, -1}}, {Keys::Letter('A'), {-1, 0}}, {Keys::Letter('D'), {1, 0}}}};
    ActionDesc light{"LightAttack", ActionType::Button, {{Keys::Letter('J'), {}}}, 0.25f};
    ActionDesc dash{"Dash", ActionType::Button, {{Keys::Letter('Q'), {}}}, 0.15f};
    gameplay.actions = {move, light, dash};
    InputContext thought;
    thought.name = "ThoughtPrompt";
    thought.priority = 10;
    thought.enabled = false;
    thought.consumesKeys = true;
    thought.actions = {ActionDesc{"Submit", ActionType::Button, {{Keys::Enter, {}}}},
        ActionDesc{"TypeJ", ActionType::Button, {{Keys::Letter('J'), {}}}}};
    InputSystem input;
    input.AddContext(gameplay);
    input.AddContext(thought);
    return input;
}
Input::InputSnapshot Keys(std::initializer_list<std::uint16_t> down, float dt = 1.0f / 60.0f) {
    Input::InputSnapshot s;
    for (std::uint16_t k : down) s.keys.set(k);
    s.dt = dt;
    return s;
}
} // namespace

ASTRAL_TEST(InputActionsAxesAndEdges) {
    Input::InputSystem input = MakeCombatInput();
    input.Update(Keys({'W', 'D'}));
    const Math::Vec2 axis = input.Axis("Move");
    ASTRAL_CHECK_NEAR(Math::Length(axis), 1.0f, 1e-5); // diagonal normalised
    ASTRAL_CHECK(input.Down("Move"));
    input.Update(Keys({'J'}));
    ASTRAL_CHECK(input.Pressed("LightAttack"));
    input.Update(Keys({'J'}));
    ASTRAL_CHECK(!input.Pressed("LightAttack") && input.Down("LightAttack"));
    input.Update(Keys({}));
    ASTRAL_CHECK(input.Released("LightAttack") && input.Tapped("LightAttack"));
    // Double tap within the window.
    input.Update(Keys({'Q'}));
    input.Update(Keys({}));
    input.Update(Keys({'Q'}));
    ASTRAL_CHECK(input.DoubleTapped("Dash"));
}

ASTRAL_TEST(InputBufferAndContextsPriority) {
    Input::InputSystem input = MakeCombatInput();
    // Pressed during an attack recovery, consumed 8 frames later: still fires once.
    input.Update(Keys({'J'}));
    for (int i = 0; i < 8; ++i) input.Update(Keys({}));
    ASTRAL_CHECK(input.ConsumeBuffered("LightAttack"));
    ASTRAL_CHECK(!input.ConsumeBuffered("LightAttack"));
    // Too late: the 0.25 s buffer has expired.
    input.Update(Keys({'J'}));
    for (int i = 0; i < 20; ++i) input.Update(Keys({}));
    ASTRAL_CHECK(!input.ConsumeBuffered("LightAttack"));
    // The Thought prompt context captures J while open (typing does not attack).
    ASTRAL_CHECK(input.SetContextEnabled("ThoughtPrompt", true));
    input.Update(Keys({'J'}));
    ASTRAL_CHECK(input.Pressed("TypeJ"));
    ASTRAL_CHECK(!input.Pressed("LightAttack"));
    input.SetContextEnabled("ThoughtPrompt", false);
    input.Update(Keys({}));
    ASTRAL_CHECK(!input.Down("TypeJ"));
}

ASTRAL_TEST(InputRebindingAndRecordingRoundTrip) {
    Input::InputSystem input = MakeCombatInput();
    ASTRAL_CHECK(input.Rebind("LightAttack", 'J', 'U'));
    ASTRAL_CHECK(!input.Rebind("LightAttack", 'J', 'U'));
    const std::string saved = input.SaveBindings();
    Input::InputSystem restored = MakeCombatInput();
    std::string error;
    ASTRAL_CHECK(restored.LoadBindings(saved, error));
    restored.Update(Keys({'U'}));
    ASTRAL_CHECK(restored.Pressed("LightAttack"));
    ASTRAL_CHECK(!restored.LoadBindings("ASTRAL_BINDINGS 1\nGameplay LightAttack 0 999\n", error));
    ASTRAL_CHECK(!restored.LoadBindings("junk", error));

    Input::InputRecording recording;
    recording.Record(Keys({'W'}, 0.016f));
    recording.Record(Keys({'W', 'J'}, 0.017f));
    recording.Record(Keys({}, 0.016f));
    Input::InputRecording replay;
    ASTRAL_CHECK(replay.Deserialize(recording.Serialize(), error));
    ASTRAL_CHECK(replay.FrameCount() == 3);
    ASTRAL_CHECK(replay.Frame(1).Down('J') && replay.Frame(1).Down('W') && !replay.Frame(2).Down('W'));
    ASTRAL_CHECK_NEAR(replay.Frame(1).dt, 0.017f, 1e-7);
    // Replaying the recording through a fresh system reproduces the same edges.
    Input::InputSystem a = MakeCombatInput(), b = MakeCombatInput();
    for (std::size_t i = 0; i < recording.FrameCount(); ++i) {
        a.Update(recording.Frame(i));
        b.Update(replay.Frame(i));
        ASTRAL_CHECK(a.Pressed("LightAttack") == b.Pressed("LightAttack"));
    }
    ASTRAL_CHECK(!replay.Deserialize("ASTRAL_INPUT 1 2\n0.01 5\n", error));
    ASTRAL_CHECK(!replay.Deserialize("ASTRAL_INPUT 1 1\n0.01 300\n", error));
}

ASTRAL_TEST(InputRecordingPreservesMouseDeltas) {
    Input::InputRecording recording;
    Input::InputSnapshot look = Keys({'W'}, 0.016f);
    look.mouseDelta = {12.5f, -3.25f};
    recording.Record(look);
    Input::InputSnapshot still = Keys({}, 0.02f);
    recording.Record(still);
    Input::InputRecording replay;
    std::string error;
    ASTRAL_CHECK(replay.Deserialize(recording.Serialize(), error));
    ASTRAL_CHECK(replay.FrameCount() == 2);
    ASTRAL_CHECK(replay.Frame(0).mouseDelta.x == 12.5f && replay.Frame(0).mouseDelta.y == -3.25f);
    ASTRAL_CHECK(replay.Frame(0).Down('W'));
    ASTRAL_CHECK(replay.Frame(1).mouseDelta.x == 0.0f && replay.Frame(1).mouseDelta.y == 0.0f);
    // Version 1 recordings (no mouse fields) still load; malformed v2 rows do not.
    ASTRAL_CHECK(replay.Deserialize("ASTRAL_INPUT 1 1\n0.01 87\n", error) && replay.Frame(0).Down('W'));
    ASTRAL_CHECK(!replay.Deserialize("ASTRAL_INPUT 2 1\n0.01\n", error));
    ASTRAL_CHECK(!replay.Deserialize("ASTRAL_INPUT 2 1\n0.01 nan 0\n", error));
}

// ------------------------------------------------------------------ Audio

ASTRAL_TEST(MixerSpatialisesPansAndLimits) {
    Audio::AudioMixer mixer(48000, 4);
    const Audio::AudioClip tone = Audio::Synth::Tone(440.0f, 0.5f, 0.0f, 0.0f);
    mixer.SetListener({0, 0, 0}, {0, 0, 1});
    Audio::PlayParams right;
    right.spatial = true;
    right.position = {5, 0, 0};
    const Audio::VoiceId id = mixer.Play(&tone, right);
    ASTRAL_CHECK(mixer.IsPlaying(id));
    std::vector<float> buffer(2 * 480);
    mixer.Mix(buffer.data(), 480);
    float left = 0.0f, rightEnergy = 0.0f;
    for (int i = 0; i < 480; ++i) {
        left += buffer[static_cast<std::size_t>(i * 2)] * buffer[static_cast<std::size_t>(i * 2)];
        rightEnergy += buffer[static_cast<std::size_t>(i * 2 + 1)] * buffer[static_cast<std::size_t>(i * 2 + 1)];
    }
    ASTRAL_CHECK(rightEnergy > left * 50.0f); // source on the listener's right
    // Distance attenuation.
    mixer.SetVoicePosition(id, {0, 0, 30});
    mixer.Mix(buffer.data(), 480);
    float far = 0.0f;
    for (float s : buffer) far += s * s;
    ASTRAL_CHECK(far < (left + rightEnergy) * 0.1f);
    // Clip ends and the voice frees itself.
    std::vector<float> long_(2 * 48000);
    mixer.Mix(long_.data(), 48000);
    ASTRAL_CHECK(!mixer.IsPlaying(id));
    // Many loud voices are limited below full scale and excess voices are stolen.
    Audio::PlayParams loud;
    loud.volume = 4.0f;
    loud.loop = true;
    for (int i = 0; i < 6; ++i) mixer.Play(&tone, loud);
    mixer.Mix(buffer.data(), 480);
    for (float s : buffer) ASTRAL_CHECK(std::fabs(s) <= 1.0f);
    const Audio::MixerStats stats = mixer.Stats();
    ASTRAL_CHECK(stats.activeVoices == 4 && stats.stolenVoices == 2 && stats.limitedSamples > 0);
    ASTRAL_CHECK(mixer.Play(nullptr, loud).IsNull());
}

ASTRAL_TEST(ProceduralSfxAndWavOutput) {
    for (const Audio::AudioClip& clip : {Audio::Synth::Whoosh(0.3f, 300, 3000), Audio::Synth::Impact(0.4f, 60),
             Audio::Synth::Chime(880, 1.0f), Audio::Synth::Drone(55, 2.0f), Audio::Synth::Footstep(3)}) {
        ASTRAL_CHECK(!clip.samples.empty());
        float peak = 0.0f;
        for (float s : clip.samples) {
            ASTRAL_CHECK(std::isfinite(s));
            peak = std::max(peak, std::fabs(s));
        }
        ASTRAL_CHECK(peak > 0.01f && peak <= 1.5f);
    }
    std::vector<float> stereo(200, 0.25f);
    std::string error;
    const std::string path = "astral_audio_test.wav";
    ASTRAL_CHECK(Audio::WriteWav(path, stereo, 48000, error));
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    ASTRAL_CHECK(static_cast<long>(file.tellg()) == 44 + 400);
    file.close();
    std::remove(path.c_str());
}

// ------------------------------------------------------------------ AI

ASTRAL_TEST(NavGridPathsAroundObstaclesWithoutCornerCutting) {
    AI::NavGridDesc desc;
    desc.origin = {-10, 0, -10};
    desc.cellSize = 0.5f;
    desc.width = 40;
    desc.depth = 40;
    AI::NavGrid grid(desc);
    Physics::PhysicsWorld world;
    Physics::BodyDesc wall;
    wall.type = Physics::BodyType::Static;
    wall.shape = Physics::Shape::Box({0.5f, 1.0f, 6.0f});
    wall.position = {0, 1, -2};
    world.CreateBody(wall);
    grid.BakeFromPhysics(world, 0.4f);
    ASTRAL_CHECK(grid.WalkableCount() < 1600);
    const AI::PathResult path = grid.FindPath({-5, 0, 0}, {5, 0, 0});
    ASTRAL_CHECK(path.found && path.points.size() >= 3);
    // Every segment of the smoothed path stays clear of the wall's footprint.
    for (std::size_t i = 0; i + 1 < path.points.size(); ++i) ASTRAL_CHECK(grid.LineWalkable(path.points[i], path.points[i + 1]));
    ASTRAL_CHECK(path.points.front().x == -5.0f && path.points.back().x == 5.0f);
    // Goes around the end of the wall (z > 4) since the wall spans z -8..4.
    bool detour = false;
    for (const Vec3& p : path.points) detour |= p.z > 4.0f;
    ASTRAL_CHECK(detour);
    // Fully enclosed goal: no path, bounded search.
    for (int x = 30; x < 36; ++x) {
        grid.SetWalkable(x, 30, false);
        grid.SetWalkable(x, 35, false);
    }
    for (int z = 30; z <= 35; ++z) {
        grid.SetWalkable(30, z, false);
        grid.SetWalkable(35, z, false);
    }
    const Vec3 enclosed = grid.CellCenter(32, 32);
    const AI::PathResult none = grid.FindPath({-5, 0, 0}, enclosed);
    ASTRAL_CHECK(!none.found || !grid.LineWalkable(none.points.back(), enclosed) || none.points.back().x != enclosed.x);
    // Diagonal squeeze between two blocked corners is not allowed.
    AI::NavGrid tight(desc);
    tight.SetWalkable(10, 11, false);
    tight.SetWalkable(11, 10, false);
    ASTRAL_CHECK(!tight.LineWalkable(tight.CellCenter(10, 10), tight.CellCenter(11, 11)));
}

ASTRAL_TEST(BehaviorTreeEnemyLoop) {
    using namespace AI;
    // Chase until in range, attack with a cooldown, otherwise patrol.
    int attacks = 0, patrolTicks = 0;
    auto chaseOrAttack = MakeSequence(
        Make<Condition>([](const Blackboard& bb) { return bb.GetBool("SeesPlayer"); }),
        MakeSelector(
            MakeSequence(Make<Condition>([](const Blackboard& bb) { return bb.GetFloat("Distance") < 2.0f; }),
                Make<Cooldown>(Make<Action>([&](Blackboard&, float) {
                    ++attacks;
                    return Status::Success;
                }), 1.0f)),
            Make<Action>([](Blackboard& bb, float dt) {
                bb.SetFloat("Distance", std::max(0.0f, bb.GetFloat("Distance") - 5.0f * dt));
                return Status::Running;
            })));
    auto root = MakeSelector(std::move(chaseOrAttack), Make<Action>([&](Blackboard&, float) {
        ++patrolTicks;
        return Status::Running;
    }));
    BehaviorTree tree(std::move(root));
    Blackboard bb;
    bb.SetFloat("Distance", 10.0f);
    for (int i = 0; i < 10; ++i) tree.Tick(bb, 0.1f);
    ASTRAL_CHECK(patrolTicks == 10 && attacks == 0);
    bb.SetBool("SeesPlayer", true);
    for (int i = 0; i < 40; ++i) tree.Tick(bb, 0.1f);
    ASTRAL_CHECK(bb.GetFloat("Distance") < 2.0f);
    ASTRAL_CHECK(attacks == 3); // ticks 18, 29 and 40: the cooldown survives branch aborts
    tree.Reset(); // a full reset (encounter restart) clears the cooldown
    tree.Tick(bb, 0.1f);
    ASTRAL_CHECK(attacks == 4);
    // Decorators.
    int ran = 0;
    Repeat repeat(Make<Action>([&](Blackboard&, float) {
        ++ran;
        return Status::Success;
    }), 3);
    ASTRAL_CHECK(repeat.Tick(bb, 0) == Status::Running);
    repeat.Tick(bb, 0);
    ASTRAL_CHECK(repeat.Tick(bb, 0) == Status::Success && ran == 3);
    TimeLimit limit(Make<Wait>(5.0f), 1.0f);
    ASTRAL_CHECK(limit.Tick(bb, 0.6f) == Status::Running);
    ASTRAL_CHECK(limit.Tick(bb, 0.6f) == Status::Failure);
    Parallel parallel(2);
    parallel.Add(Make<Wait>(0.1f)).Add(Make<Wait>(0.2f));
    ASTRAL_CHECK(parallel.Tick(bb, 0.15f) == Status::Running);
    ASTRAL_CHECK(parallel.Tick(bb, 0.15f) == Status::Success);
    Inverter inverter(Make<Condition>([](const Blackboard&) { return true; }));
    ASTRAL_CHECK(inverter.Tick(bb, 0) == Status::Failure);
}

ASTRAL_TEST(PerceptionUsesConeRangeAndOcclusion) {
    Physics::PhysicsWorld world;
    Physics::BodyDesc pillar;
    pillar.type = Physics::BodyType::Static;
    pillar.shape = Physics::Shape::Box({0.5f, 2, 0.5f});
    pillar.position = {0, 2, 5};
    world.CreateBody(pillar);
    const Vec3 eye{0, 1.6f, 0};
    ASTRAL_CHECK(!AI::CanSee(world, eye, {0, 0, 1}, {0, 1.6f, 10}, 90, 20)); // behind the pillar
    ASTRAL_CHECK(AI::CanSee(world, eye, {0, 0, 1}, {3, 1.6f, 10}, 90, 20));  // visible past it
    ASTRAL_CHECK(!AI::CanSee(world, eye, {0, 0, 1}, {0, 1.6f, -10}, 90, 20)); // behind the viewer
    ASTRAL_CHECK(!AI::CanSee(world, eye, {0, 0, 1}, {3, 1.6f, 30}, 90, 20));  // out of range
}

ASTRAL_TEST_MAIN("EngineSystemsTests")
