#include "Engine/AI/BehaviorTree.h"
#include "Engine/AI/Navigation.h"
#include "Engine/Audio/AudioMixer.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/VFX/Particles.h"
#include "Tests/EngineTestSupport.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <thread>

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
Input::InputSnapshot KeySnapshot(std::initializer_list<std::uint16_t> down, float dt = 1.0f / 60.0f) {
    Input::InputSnapshot s;
    for (std::uint16_t k : down) s.keys.set(k);
    s.dt = dt;
    return s;
}
} // namespace

ASTRAL_TEST(InputActionsAxesAndEdges) {
    Input::InputSystem input = MakeCombatInput();
    input.Update(KeySnapshot({'W', 'D'}));
    const Math::Vec2 axis = input.Axis("Move");
    ASTRAL_CHECK_NEAR(Math::Length(axis), 1.0f, 1e-5); // diagonal normalised
    ASTRAL_CHECK(input.Down("Move"));
    input.Update(KeySnapshot({'J'}));
    ASTRAL_CHECK(input.Pressed("LightAttack"));
    input.Update(KeySnapshot({'J'}));
    ASTRAL_CHECK(!input.Pressed("LightAttack") && input.Down("LightAttack"));
    input.Update(KeySnapshot({}));
    ASTRAL_CHECK(input.Released("LightAttack") && input.Tapped("LightAttack"));
    // Double tap within the window.
    input.Update(KeySnapshot({'Q'}));
    input.Update(KeySnapshot({}));
    input.Update(KeySnapshot({'Q'}));
    ASTRAL_CHECK(input.DoubleTapped("Dash"));
}

ASTRAL_TEST(InputBufferAndContextsPriority) {
    Input::InputSystem input = MakeCombatInput();
    // Pressed during an attack recovery, consumed 8 frames later: still fires once.
    input.Update(KeySnapshot({'J'}));
    for (int i = 0; i < 8; ++i) input.Update(KeySnapshot({}));
    ASTRAL_CHECK(input.ConsumeBuffered("LightAttack"));
    ASTRAL_CHECK(!input.ConsumeBuffered("LightAttack"));
    // Too late: the 0.25 s buffer has expired.
    input.Update(KeySnapshot({'J'}));
    for (int i = 0; i < 20; ++i) input.Update(KeySnapshot({}));
    ASTRAL_CHECK(!input.ConsumeBuffered("LightAttack"));
    // The Thought prompt context captures J while open (typing does not attack).
    ASTRAL_CHECK(input.SetContextEnabled("ThoughtPrompt", true));
    input.Update(KeySnapshot({'J'}));
    ASTRAL_CHECK(input.Pressed("TypeJ"));
    ASTRAL_CHECK(!input.Pressed("LightAttack"));
    input.SetContextEnabled("ThoughtPrompt", false);
    input.Update(KeySnapshot({}));
    ASTRAL_CHECK(!input.Down("TypeJ"));
}

ASTRAL_TEST(InputExtremeFiniteAxesPreserveDirectionAndCancellation) {
    using namespace Input;
    const float largest = std::numeric_limits<float>::max();
    const float smallest = std::numeric_limits<float>::denorm_min();
    struct AxisCase { std::vector<Binding> bindings; Math::Vec2 expected; };
    const std::vector<AxisCase> cases = {
        {{{'W', {largest, largest}}, {'D', {largest, largest}}}, {0.70710678f, 0.70710678f}},
        {{{'W', {largest, largest}}, {'D', {-largest, -largest}}}, {}},
        {{{'W', {largest, 0}}, {'D', {1, 0}}, {'A', {-largest, 0}}}, {1, 0}},
        {{{'W', {smallest, 0}}}, {smallest, 0}},
        {{{'W', {0.25f, -0.5f}}}, {0.25f, -0.5f}},
    };
    for (const AxisCase& axisCase : cases) {
        InputSystem input;
        InputContext context;
        context.name = "Finite axes";
        context.actions = {ActionDesc{"Move", ActionType::Axis2D, axisCase.bindings}};
        ASTRAL_CHECK(input.AddContext(context));
        input.Update(KeySnapshot({'W', 'D', 'A'}));
        const Math::Vec2 axis = input.Axis("Move");
        ASTRAL_CHECK(std::isfinite(axis.x) && std::isfinite(axis.y));
        if (axisCase.expected.x == smallest) ASTRAL_CHECK(axis.x == smallest);
        else ASTRAL_CHECK_NEAR(axis.x, axisCase.expected.x, 1e-6);
        ASTRAL_CHECK_NEAR(axis.y, axisCase.expected.y, 1e-6);
        ASTRAL_CHECK(input.Down("Move") == (axisCase.expected.x != 0 || axisCase.expected.y != 0));
    }
}

ASTRAL_TEST(InputElapsedTimersStayFiniteAndExpireBeyondLargestWindow) {
    using namespace Input;
    const float largest = std::numeric_limits<float>::max();
    InputContext context;
    context.name = "Finite time";
    context.actions = {ActionDesc{"Held", ActionType::Button, {{'W', {}}}, largest, largest, largest}};
    InputSystem input;
    ASTRAL_CHECK(input.AddContext(context));
    input.Update(KeySnapshot({'W'}, 0));
    ASTRAL_CHECK(input.Pressed("Held") && !input.DoubleTapped("Held"));
    input.Update(KeySnapshot({'W'}, largest));
    ASTRAL_CHECK(input.HeldSeconds("Held") == largest && !input.ConsumeBuffered("Held"));
    input.Update(KeySnapshot({'W'}, largest));
    ASTRAL_CHECK(input.HeldSeconds("Held") == largest);
    input.Update(KeySnapshot({}, 0));
    ASTRAL_CHECK(input.Released("Held") && !input.Tapped("Held"));
    input.Update(KeySnapshot({'W'}, 0));
    ASTRAL_CHECK(input.Pressed("Held") && !input.DoubleTapped("Held"));
    for (float dt : {std::numeric_limits<float>::infinity(), -largest,
             std::numeric_limits<float>::quiet_NaN(), 0.0f}) {
        input.Update(KeySnapshot({'W'}, dt));
        ASTRAL_CHECK(input.HeldSeconds("Held") == 0 && !input.Pressed("Held"));
        ASTRAL_CHECK(std::isfinite(input.Axis("Held").x) && std::isfinite(input.Axis("Held").y));
    }
    input.Update(KeySnapshot({}, 0));
    ASTRAL_CHECK(input.Tapped("Held"));
    input.Update(KeySnapshot({'W'}, 0));
    ASTRAL_CHECK(input.DoubleTapped("Held"));
}

ASTRAL_TEST(InputDisabledContextsStillAgePriorPressHistory) {
    using namespace Input;
    const float largest = std::numeric_limits<float>::max();
    InputContext context;
    context.name = "History";
    context.actions = {ActionDesc{"Tap", ActionType::Button, {{'W', {}}}, 0, largest, largest}};
    InputSystem input;
    ASTRAL_CHECK(input.AddContext(context));
    input.Update(KeySnapshot({'W'}, 0));
    input.Update(KeySnapshot({}, 0));
    ASTRAL_CHECK(input.SetContextEnabled("History", false));
    input.Update(KeySnapshot({}, largest));
    input.Update(KeySnapshot({}, largest));
    ASTRAL_CHECK(input.SetContextEnabled("History", true));
    input.Update(KeySnapshot({'W'}, 0));
    if (input.DoubleTapped("Tap")) std::fprintf(stderr, "expired disabled-context history still double-taps\n");
    ASTRAL_CHECK(input.Pressed("Tap") && !input.DoubleTapped("Tap"));

    // A short disabled interval preserves history, and enabled frames age it
    // exactly once (the accumulated 0.2 seconds remains inside 0.3 seconds).
    InputSystem ordinary;
    context.actions[0].doubleTapSeconds = 0.3f;
    ASTRAL_CHECK(ordinary.AddContext(context));
    ordinary.Update(KeySnapshot({'W'}, 0));
    ordinary.Update(KeySnapshot({}, 0.1f));
    ASTRAL_CHECK(ordinary.SetContextEnabled("History", false));
    ordinary.Update(KeySnapshot({}, 0.1f));
    ASTRAL_CHECK(ordinary.SetContextEnabled("History", true));
    ordinary.Update(KeySnapshot({'W'}, 0));
    ASTRAL_CHECK(ordinary.DoubleTapped("Tap"));
    ordinary.Update(KeySnapshot({}, 0));
    ASTRAL_CHECK(ordinary.SetContextEnabled("History", false));
    ordinary.Update(KeySnapshot({}, 0.31f));
    ASTRAL_CHECK(ordinary.SetContextEnabled("History", true));
    ordinary.Update(KeySnapshot({'W'}, 0));
    ASTRAL_CHECK(!ordinary.DoubleTapped("Tap"));
}

ASTRAL_TEST(InputTimerSaturationDetectsLargestDeltaAfterSmallInterval) {
    using namespace Input;
    const float largest = std::numeric_limits<float>::max();
    InputContext context;
    context.name = "Residual time";
    context.actions = {ActionDesc{"Tap", ActionType::Button, {{'W', {}}}, 0, largest, largest}};
    InputSystem input;
    ASTRAL_CHECK(input.AddContext(context));
    input.Update(KeySnapshot({'W'}, 0));
    input.Update(KeySnapshot({'W'}, 1));
    input.Update(KeySnapshot({'W'}, largest));
    ASTRAL_CHECK(input.HeldSeconds("Tap") == largest);
    input.Update(KeySnapshot({}, 0));
    const bool tapped = input.Tapped("Tap");
    input.Update(KeySnapshot({'W'}, 0));
    const bool doubleTapped = input.DoubleTapped("Tap");
    if (tapped || doubleTapped) std::fprintf(stderr, "FLT_MAX plus small interval: tapped=%d doubleTapped=%d\n", tapped, doubleTapped);
    ASTRAL_CHECK(!tapped && !doubleTapped);
    InputSystem exact;
    ASTRAL_CHECK(exact.AddContext(context));
    exact.Update(KeySnapshot({'W'}, 0));
    exact.Update(KeySnapshot({'W'}, largest));
    exact.Update(KeySnapshot({}, 0));
    ASTRAL_CHECK(exact.Tapped("Tap"));
    exact.Update(KeySnapshot({'W'}, 0));
    ASTRAL_CHECK(exact.DoubleTapped("Tap")); // exactly FLT_MAX remains inside the window
}

ASTRAL_TEST(InputContextRejectsInvalidNumbersBeforeMutation) {
    using namespace Input;
    InputContext good;
    good.name = "Original";
    good.actions = {ActionDesc{"Keep", ActionType::Button, {{'W', {}}}, 0.5f}};
    InputSystem input;
    ASTRAL_CHECK(input.AddContext(good));
    input.Update(KeySnapshot({'W'}));
    const std::string originalBindings = input.SaveBindings();
    const float invalid[] = {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(), -1.0f};
    for (int field = 0; field < 5; ++field) {
        for (float value : invalid) {
            if (field < 2 && value == -1.0f) continue; // finite negative axes are valid
            InputContext rejected;
            rejected.name = "Rejected";
            rejected.priority = 100;
            rejected.consumesKeys = true;
            rejected.actions = {ActionDesc{"Partial", ActionType::Button, {{'W', {}}}}, good.actions[0]};
            ActionDesc& bad = rejected.actions[1];
            if (field == 0) bad.bindings[0].axisContribution.x = value;
            if (field == 1) bad.bindings[0].axisContribution.y = value;
            if (field == 2) bad.bufferSeconds = value;
            if (field == 3) bad.doubleTapSeconds = value;
            if (field == 4) bad.tapMaxSeconds = value;
            std::string error;
            ASTRAL_CHECK(!input.AddContext(rejected, &error) && error.find("actions[1]") != std::string::npos);
            ASTRAL_CHECK(input.SaveBindings() == originalBindings && !input.SetContextEnabled("Rejected", false));
            input.Update(KeySnapshot({'W'}));
            ASTRAL_CHECK(input.Down("Keep") && !input.Pressed("Keep") && !input.Down("Partial"));
        }
    }
    ASTRAL_CHECK(input.HeldSeconds("Keep") > 0.0f && input.ConsumeBuffered("Keep"));
}

ASTRAL_TEST(InputFiniteAxisRepairPreservesPriorityAndKeyConsumption) {
    using namespace Input;
    InputContext high;
    high.name = "High";
    high.priority = 10;
    high.consumesKeys = true;
    const float largest = std::numeric_limits<float>::max();
    high.actions = {ActionDesc{"Move", ActionType::Axis2D, {{'W', {largest, 0}}, {'D', {-largest, 0}}}}};
    InputContext low;
    low.name = "Low";
    low.actions = {ActionDesc{"Other", ActionType::Button, {{'W', {}}}}};
    InputSystem input;
    ASTRAL_CHECK(input.AddContext(low) && input.AddContext(high));
    input.Update(KeySnapshot({'W'}));
    ASTRAL_CHECK(input.Axis("Move").x == 1 && input.Pressed("Move") && !input.Down("Other"));
    input.Update(KeySnapshot({'W', 'D'}));
    ASTRAL_CHECK(input.Axis("Move").x == 0 && input.Released("Move") && !input.Down("Other"));
    ASTRAL_CHECK(input.SetContextEnabled("High", false));
    input.Update(KeySnapshot({'W'}));
    ASTRAL_CHECK(input.Axis("Move").x == 0 && input.Pressed("Other"));
}

ASTRAL_TEST(InputRebindingAndRecordingRoundTrip) {
    Input::InputSystem input = MakeCombatInput();
    ASTRAL_CHECK(input.Rebind("LightAttack", 'J', 'U'));
    ASTRAL_CHECK(!input.Rebind("LightAttack", 'J', 'U'));
    const std::string saved = input.SaveBindings();
    Input::InputSystem restored = MakeCombatInput();
    std::string error;
    ASTRAL_CHECK(restored.LoadBindings(saved, error));
    restored.Update(KeySnapshot({'U'}));
    ASTRAL_CHECK(restored.Pressed("LightAttack"));
    ASTRAL_CHECK(!restored.LoadBindings("ASTRAL_BINDINGS 1\nGameplay LightAttack 0 999\n", error));
    ASTRAL_CHECK(!restored.LoadBindings("ASTRAL_BINDINGS 1\nGameplay LightAttack 0 75\nGameplay Move 0 999\n", error));
    ASTRAL_CHECK(restored.SaveBindings() == saved);
    ASTRAL_CHECK(!restored.LoadBindings("junk", error));

    Input::InputRecording recording;
    recording.Record(KeySnapshot({'W'}, 0.016f));
    recording.Record(KeySnapshot({'W', 'J'}, 0.017f));
    recording.Record(KeySnapshot({}, 0.016f));
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
    Input::InputSnapshot look = KeySnapshot({'W'}, 0.016f);
    look.mouseDelta = {12.5f, -3.25f};
    recording.Record(look);
    Input::InputSnapshot still = KeySnapshot({}, 0.02f);
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
    const std::string previous = replay.Serialize();
    ASTRAL_CHECK(!replay.Deserialize("ASTRAL_INPUT 2 1\n1e100 0 0\n", error));
    ASTRAL_CHECK(!replay.Deserialize("ASTRAL_INPUT 2 1\n0.01 1e100 0\n", error));
    ASTRAL_CHECK(replay.Serialize() == previous);
    ASTRAL_CHECK(replay.Deserialize("ASTRAL_INPUT 2 1\n3.40282347e38 3.40282347e38 -3.40282347e38 87\n", error));
    ASTRAL_CHECK(std::isfinite(replay.Frame(0).dt) && std::isfinite(replay.Frame(0).mouseDelta.x));
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

ASTRAL_TEST(MixerOwnedClipsReleaseAtCompletionAndStop) {
    Audio::AudioMixer mixer(48000, 1);
    auto clip = std::make_shared<Audio::AudioClip>();
    clip->samples.assign(8, 0.25f);
    std::weak_ptr<const Audio::AudioClip> weak = clip;
    Audio::PlayParams params;
    const Audio::VoiceId voice = mixer.Play(clip, params);
    clip.reset();
    ASTRAL_CHECK(mixer.IsPlaying(voice) && !weak.expired());
    float stereo[16]{};
    mixer.Mix(stereo, 8);
    for (float sample : stereo) ASTRAL_CHECK(sample > 0.1f && sample < 0.3f);
    ASTRAL_CHECK(!mixer.IsPlaying(voice) && weak.expired());

    clip = std::make_shared<Audio::AudioClip>(Audio::Synth::Tone(220, 0.05f));
    weak = clip;
    params.loop = true;
    const Audio::VoiceId looping = mixer.Play(clip, params);
    clip.reset();
    mixer.Stop(voice); // prior generation must not stop the reused slot
    ASTRAL_CHECK(mixer.IsPlaying(looping) && !weak.expired());
    mixer.Stop(looping);
    ASTRAL_CHECK(!mixer.IsPlaying(looping) && weak.expired());
}

ASTRAL_TEST(MixerOwnedClipStealingAndRejectedPlaybackReleaseOwners) {
    Audio::AudioMixer mixer(48000, 1);
    Audio::PlayParams params;
    params.loop = true;
    params.priority = 2;
    auto first = std::make_shared<Audio::AudioClip>(Audio::Synth::Tone(220, 0.05f));
    std::weak_ptr<const Audio::AudioClip> firstWeak = first;
    const Audio::VoiceId firstVoice = mixer.Play(first, params);
    first.reset();

    auto rejected = std::make_shared<Audio::AudioClip>(Audio::Synth::Tone(330, 0.05f));
    std::weak_ptr<const Audio::AudioClip> rejectedWeak = rejected;
    Audio::PlayParams lower = params;
    lower.priority = 1;
    ASTRAL_CHECK(mixer.Play(rejected, lower).IsNull());
    rejected.reset();
    ASTRAL_CHECK(rejectedWeak.expired() && !firstWeak.expired() && mixer.IsPlaying(firstVoice));

    Audio::AudioClip borrowed = Audio::Synth::Tone(440, 0.05f);
    const Audio::VoiceId replacement = mixer.Play(&borrowed, params);
    ASTRAL_CHECK(firstWeak.expired() && !mixer.IsPlaying(firstVoice) && mixer.IsPlaying(replacement));
    mixer.Stop(firstVoice);
    ASTRAL_CHECK(mixer.IsPlaying(replacement));
    float stereo[32]{};
    mixer.Mix(stereo, 16); // explicit borrowed clip remains alive in this scope
    mixer.Stop(replacement);

    auto empty = std::make_shared<Audio::AudioClip>();
    std::weak_ptr<const Audio::AudioClip> emptyWeak = empty;
    ASTRAL_CHECK(mixer.Play(empty, params).IsNull());
    empty.reset();
    ASTRAL_CHECK(emptyWeak.expired());
}

ASTRAL_TEST(MixerOwnedPlaybackAndControlShareTheMixLock) {
    Audio::AudioMixer mixer(48000, 4);
    std::atomic<bool> done{false}, finite{true}, audible{true};
    std::atomic<int> published{0}, mixed{0}, mixCalls{0};
    std::thread audio([&] {
        float stereo[128]{};
        while (!done.load()) {
            const int phase = published.load();
            mixer.Mix(stereo, 64);
            ++mixCalls;
            bool positive = false;
            for (float sample : stereo) if (!std::isfinite(sample)) finite.store(false);
            for (float sample : stereo) if (sample > 0.1f) positive = true;
            if (phase > mixed.load()) {
                if (!positive) audible.store(false);
                mixed.store(phase);
            }
        }
    });
    Audio::PlayParams params;
    params.loop = true;
    for (int n = 0; n != 128; ++n) {
        auto clip = std::make_shared<Audio::AudioClip>();
        clip->samples.assign(64, 0.25f);
        const Audio::VoiceId voice = mixer.Play(std::move(clip), params);
        published.store(n + 1);
        // The audio worker must actually read this owned voice before Stop.
        // It keeps mixing while control releases/replaces voices between phases.
        while (mixed.load() < n + 1) std::this_thread::yield();
        mixer.Stop(voice);
    }
    done.store(true);
    audio.join();
    ASTRAL_CHECK(finite.load() && audible.load() && mixed.load() == 128 && mixCalls.load() >= 128);
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
