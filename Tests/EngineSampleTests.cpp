// Game host and the Playground sample project: project files and input maps,
// scene loading and switching, hot reload, the developer console, HUD
// rendering, and the sample's gameplay played through the real host.

#include "Engine/Framework/Components.h"
#include "Engine/Framework/GameHost.h"
#include "Engine/Framework/Sequencer.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/UI/Widgets.h"
#include "Game/Samples/Playground/PlaygroundBehaviours.h"
#include "Tests/EngineTestSupport.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

using namespace Astral;
using Framework::Entity;
using Framework::GameHost;
using Framework::HostInput;
using Math::Vec3;

namespace {

constexpr float kFrame = 1.0f / 60.0f;

#ifndef ASTRAL_SOURCE_ROOT
#define ASTRAL_SOURCE_ROOT "."
#endif

std::string PlaygroundRoot() { return std::string(ASTRAL_SOURCE_ROOT) + "/Content/Samples/Playground"; }

Core::JsonValue Parse(const std::string& text) {
    Core::JsonValue json;
    std::string error;
    if (!Core::ParseJson(text, json, error)) std::fprintf(stderr, "bad test JSON: %s\n", error.c_str());
    return json;
}

std::unique_ptr<GameHost> MakeHost(const std::string& root, bool hotReload = false) {
    Samples::RegisterPlaygroundBehaviours();
    Framework::HostSettings settings;
    settings.contentRoot = root;
    settings.workerThreads = 0; // deterministic and sanitizer-friendly
    settings.hotReload = hotReload;
    settings.hotReloadInterval = 0.1f;
    return std::make_unique<GameHost>(settings);
}

std::unique_ptr<GameHost> LoadPlayground() {
    auto host = MakeHost(PlaygroundRoot());
    std::string error;
    if (!host->LoadProject("project.json", error)) std::fprintf(stderr, "playground failed to load: %s\n", error.c_str());
    return host;
}

HostInput Hold(std::initializer_list<const char*> keys) {
    HostInput input;
    for (const char* key : keys) input.keys.keys.set(Input::KeyFromName(key));
    return input;
}

void Run(GameHost& host, int frames, const HostInput& input = {}) {
    for (int i = 0; i < frames; ++i) host.Update(kFrame, input);
}

Vec3 PositionOf(GameHost& host, const char* name) {
    const Entity entity = host.World().Find(name);
    return entity.IsNull() ? Vec3{-1.0e9f, -1.0e9f, -1.0e9f} : host.World().GetWorld(entity).translation;
}

Samples::GameMode* Mode(GameHost& host) {
    const Entity entity = host.World().Find("GameMode");
    return entity.IsNull() ? nullptr : host.World().GetBehaviour<Samples::GameMode>(entity);
}

std::string TempDir(const char* name) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(path);
    std::filesystem::create_directories(path);
    return path.string();
}

void WriteText(const std::string& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file << text;
}

// Moves a file's timestamp forward so change polling sees an edit even on
// file systems with coarse timestamps.
void Touch(const std::string& path, int seconds) {
    std::filesystem::last_write_time(path, std::filesystem::last_write_time(path) + std::chrono::seconds(seconds));
}

} // namespace

ASTRAL_TEST(KeyNamesRoundTrip) {
    ASTRAL_CHECK(Input::KeyFromName("W") == 'W' && Input::KeyFromName("w") == 'W' && Input::KeyFromName("7") == '7');
    ASTRAL_CHECK(Input::KeyFromName("Space") == Input::Keys::Space && Input::KeyFromName("SPACE") == Input::Keys::Space);
    ASTRAL_CHECK(Input::KeyFromName("F1") == Input::Keys::F1 && Input::KeyFromName("F12") == Input::Keys::F1 + 11);
    ASTRAL_CHECK(Input::KeyFromName("F13") == 0 && Input::KeyFromName("Jump") == 0 && Input::KeyFromName("") == 0);
    for (int key = 1; key < 256; ++key) {
        const std::string name = Input::KeyName(static_cast<std::uint16_t>(key));
        if (!name.empty()) ASTRAL_CHECK(Input::KeyFromName(name) == key);
    }
    ASTRAL_CHECK(Input::KeyName(Input::Keys::Escape) == "Escape" && Input::KeyName(0xC0).empty());
}

ASTRAL_TEST(ProjectFilesParseAndReportErrors) {
    Framework::ProjectDesc project;
    std::string error;
    ASTRAL_CHECK(Framework::ParseProject(Parse(R"({
        "format": "astral-project", "version": 1, "name": "Demo", "startupScene": "a.json",
        "config": {"r.Quality": 1, "host.showStats": true},
        "input": {"contexts": [{"name": "Gameplay", "priority": 2, "consumesKeys": true, "actions": [
            {"name": "Move", "type": "Axis2D", "bindings": [{"key": "W", "axis": [0, 1]}, {"key": 40, "axis": [0, -1]}]},
            {"name": "Fire", "bindings": [{"key": "MouseLeft"}], "bufferSeconds": 0.2}]}]}})"), project, error));
    ASTRAL_CHECK(project.name == "Demo" && project.startupScene == "a.json");
    ASTRAL_CHECK(project.config.size() == 2 && project.config[0].second == "1" && project.config[1].second == "1");
    ASTRAL_CHECK(project.input.size() == 1 && project.input[0].priority == 2 && project.input[0].consumesKeys);
    ASTRAL_CHECK(project.input[0].actions.size() == 2);
    ASTRAL_CHECK(project.input[0].actions[0].type == Input::ActionType::Axis2D);
    ASTRAL_CHECK(project.input[0].actions[0].bindings[1].key == 40);
    ASTRAL_CHECK(project.input[0].actions[1].bufferSeconds == 0.2f);

    const std::pair<const char*, const char*> bad[] = {
        {R"({"format": "astral-scene", "startupScene": "a.json"})", "format"},
        {R"({"format": "astral-project"})", "startupScene"},
        {R"({"format": "astral-project", "startupScene": "a.json", "extra": 1})", "extra: unknown key"},
        {R"({"format": "astral-project", "version": 9, "startupScene": "a.json"})", "version"},
        {R"({"format": "astral-project", "startupScene": "a.json", "input": {"contexts": [{"name": "G", "actions": [
            {"name": "Jump", "bindings": [{"key": "Spacebar"}]}]}]}})", "input.contexts[0].actions[0].bindings[0].key"},
        {R"({"format": "astral-project", "startupScene": "a.json", "input": {"contexts": [{"name": "G", "actions": [
            {"name": "Move", "type": "Axis2D", "bindings": [{"key": "W"}]}]}]}})", "axis"},
        {R"({"format": "astral-project", "startupScene": "a.json", "input": {"contexts": [{"name": "G", "actions": [
            {"name": "Jump", "bindings": [{"key": "Space"}]}, {"name": "Jump", "bindings": [{"key": "J"}]}]}]}})", "duplicate action"},
        {R"({"format": "astral-project", "startupScene": "a.json", "input": {"contexts": [{"name": "G", "actions": [
            {"name": "Jump", "type": "Trigger", "bindings": [{"key": "Space"}]}]}]}})", "type"},
        {R"({"format": "astral-project", "startupScene": "a.json", "config": {"r.Quality": [1]}})", "config.r.Quality"},
    };
    for (const auto& [text, expected] : bad) {
        Framework::ProjectDesc rejected;
        error.clear();
        const bool ok = Framework::ParseProject(Parse(text), rejected, error);
        if (ok || error.find(expected) == std::string::npos) std::fprintf(stderr, "  expected '%s' in '%s'\n", expected, error.c_str());
        ASTRAL_CHECK(!ok && error.find(expected) != std::string::npos);
    }
}

ASTRAL_TEST(PlaygroundLoadsWithInputHudAndStars) {
    auto host = LoadPlayground();
    ASTRAL_CHECK(host->LastError().empty());
    ASTRAL_CHECK(host->ScenePath() == "scenes/playground.scene.json" && host->Project().name == "Playground");
    ASTRAL_CHECK(!host->World().Find("Player").IsNull() && !host->World().Find("Camera").IsNull());
    ASTRAL_CHECK(host->Commands().Find("r.Quality")->GetInt() == 2);
    Run(*host, 2);
    Samples::GameMode* mode = Mode(*host);
    ASTRAL_CHECK(mode && mode->Total() == 5 && mode->Collected() == 0);
    ASTRAL_CHECK(mode && mode->StatusLabel() && mode->StatusLabel()->text == "Stars 0 / 5");
    // The star prefab file brought its material and LODs along.
    const Entity star = host->World().Find("Star1");
    const auto* renderer = host->World().Get<Framework::MeshRenderer>(star);
    ASTRAL_CHECK(renderer && renderer->material && renderer->material->name == "star" && renderer->lods && renderer->lods->Count() > 1);
    // Spinner and Bobber animate the stars.
    const Math::TRS before = host->World().GetLocal(star);
    Run(*host, 15);
    const Math::TRS after = host->World().GetLocal(star);
    ASTRAL_CHECK(std::fabs(after.translation.y - before.translation.y) > 1.0e-3f);
    ASTRAL_CHECK(std::fabs(Math::Dot(after.rotation, before.rotation)) < 0.9999f);
}

ASTRAL_TEST(PlayerWalksJumpsAndPushesCrates) {
    auto host = LoadPlayground();
    Run(*host, 30); // settle on the floor
    const Vec3 start = PositionOf(*host, "Player");
    ASTRAL_CHECK(std::fabs(start.y) < 0.05f);
    // W walks away from the camera (+Z), D strafes right (+X).
    Run(*host, 60, Hold({"W"}));
    const Vec3 walked = PositionOf(*host, "Player");
    ASTRAL_CHECK(walked.z - start.z > 3.0f && std::fabs(walked.x - start.x) < 0.3f);
    // The player turned to face the walking direction.
    const Vec3 facing = Math::Rotate(host->World().GetWorld(host->World().Find("Player")).rotation, {0, 0, 1});
    ASTRAL_CHECK(facing.z > 0.95f);
    // Walking back toward the camera (open floor); sprinting covers more ground.
    Run(*host, 20);
    const Vec3 a = PositionOf(*host, "Player");
    Run(*host, 30, Hold({"S"}));
    const Vec3 b = PositionOf(*host, "Player");
    Run(*host, 30, Hold({"S", "Shift"}));
    const Vec3 c = PositionOf(*host, "Player");
    if (!(a.z - b.z > 1.5f && (b.z - c.z) > (a.z - b.z) * 1.3f)) {
        std::fprintf(stderr, "  walk %.3f sprint %.3f\n", static_cast<double>(a.z - b.z), static_cast<double>(b.z - c.z));
    }
    ASTRAL_CHECK(a.z - b.z > 1.5f && (b.z - c.z) > (a.z - b.z) * 1.3f);
    // Jump (buffered press) leaves the ground and lands again.
    Run(*host, 20);
    HostInput jump = Hold({"Space"});
    host->Update(kFrame, jump);
    float peak = PositionOf(*host, "Player").y;
    for (int i = 0; i < 90; ++i) {
        host->Update(kFrame, {});
        peak = std::max(peak, PositionOf(*host, "Player").y);
    }
    auto* controller = host->World().GetBehaviour<Samples::PlayerController>(host->World().Find("Player"));
    ASTRAL_CHECK(controller && controller->Jumps() == 1);
    ASTRAL_CHECK(peak > 1.0f && std::fabs(PositionOf(*host, "Player").y) < 0.05f);
    // The camera follows behind the player and looks at it.
    const Vec3 player = PositionOf(*host, "Player");
    const Vec3 camera = PositionOf(*host, "Camera");
    ASTRAL_CHECK(camera.z < player.z - 3.0f && camera.y > player.y + 1.0f);
    // Walking into the crate stack pushes a crate.
    host->World().SetWorldPosition(host->World().Find("Player"), {-3.0f, 0.05f, -2.0f});
    Run(*host, 5);
    const Vec3 crateBefore = PositionOf(*host, "Crate1");
    Run(*host, 60, Hold({"W"}));
    const Vec3 crateAfter = PositionOf(*host, "Crate1");
    ASTRAL_CHECK(Math::Length(crateAfter - crateBefore) > 0.2f);
}

ASTRAL_TEST(CollectingEveryStarWinsOpensTheGateAndRestarts) {
    auto host = LoadPlayground();
    Run(*host, 10);
    Samples::GameMode* mode = Mode(*host);
    ASTRAL_CHECK(mode != nullptr);
    if (!mode) return;
    const Entity player = host->World().Find("Player");
    const float gateStart = PositionOf(*host, "Gate").y;
    for (const char* name : {"Star1", "Star2", "Star3", "Star4", "Star5"}) {
        const Vec3 star = PositionOf(*host, name);
        host->World().SetWorldPosition(player, star - Vec3{0.0f, 0.9f, 0.0f});
        Run(*host, 3);
        ASTRAL_CHECK(host->World().Find(name).IsNull()); // collected and destroyed
    }
    ASTRAL_CHECK(mode->Collected() == 5 && mode->Score() == 5 && mode->Won());
    ASTRAL_CHECK(mode->StatusLabel()->text == "Stars 5 / 5");
    ASTRAL_CHECK(mode->Banner()->visible && mode->Banner()->text.find("All 5 collected") == 0);
    // The win timeline raises the gate.
    Run(*host, 180);
    ASTRAL_CHECK(PositionOf(*host, "Gate").y > gateStart + 2.5f);
    ASTRAL_CHECK(host->Stats().sceneLoads == 1);
    // After the restart delay the scene reloads (a LoadSceneRequest from the GameMode).
    Run(*host, 90);
    ASTRAL_CHECK(host->Stats().sceneLoads == 2 && host->LastError().empty());
    Run(*host, 2);
    Samples::GameMode* fresh = Mode(*host);
    ASTRAL_CHECK(fresh && fresh->Total() == 5 && fresh->Collected() == 0 && !fresh->Won());
    ASTRAL_CHECK(!host->World().Find("Star1").IsNull());
    // The old scene's HUD widgets are gone; the new GameMode owns the only ones.
    int panels = 0;
    for (const auto& child : host->Hud().Children()) {
        if (!child->IsPendingRemoval() && child->name == "GameMode.panel") ++panels;
    }
    ASTRAL_CHECK(panels == 1);
}

ASTRAL_TEST(JumpPadLaunchesTheCharacter) {
    auto host = LoadPlayground();
    Run(*host, 10);
    const Entity player = host->World().Find("Player");
    host->World().SetWorldPosition(player, {-8.0f, 0.05f, 4.5f});
    float peak = 0.0f;
    for (int i = 0; i < 60; ++i) {
        host->Update(kFrame, {});
        peak = std::max(peak, PositionOf(*host, "Player").y);
    }
    auto* pad = host->World().GetBehaviour<Samples::JumpPad>(host->World().Find("JumpPad"));
    ASTRAL_CHECK(pad && pad->Launches() >= 1);
    ASTRAL_CHECK(peak > 4.5f); // 16 m/s against -24 m/s^2 rises about 5.3 m
}

ASTRAL_TEST(ConsoleCommandsDriveTheHost) {
    auto host = LoadPlayground();
    Run(*host, 10);
    // The console takes the keyboard: holding W does not move the player.
    HostInput toggle;
    toggle.ui.keysPressed.push_back(host->Console().toggleKey);
    host->Update(kFrame, toggle);
    ASTRAL_CHECK(host->Console().IsOpen() && host->UI().WantsKeyboard());
    const Vec3 before = PositionOf(*host, "Player");
    Run(*host, 30, Hold({"W"}));
    ASTRAL_CHECK(Math::Length(PositionOf(*host, "Player") - before) < 0.05f);
    host->Update(kFrame, toggle);
    ASTRAL_CHECK(!host->Console().IsOpen());

    const std::size_t entities = host->World().Stats().entities;
    host->Console().Submit("spawn Crate 0 4 0");
    Run(*host, 1);
    ASTRAL_CHECK(host->World().Stats().entities == entities + 1);
    ASTRAL_CHECK(host->Commands().Execute("spawn Teapot").find("unknown prefab") != std::string::npos);
    ASTRAL_CHECK(host->Commands().Execute("entities").find(std::to_string(entities + 1) + " entities") == 0);

    host->Commands().Execute("pause");
    const std::uint64_t frame = host->World().Frame();
    Run(*host, 5);
    ASTRAL_CHECK(host->Paused() && host->World().Frame() == frame);
    host->Commands().Execute("pause");
    Run(*host, 1);
    ASTRAL_CHECK(host->World().Frame() == frame + 1);

    host->Commands().Execute("slomo 0.5");
    ASTRAL_CHECK(host->World().Settings().timeScale == 0.5f);
    ASTRAL_CHECK(host->Commands().Execute("slomo fast").find("must be a number") != std::string::npos);

    host->Commands().Execute("r.Quality 0");
    ASTRAL_CHECK(!host->Renderer().Settings().bloom);
    host->Commands().Execute("r.Quality 3");
    ASTRAL_CHECK(host->Renderer().Settings().bloom);

    // A missing scene is reported and the running scene keeps going.
    host->Commands().Execute("open scenes/missing.scene.json");
    Run(*host, 1);
    ASTRAL_CHECK(!host->LastError().empty() && !host->World().Find("Player").IsNull());
    ASTRAL_CHECK(host->ScenePath() == "scenes/playground.scene.json");
    host->Commands().Execute("restart");
    Run(*host, 1);
    ASTRAL_CHECK(host->Stats().sceneLoads == 2 && host->LastError().empty());
    Run(*host, 1); // world stats are gathered while ticking
    ASTRAL_CHECK(host->World().Stats().entities == entities); // the spawned crate went with the old world

    host->Commands().Execute("quit");
    ASTRAL_CHECK(host->QuitRequested());
}

ASTRAL_TEST(RenderedFramesShowTheWorldAndHud) {
    auto host = LoadPlayground();
    Run(*host, 5);
    const Graphics::ImageRgba8& frame = host->Render(160, 90);
    ASTRAL_CHECK(frame.width == 160 && frame.height == 90);
    ASTRAL_CHECK(host->Stats().render.drawsVisible > 5 && host->Stats().render.trianglesRasterized > 100);
    const std::uint64_t withHud = Graphics::HashImage(frame);
    // Hiding the HUD changes only the HUD corner.
    host->Hud().visible = false;
    const Graphics::ImageRgba8& plain = host->Render(160, 90);
    ASTRAL_CHECK(Graphics::HashImage(plain) != withHud);
    host->Hud().visible = true;
    // The stats overlay draws too.
    host->Commands().Execute("stat");
    const std::uint64_t stats = Graphics::HashImage(host->Render(160, 90));
    ASTRAL_CHECK(stats != withHud);

    // A scene without a camera still renders (a placeholder).
    auto empty = MakeHost(PlaygroundRoot());
    std::string error;
    ASTRAL_CHECK(empty->LoadSceneJson(Parse(R"({"entities": [{"name": "Lonely"}]})"), error));
    const Graphics::ImageRgba8& placeholder = empty->Render(64, 32);
    ASTRAL_CHECK(placeholder.width == 64 && placeholder.height == 32);
}

ASTRAL_TEST(HotReloadPicksUpSceneEdits) {
    const std::string root = TempDir("astral_host_hot_reload");
    const std::string scene = root + "/level.scene.json";
    WriteText(scene, R"({"entities": [{"name": "First"}]})");
    auto host = MakeHost(root, true);
    std::string error;
    ASTRAL_CHECK(host->LoadScene("level.scene.json", error));
    ASTRAL_CHECK(!host->World().Find("First").IsNull());
    WriteText(scene, R"({"entities": [{"name": "Second"}, {"name": "Third"}]})");
    Touch(scene, 5);
    for (int i = 0; i < 12; ++i) host->Update(0.02f, {});
    ASTRAL_CHECK(host->Stats().hotReloads == 1);
    ASTRAL_CHECK(host->World().Find("First").IsNull() && !host->World().Find("Third").IsNull());
    // A broken edit is reported once and the running scene survives.
    WriteText(scene, R"({"entities": [{"name": "Broken", "components": {"Nope": {}}}]})");
    Touch(scene, 10);
    for (int i = 0; i < 12; ++i) host->Update(0.02f, {});
    if (host->Stats().hotReloads != 1 || host->LastError().empty()) {
        std::fprintf(stderr, "  hot reloads %zu, last error '%s'\n", host->Stats().hotReloads, host->LastError().c_str());
    }
    ASTRAL_CHECK(host->Stats().hotReloads == 1 && !host->LastError().empty());
    ASTRAL_CHECK(!host->World().Find("Third").IsNull());
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(ScenesSwitchThroughLoadRequests) {
    const std::string root = TempDir("astral_host_switch");
    WriteText(root + "/a.scene.json", R"({"entities": [{"name": "InA"}]})");
    WriteText(root + "/b.scene.json", R"({"settings": {"timeScale": 0.5}, "entities": [{"name": "InB"}]})");
    auto host = MakeHost(root);
    std::string error;
    ASTRAL_CHECK(host->LoadScene("a.scene.json", error));
    // Queued from gameplay code (a behaviour would do this); applied after the frame.
    host->World().Events().Queue(Framework::LoadSceneRequest{"b.scene.json"});
    ASTRAL_CHECK(!host->World().Find("InA").IsNull());
    host->Update(kFrame, {});
    ASTRAL_CHECK(host->ScenePath() == "b.scene.json" && !host->World().Find("InB").IsNull());
    ASTRAL_CHECK(host->World().Settings().timeScale == 0.5f);
    // An empty path reloads the current scene.
    host->World().Events().Queue(Framework::LoadSceneRequest{});
    host->Update(kFrame, {});
    ASTRAL_CHECK(host->Stats().sceneLoads == 3 && host->ScenePath() == "b.scene.json");
    host->World().Events().Queue(Framework::QuitRequest{});
    host->Update(kFrame, {});
    ASTRAL_CHECK(host->QuitRequested());
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(PlaythroughIsDeterministic) {
    auto run = [] {
        auto host = LoadPlayground();
        for (int i = 0; i < 150; ++i) {
            HostInput input = Hold({i < 60 ? "W" : "D"});
            if (i == 70) input.keys.keys.set(Input::Keys::Space);
            host->Update(kFrame, input);
        }
        const Vec3 p = PositionOf(*host, "Player");
        return std::vector<float>{p.x, p.y, p.z, PositionOf(*host, "Crate3").y, PositionOf(*host, "Camera").x};
    };
    const std::vector<float> first = run();
    const std::vector<float> second = run();
    ASTRAL_CHECK(first == second);
}

ASTRAL_TEST_MAIN("EngineSampleTests")
