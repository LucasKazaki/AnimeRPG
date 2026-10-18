#include "Engine/Animation/Timeline.h"
#include "Engine/Animation/Tween.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Core/Json.h"
#include "Engine/Framework/GameWorld.h"
#include "Engine/Framework/Sequencer.h"
#include "Tests/EngineTestSupport.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace Astral;
using namespace Astral::Animation;
using Core::JsonValue;
using Math::Quat;
using Math::Vec3;

namespace {

bool Near(Vec3 a, Vec3 b, float tolerance) { return Math::Length(a - b) <= tolerance; }

JsonValue ParseText(const std::string& text) {
    JsonValue json;
    std::string error;
    const bool ok = Core::ParseJson(text, json, error);
    if (!ok) std::fprintf(stderr, "json: %s\n", error.c_str());
    ASTRAL_CHECK(ok);
    return json;
}

// Records everything a timeline applies.
struct RecordingBinder : TimelineBinder {
    std::vector<std::string> events;
    std::vector<std::string> activations;
    float lastFloat{};
    Vec3 lastVector{};
    Quat lastRotation{};
    void SetFloat(const std::string&, const std::string&, float value) override { lastFloat = value; }
    void SetVector(const std::string&, const std::string&, Vec3 value) override { lastVector = value; }
    void SetRotation(const std::string&, const std::string&, Quat value) override { lastRotation = value; }
    void SetActive(const std::string& binding, bool active) override {
        activations.push_back(binding + (active ? ":on" : ":off"));
    }
    void OnEvent(const std::string&, const TimelineEvent& event) override { events.push_back(event.name); }
};

std::shared_ptr<TimelineAsset> EventTimeline() {
    auto asset = std::make_shared<TimelineAsset>();
    asset->name = "Beats";
    asset->duration = 4.0f;
    TimelineTrack events;
    events.kind = TrackKind::Event;
    events.events = {{0.0f, "start", ""}, {1.0f, "one", ""}, {2.5f, "mid", "payload"}, {4.0f, "end", ""}};
    TimelineTrack value;
    value.kind = TrackKind::Float;
    value.binding = "Lamp";
    value.property = "light.intensity";
    value.floatKeys = {{0.0f, 0.0f, KeyInterpolation::Linear, Ease::Linear}, {4.0f, 8.0f, KeyInterpolation::Linear, Ease::Linear}};
    asset->tracks = {events, value};
    std::string error;
    ASTRAL_CHECK(asset->Validate(error));
    return asset;
}

} // namespace

// ------------------------------------------------------------------ easing and tweens

ASTRAL_TEST(EasingCurvesHitEndpointsAndHaveTheirShapes) {
    for (int i = 0; i < static_cast<int>(Ease::Count); ++i) {
        const Ease ease = static_cast<Ease>(i);
        ASTRAL_CHECK(EaseValue(ease, 0.0f) == 0.0f && EaseValue(ease, 1.0f) == 1.0f);
        ASTRAL_CHECK(EaseValue(ease, -3.0f) == 0.0f && EaseValue(ease, 7.0f) == 1.0f && EaseValue(ease, std::nanf("")) == 0.0f);
        Ease parsed;
        ASTRAL_CHECK(ParseEase(EaseName(ease), parsed) && parsed == ease);
        // Continuity: no jumps between neighbouring samples.
        float previous = 0.0f, low = 0.0f, high = 1.0f;
        for (int s = 1; s <= 400; ++s) {
            const float value = EaseValue(ease, static_cast<float>(s) / 400.0f);
            ASTRAL_CHECK(std::isfinite(value) && std::fabs(value - previous) < 0.2f);
            low = std::min(low, value);
            high = std::max(high, value);
            previous = value;
        }
        const std::string name = EaseName(ease);
        const bool overshoots = name.find("Back") != std::string::npos || name.find("Elastic") != std::string::npos;
        if (!overshoots) ASTRAL_CHECK(low >= -1e-5f && high <= 1.0f + 1e-5f);
        if (name.rfind("InOut", 0) == 0 && name != "InOutElastic") ASTRAL_CHECK_NEAR(EaseValue(ease, 0.5f), 0.5f, 1e-3);
    }
    Ease parsed;
    ASTRAL_CHECK(!ParseEase("Wobbly", parsed));
    ASTRAL_CHECK(EaseValue(Ease::InBack, 0.3f) < 0.0f && EaseValue(Ease::OutBack, 0.7f) > 1.0f);
    ASTRAL_CHECK(EaseValue(Ease::InQuad, 0.5f) < 0.5f && EaseValue(Ease::OutQuad, 0.5f) > 0.5f);
    ASTRAL_CHECK_NEAR(EaseValue(Ease::OutBounce, 1.0f / 2.75f), 1.0f, 1e-5);
}

ASTRAL_TEST(TweensInterpolateLoopAndComplete) {
    TweenManager tweens;
    float value = -1.0f;
    int completions = 0;
    const TweenHandle linear = tweens.Float(0.0f, 10.0f, 1.0f, [&](float v) { value = v; }, Ease::Linear);
    tweens.OnComplete(linear, [&] { ++completions; });
    tweens.Update(0.25f);
    ASTRAL_CHECK_NEAR(value, 2.5f, 1e-5);
    tweens.Update(1.0f);
    ASTRAL_CHECK(value == 10.0f && completions == 1 && !tweens.IsActive(linear));
    tweens.Update(1.0f);
    ASTRAL_CHECK(completions == 1);

    // Delay, yoyo loops and time scale.
    const TweenHandle yoyo = tweens.Float(0.0f, 4.0f, 1.0f, [&](float v) { value = v; }, Ease::Linear);
    tweens.SetDelay(yoyo, 0.5f);
    tweens.SetLoops(yoyo, 3, LoopMode::Yoyo);
    ASTRAL_CHECK_NEAR(tweens.TotalDuration(yoyo), 3.5f, 1e-6);
    value = -1.0f;
    tweens.Update(0.25f);
    ASTRAL_CHECK(value == -1.0f); // still delayed
    tweens.Update(0.5f);
    ASTRAL_CHECK_NEAR(value, 1.0f, 1e-5);
    tweens.Update(1.0f); // t = 1.25 -> second (reversed) cycle at 0.25 -> 3.0
    ASTRAL_CHECK_NEAR(value, 3.0f, 1e-5);
    tweens.SetTimeScale(yoyo, 2.0f);
    tweens.Update(0.25f); // +0.5 s -> t = 1.75, reversed cycle at 0.75 -> 1.0
    ASTRAL_CHECK_NEAR(value, 1.0f, 1e-5);
    tweens.Pause(yoyo, true);
    tweens.Update(5.0f);
    ASTRAL_CHECK(tweens.IsActive(yoyo) && value == 1.0f);
    tweens.Pause(yoyo, false);
    tweens.Update(5.0f);
    ASTRAL_CHECK(value == 4.0f && !tweens.IsActive(yoyo)); // odd loop count ends forward

    // Kill with completion jumps to the end; infinite loops end their cycle.
    Vec3 position{};
    const TweenHandle move = tweens.Vector({0, 0, 0}, {2, 4, 6}, 2.0f, [&](Vec3 v) { position = v; });
    tweens.Update(0.5f);
    ASTRAL_CHECK(tweens.Kill(move, true) && Near(position, {2, 4, 6}, 1e-6f));
    Quat rotation{};
    const Quat target = Math::QuatFromAxisAngle({0, 1, 0}, Math::kHalfPi);
    const TweenHandle spin = tweens.Rotation({}, target, 1.0f, [&](Quat q) { rotation = q; }, Ease::Linear);
    tweens.SetLoops(spin, -1);
    ASTRAL_CHECK(tweens.TotalDuration(spin) < 0.0f);
    tweens.Update(10.5f);
    ASTRAL_CHECK(tweens.IsActive(spin));
    ASTRAL_CHECK(Near(Math::Rotate(rotation, {1, 0, 0}), Math::Rotate(Math::Slerp({}, target, 0.5f), {1, 0, 0}), 1e-4f));
    ASTRAL_CHECK(tweens.Kill(spin, true) && Near(Math::Rotate(rotation, {1, 0, 0}), Math::Rotate(target, {1, 0, 0}), 1e-5f));

    // Owners, callbacks that start tweens, and invalid input.
    int chained = 0;
    const TweenHandle first = tweens.Custom(0.1f, [](float) {});
    tweens.OnComplete(first, [&] { tweens.OnComplete(tweens.Custom(0.1f, [](float) {}), [&] { ++chained; }); });
    tweens.SetOwner(tweens.Custom(5.0f, [](float) {}), 42);
    tweens.SetOwner(tweens.Custom(5.0f, [](float) {}), 42);
    for (int i = 0; i < 5; ++i) tweens.Update(0.06f);
    ASTRAL_CHECK(chained == 1);
    ASTRAL_CHECK(tweens.KillOwner(42) == 2 && tweens.Count() == 0);
    ASTRAL_CHECK(tweens.Float(0, 1, 1, nullptr).IsNull());
    ASTRAL_CHECK(!tweens.SetLoops({999}, 2) && !tweens.Kill({999}));
    float instant = 0.0f;
    tweens.Float(0.0f, 3.0f, 0.0f, [&](float v) { instant = v; });
    tweens.Update(0.0f);
    ASTRAL_CHECK(instant == 3.0f && tweens.Count() == 0);
}

ASTRAL_TEST(SequencesAppendJoinInsertAndCallbacks) {
    TweenManager tweens;
    float a = 0.0f, b = 0.0f, c = 0.0f;
    std::vector<std::string> log;
    const TweenHandle sequence = tweens.Sequence();
    ASTRAL_CHECK(tweens.Append(sequence, tweens.Float(0, 1, 1.0f, [&](float v) { a = v; }, Ease::Linear)));
    const TweenHandle joined = tweens.Float(0, 2, 0.5f, [&](float v) { b = v; }, Ease::Linear);
    tweens.OnComplete(joined, [&] { log.push_back("joined"); });
    ASTRAL_CHECK(tweens.Join(sequence, joined));
    ASTRAL_CHECK(tweens.AppendInterval(sequence, 0.5f));
    ASTRAL_CHECK(tweens.AppendCallback(sequence, [&] { log.push_back("cue"); }));
    ASTRAL_CHECK(tweens.Append(sequence, tweens.Float(0, 3, 1.0f, [&](float v) { c = v; }, Ease::Linear)));
    ASTRAL_CHECK(tweens.InsertCallback(sequence, 0.0f, [&] { log.push_back("begin"); }));
    ASTRAL_CHECK_NEAR(tweens.TotalDuration(sequence), 2.5f, 1e-6);
    // Children belong to the sequence now.
    ASTRAL_CHECK(!tweens.Append(sequence, joined) && !tweens.Kill(joined));
    tweens.Update(0.25f);
    ASTRAL_CHECK_NEAR(a, 0.25f, 1e-5);
    ASTRAL_CHECK_NEAR(b, 1.0f, 1e-5);
    ASTRAL_CHECK(c == 0.0f && (log == std::vector<std::string>{"begin"}));
    tweens.Update(0.5f);
    ASTRAL_CHECK(b == 2.0f && log.back() == "joined");
    tweens.Update(1.0f); // 1.75: interval over, cue fired, third tween at 0.25
    ASTRAL_CHECK(a == 1.0f && log.back() == "cue");
    ASTRAL_CHECK_NEAR(c, 0.75f, 1e-5);
    int done = 0;
    tweens.OnComplete(sequence, [&] { ++done; });
    tweens.Update(1.0f);
    ASTRAL_CHECK(c == 3.0f && done == 1 && tweens.Count() == 0);
    ASTRAL_CHECK((log == std::vector<std::string>{"begin", "joined", "cue"}));

    // Looping sequences replay callbacks each lap; yoyo laps rewind children.
    log.clear();
    float x = 0.0f;
    const TweenHandle loop = tweens.Sequence();
    tweens.Append(loop, tweens.Float(0, 1, 1.0f, [&](float v) { x = v; }, Ease::Linear));
    tweens.InsertCallback(loop, 0.5f, [&] { log.push_back("tick"); });
    tweens.SetLoops(loop, 4, LoopMode::Yoyo);
    tweens.Update(1.25f); // lap 2 (reverse) at 0.25 from the end
    ASTRAL_CHECK_NEAR(x, 0.75f, 1e-5);
    ASTRAL_CHECK(log.size() == 1);
    tweens.Update(0.5f);
    ASTRAL_CHECK(log.size() == 2); // crossed 0.5 going backwards
    ASTRAL_CHECK_NEAR(x, 0.25f, 1e-5);
    tweens.Update(10.0f); // finishes, possibly skipping whole laps in one update
    ASTRAL_CHECK(log.size() == 4 && x == 0.0f && tweens.Count() == 0);
}

// ------------------------------------------------------------------ timelines

ASTRAL_TEST(TimelineCurvesInterpolateAsKeyed) {
    TimelineTrack track;
    track.kind = TrackKind::Float;
    track.property = "value";
    track.floatKeys = {{0.0f, 0.0f, KeyInterpolation::Linear, Ease::Linear}, {1.0f, 10.0f, KeyInterpolation::Constant, Ease::Linear},
        {2.0f, 20.0f, KeyInterpolation::Linear, Ease::InQuad}, {3.0f, 30.0f, KeyInterpolation::Linear, Ease::Linear}};
    ASTRAL_CHECK(track.EvaluateFloat(-1.0f) == 0.0f && track.EvaluateFloat(5.0f) == 30.0f);
    ASTRAL_CHECK_NEAR(track.EvaluateFloat(0.5f), 5.0f, 1e-5);
    ASTRAL_CHECK(track.EvaluateFloat(1.5f) == 10.0f); // constant segment holds
    ASTRAL_CHECK_NEAR(track.EvaluateFloat(2.5f), 22.5f, 1e-4); // InQuad eased segment
    ASTRAL_CHECK(track.EvaluateFloat(2.0f) == 20.0f);

    TimelineTrack path;
    path.kind = TrackKind::Vector;
    path.property = "position";
    for (int i = 0; i < 4; ++i) {
        path.vectorKeys.push_back({static_cast<float>(i), {static_cast<float>(i * i), 0.0f, static_cast<float>(i)}, KeyInterpolation::Cubic, Ease::Linear});
    }
    for (int i = 0; i < 4; ++i) ASTRAL_CHECK(Near(path.EvaluateVector(static_cast<float>(i)), path.vectorKeys[static_cast<std::size_t>(i)].value, 1e-5f));
    // Auto tangents are flat at the ends and smooth through interior keys.
    ASTRAL_CHECK(std::fabs(path.EvaluateVector(0.01f).x) < 0.01f);
    const float left = path.EvaluateVector(1.0f - 1e-3f).z, right = path.EvaluateVector(1.0f + 1e-3f).z;
    ASTRAL_CHECK_NEAR((right - left) / 2e-3f, 1.0f, 0.05f);

    TimelineTrack turn;
    turn.kind = TrackKind::Rotation;
    turn.property = "rotation";
    const Quat quarter = Math::QuatFromAxisAngle({0, 1, 0}, Math::kHalfPi);
    turn.rotationKeys = {{0.0f, {}, KeyInterpolation::Linear, Ease::Linear}, {2.0f, quarter, KeyInterpolation::Linear, Ease::Linear}};
    const Vec3 halfway = Math::Rotate(turn.EvaluateRotation(1.0f), {1, 0, 0});
    ASTRAL_CHECK(Near(halfway, Math::Rotate(Math::QuatFromAxisAngle({0, 1, 0}, Math::kHalfPi * 0.5f), {1, 0, 0}), 1e-5f));

    TimelineTrack door;
    door.kind = TrackKind::Activation;
    door.ranges = {{1.0f, 2.0f}, {3.0f, 3.5f}};
    ASTRAL_CHECK(!door.IsActive(0.5f) && door.IsActive(1.0f) && !door.IsActive(2.0f) && door.IsActive(3.2f));
}

ASTRAL_TEST(TimelinePlayerWrapsScrubsAndFiresEvents) {
    auto asset = EventTimeline();
    RecordingBinder binder;
    TimelinePlayer player(asset, &binder);
    player.Play();
    ASTRAL_CHECK((binder.events == std::vector<std::string>{"start"}));
    for (int i = 0; i < 50; ++i) player.Update(0.1f);
    ASTRAL_CHECK((binder.events == std::vector<std::string>{"start", "one", "mid", "end"}));
    ASTRAL_CHECK(player.Finished() && !player.IsPlaying() && player.Time() == 4.0f && binder.lastFloat == 8.0f);
    // Replaying restarts; seeking evaluates without events unless asked.
    binder.events.clear();
    player.Seek(2.0f);
    ASTRAL_CHECK(binder.events.empty() && binder.lastFloat == 4.0f);
    player.Seek(3.0f, true);
    ASTRAL_CHECK((binder.events == std::vector<std::string>{"mid"}));
    player.Seek(0.5f, true); // backwards across "mid" and "one"
    ASTRAL_CHECK((binder.events == std::vector<std::string>{"mid", "mid", "one"}));
    player.Stop();
    ASTRAL_CHECK(player.Time() == 0.0f && !player.IsPlaying() && binder.lastFloat == 0.0f);

    // Looping fires every lap (including the lap's first instant) in order.
    binder.events.clear();
    player.wrap = TimelineWrap::Loop;
    player.Play();
    player.Update(9.0f); // two full laps + 1 s
    ASTRAL_CHECK((binder.events == std::vector<std::string>{"start", "one", "mid", "end", "start", "one", "mid", "end", "start", "one"}));
    ASTRAL_CHECK_NEAR(player.Time(), 1.0f, 1e-5);
    // Reverse playback fires events backwards.
    binder.events.clear();
    player.wrap = TimelineWrap::Once;
    player.speed = -1.0f;
    player.Seek(3.0f);
    player.Update(10.0f);
    ASTRAL_CHECK((binder.events == std::vector<std::string>{"mid", "one", "start"}));
    ASTRAL_CHECK(player.Finished() && player.Time() == 0.0f);
    // Ping-pong bounces off the end.
    binder.events.clear();
    player.speed = 1.0f;
    player.wrap = TimelineWrap::PingPong;
    player.Play();
    player.Update(5.0f);
    ASTRAL_CHECK_NEAR(player.Time(), 3.0f, 1e-5);
    ASTRAL_CHECK(binder.events.back() == "end" || binder.events.back() == "mid");
    // Hold keeps the final pose playing.
    TimelinePlayer hold(asset, &binder);
    hold.wrap = TimelineWrap::Hold;
    hold.Play();
    hold.Update(100.0f);
    ASTRAL_CHECK(hold.Finished() && hold.IsPlaying() && hold.Time() == 4.0f);
}

ASTRAL_TEST(TimelineJsonRoundTripsAndValidates) {
    const char* text = R"({
      "name": "Intro", "duration": 5,
      "tracks": [
        {"type": "vector", "binding": "Camera", "property": "position",
         "keys": [{"t": 2, "v": [0, 4, -2], "interp": "cubic"}, {"t": 0, "v": [0, 2, -8], "interp": "cubic", "ease": "OutQuad"}]},
        {"type": "rotation", "binding": "Camera", "property": "rotation",
         "keys": [{"t": 0, "v": {"euler": [10, 0, 0]}}, {"t": 5, "v": [0, 0.7071068, 0, 0.7071068]}]},
        {"type": "float", "binding": "Sun", "property": "light.intensity", "keys": [{"t": 0, "v": 0.5}, {"t": 5, "v": 3}]},
        {"type": "event", "binding": "", "events": [{"t": 1.5, "name": "Shake", "payload": "0.4"}]},
        {"type": "activation", "binding": "Door", "ranges": [[1, 2.5]], "muted": true}
      ]})";
    TimelineAsset asset;
    std::string error;
    ASTRAL_CHECK(asset.FromJson(ParseText(text), error));
    ASTRAL_CHECK(asset.tracks.size() == 5 && asset.tracks[0].vectorKeys[0].time == 0.0f); // keys sorted
    ASTRAL_CHECK(asset.tracks[0].vectorKeys[0].ease == Ease::OutQuad && asset.tracks[4].muted);
    const std::string saved = Core::WriteJson(asset.ToJson());
    TimelineAsset reloaded;
    ASTRAL_CHECK(reloaded.FromJson(ParseText(saved), error));
    ASTRAL_CHECK(Core::WriteJson(reloaded.ToJson()) == saved);

    const char* bad[] = {
        R"({"duration": 1, "tracks": [{"type": "spline"}]})",
        R"({"duration": 0})",
        R"({"duration": 1, "tracks": [{"type": "float", "binding": "x", "keys": [{"t": 0, "v": 1}]}]})",
        R"({"duration": 1, "tracks": [{"type": "float", "property": "p", "keys": [{"t": 0, "v": "big"}]}]})",
        R"({"duration": 1, "tracks": [{"type": "float", "property": "p", "keys": [{"t": 0, "v": 1, "interp": "bezier"}]}]})",
        R"({"duration": 1, "tracks": [{"type": "activation", "ranges": [[2, 1]]}]})",
        R"({"duration": 1, "tracks": [{"type": "rotation", "property": "r", "keys": [{"t": 0, "v": [0, 0, 0, 0]}]}]})",
        R"({"duration": 1, "extra": true})",
    };
    for (const char* json : bad) {
        TimelineAsset rejected;
        ASTRAL_CHECK(!rejected.FromJson(ParseText(json), error) && !error.empty());
    }
}

ASTRAL_TEST(TimelinesDriveTheGameWorld) {
    Framework::RegisterFrameworkBehaviours();
    Framework::GameWorld world;
    const Framework::Entity camera = world.CreateEntity("Camera");
    world.Add<Framework::Camera>(camera);
    const Framework::Entity lamp = world.CreateEntity("Lamp");
    world.Add<Framework::Light>(lamp);
    const Framework::Entity door = world.CreateEntity("Door");
    world.Add<Framework::MeshRenderer>(door);

    auto asset = std::make_shared<TimelineAsset>();
    asset->duration = 2.0f;
    TimelineTrack move;
    move.kind = TrackKind::Vector;
    move.binding = "Camera";
    move.property = "position";
    move.vectorKeys = {{0.0f, {0, 1, -10}, KeyInterpolation::Linear, Ease::Linear}, {2.0f, {0, 3, -2}, KeyInterpolation::Linear, Ease::Linear}};
    TimelineTrack glow;
    glow.kind = TrackKind::Float;
    glow.binding = "Lamp";
    glow.property = "light.intensity";
    glow.floatKeys = {{0.0f, 0.0f, KeyInterpolation::Linear, Ease::Linear}, {2.0f, 6.0f, KeyInterpolation::Linear, Ease::Linear}};
    TimelineTrack open;
    open.kind = TrackKind::Activation;
    open.binding = "Door";
    open.ranges = {{0.0f, 1.0f}};
    TimelineTrack cues;
    cues.kind = TrackKind::Event;
    cues.events = {{1.5f, "Shake", "0.4"}};
    TimelineTrack ghost;
    ghost.kind = TrackKind::Float;
    ghost.binding = "Nobody";
    ghost.property = "light.intensity";
    ghost.floatKeys = {{0.0f, 1.0f, KeyInterpolation::Linear, Ease::Linear}};
    asset->tracks = {move, glow, open, cues, ghost};
    std::string error;
    ASTRAL_CHECK(asset->Validate(error));

    const Framework::Entity director = world.CreateEntity("Director");
    auto& sequence = world.AddBehaviour<Framework::TimelineBehaviour>(director);
    sequence.timeline = asset;
    std::vector<std::string> shakes;
    auto subscription = world.Events().SubscribeScoped<Framework::TimelineNotification>(
        [&](const Framework::TimelineNotification& n) { shakes.push_back(n.name + ":" + n.payload); });
    world.Tick(1.0f / 60.0f); // OnStart seeks to 0 and plays
    ASTRAL_CHECK(world.Get<Framework::MeshRenderer>(door)->visible);
    for (int i = 0; i < 60; ++i) world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK(Near(world.GetLocal(camera).translation, {0, 2, -6}, 0.1f));
    ASTRAL_CHECK_NEAR(world.Get<Framework::Light>(lamp)->intensity, 3.0f, 0.1f);
    ASTRAL_CHECK(!world.Get<Framework::MeshRenderer>(door)->visible);
    ASTRAL_CHECK(shakes.empty());
    for (int i = 0; i < 90; ++i) world.Tick(1.0f / 60.0f);
    ASTRAL_CHECK((shakes == std::vector<std::string>{"Shake:0.4"}));
    ASTRAL_CHECK(Near(world.GetLocal(camera).translation, {0, 3, -2}, 1e-4f));
    ASTRAL_CHECK(sequence.Player()->Finished());

    // Timeline assets load through the asset manager by name from scenes.
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "astral_timeline_assets";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    std::ofstream(root / "intro.json") << Core::WriteJson(asset->ToJson());
    Assets::AssetManager assets(root.string(), nullptr);
    assets.RegisterDefaultLoaders();
    Framework::GameWorld scripted({}, nullptr, &assets);
    const Framework::Entity scriptedCamera = scripted.CreateEntity("Camera");
    JsonValue properties = JsonValue::MakeObject();
    properties.Set("asset", "intro.json");
    properties.Set("wrap", "Loop");
    Framework::Behaviour* loaded = scripted.AddBehaviour(scripted.CreateEntity("Director"), "Timeline", properties, error);
    ASTRAL_CHECK(loaded);
    for (int i = 0; i < 150; ++i) scripted.Tick(1.0f / 60.0f); // 2.5 s: second lap at 0.5 s
    ASTRAL_CHECK(Near(scripted.GetLocal(scriptedCamera).translation, {0, 1.5f, -8}, 0.1f));
    auto* timeline = dynamic_cast<Framework::TimelineBehaviour*>(loaded);
    ASTRAL_CHECK(timeline && timeline->error.empty() && timeline->Player()->IsPlaying());
    properties.Set("asset", "missing.json");
    auto* broken = dynamic_cast<Framework::TimelineBehaviour*>(scripted.AddBehaviour(scripted.CreateEntity("Broken"), "Timeline", properties, error));
    scripted.Tick(1.0f / 60.0f);
    ASTRAL_CHECK(broken && !broken->error.empty() && !broken->Player());
    std::filesystem::remove_all(root);
}

ASTRAL_TEST_MAIN("EngineTimelineTests")
