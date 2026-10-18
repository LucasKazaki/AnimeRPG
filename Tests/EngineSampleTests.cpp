// Game host and the Playground sample project: project files and input maps,
// scene loading and switching, hot reload, the developer console, HUD
// rendering, and the sample's gameplay played through the real host.

#include "Engine/Framework/Components.h"
#include "Engine/Assets/Gltf.h"
#include "Engine/Core/Base64.h"
#include "Engine/Framework/GameHost.h"
#include "Engine/Framework/Sequencer.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/UI/Widgets.h"
#include "Game/Samples/Playground/PlaygroundBehaviours.h"
#include "Tests/EngineTestSupport.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <limits>
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

void WriteReloadBytes(const std::string& path, const std::vector<std::uint8_t>& bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

Assets::GltfDocument ReloadModel(float extent = 1.0f) {
    Assets::GltfDocument model;
    Assets::GltfPrimitive primitive;
    Graphics::MeshBuilder(primitive.mesh).AddBox({}, {extent, 0.5f, 0.5f});
    primitive.mesh.ComputeBounds();
    primitive.material = 0;
    model.meshes.push_back({"Fixture", {primitive}});
    Assets::GltfNode node;
    node.name = "Imported";
    node.mesh = 0;
    model.nodes.push_back(node);
    model.scenes.push_back({"Fixture", {0}});
    model.scene = 0;
    Assets::GltfImage image;
    image.image.Resize(2, 2, {255, 0, 0, 255});
    model.images.push_back(image);
    model.textures.push_back({"Texture", 0, false});
    Assets::GltfMaterial material;
    material.baseColorTexture = 0;
    material.unlit = true;
    model.materials.push_back(material);
    return model;
}

struct ReloadExternalModel {
    Core::JsonValue json;
    std::vector<std::uint8_t> buffer;
    std::vector<std::uint8_t> image;
};

ReloadExternalModel ExternalReloadModel(float extent = 1.0f) {
    std::string error;
    const auto bytes = Assets::ExportGltfToMemory(ReloadModel(extent), Assets::GltfContainer::Embedded, error);
    ASTRAL_CHECK(!bytes.empty());
    ReloadExternalModel out;
    ASTRAL_CHECK(Core::ParseJson(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()), out.json, error));
    auto& buffer = out.json.Find("buffers")->Items()[0];
    auto& image = out.json.Find("images")->Items()[0];
    const std::string bufferUri = buffer.String("uri"), imageUri = image.String("uri");
    ASTRAL_CHECK(Core::Base64Decode(std::string_view(bufferUri).substr(bufferUri.find(',') + 1), out.buffer));
    ASTRAL_CHECK(Core::Base64Decode(std::string_view(imageUri).substr(imageUri.find(',') + 1), out.image));
    buffer.Set("uri", "mesh.bin");
    image.Set("uri", "image.png");
    return out;
}

const Framework::MeshRenderer* ReloadMesh(GameHost& host) {
    const auto entity = host.World().Find("Imported");
    ASTRAL_CHECK(!entity.IsNull());
    const auto* mesh = host.World().Get<Framework::MeshRenderer>(entity);
    ASTRAL_CHECK(mesh && mesh->mesh && mesh->material);
    return mesh;
}

float ReloadMeshExtent(GameHost& host) {
    float extent = 0;
    for (const auto& vertex : ReloadMesh(host)->mesh->vertices) extent = std::max(extent, std::fabs(vertex.position.x));
    return extent;
}

Core::JsonValue NumericInputMap(double x = 1.0, double y = 0.0) {
    Core::JsonValue json = Parse(R"({"contexts":[{"name":"Numeric","actions":[
        {"name":"Move","type":"Axis2D","bindings":[{"key":"W","axis":[1,0]}]}]}]})");
    auto& axis = json.Find("contexts")->Items()[0].Find("actions")->Items()[0].Find("bindings")->Items()[0].Find("axis")->Items();
    axis[0] = Core::JsonValue(x);
    axis[1] = Core::JsonValue(y);
    return json;
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

ASTRAL_TEST(HostExtremeFiniteAxisBindingsRemainDirectional) {
    const std::string root = TempDir("astral_host_finite_axis");
    WriteText(root + "/empty.scene.json", R"({"entities": []})");
    WriteText(root + "/project.json", R"({"format":"astral-project","name":"Finite input fixture",
        "startupScene":"empty.scene.json","input":{"contexts":[{"name":"Neutral","actions":[
        {"name":"Move","type":"Axis2D","bindings":[{"key":"W","axis":[3e38,0]},
        {"key":"D","axis":[3e38,0]}]}]}]}})");
    Framework::HostSettings settings;
    settings.contentRoot = root;
    settings.workerThreads = 0;
    settings.audio = false;
    GameHost host(settings);
    std::string error;
    ASTRAL_CHECK(host.LoadProject("project.json", error));
    ASTRAL_CHECK(host.Project().input[0].actions[0].bindings[0].axisContribution.x == 3.0e38f);
    host.Update(kFrame, Hold({"W", "D"}));
    const Math::Vec2 axis = host.Input().Axis("Move");
    if (!std::isfinite(axis.x) || !std::isfinite(axis.y))
        std::fprintf(stderr, "host axis after simultaneous finite bindings: [%.9g, %.9g]\n", axis.x, axis.y);
    ASTRAL_CHECK(std::isfinite(axis.x) && std::isfinite(axis.y));
    ASTRAL_CHECK_NEAR(axis.x, 1.0f, 1e-6);
    ASTRAL_CHECK(axis.y == 0.0f && host.Input().Pressed("Move"));
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(InputJsonRejectsInvalidFloatNumbersTransactionally) {
    std::vector<Input::InputContext> contexts;
    std::string error;
    ASTRAL_CHECK(Framework::ParseInputMap(NumericInputMap(), contexts, error));
    Framework::ProjectDesc previous;
    previous.name = "Previous";
    previous.startupScene = "previous.scene.json";
    previous.input = contexts;
    const double invalid[] = {1e100, -1e100, std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()};
    for (int component = 0; component < 2; ++component) {
        for (double number : invalid) {
            const Core::JsonValue input = component == 0 ? NumericInputMap(number, 0) : NumericInputMap(1, number);
            ASTRAL_CHECK(!Framework::ParseInputMap(input, contexts, error));
            ASTRAL_CHECK(error.find("input.contexts[0].actions[0].bindings[0].axis[" + std::to_string(component) + "]") != std::string::npos);
            ASTRAL_CHECK(contexts.size() == 1 && contexts[0].name == "Numeric");
            ASTRAL_CHECK(contexts[0].actions[0].bindings[0].axisContribution.x == 1);
            Core::JsonValue project = Parse(R"({"format":"astral-project","name":"Rejected","startupScene":"new.scene.json"})");
            project.Set("input", input);
            ASTRAL_CHECK(!Framework::ParseProject(project, previous, error));
            ASTRAL_CHECK(previous.name == "Previous" && previous.startupScene == "previous.scene.json");
            ASTRAL_CHECK(previous.input[0].actions[0].bindings[0].axisContribution.x == 1);
        }
    }
    for (const char* timer : {"bufferSeconds", "doubleTapSeconds", "tapMaxSeconds"}) {
        for (double number : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(), -1.0, 11.0}) {
            Core::JsonValue input = NumericInputMap();
            input.Find("contexts")->Items()[0].Find("actions")->Items()[0].Set(timer, number);
            ASTRAL_CHECK(!Framework::ParseInputMap(input, contexts, error));
            ASTRAL_CHECK(error.find(timer) != std::string::npos && contexts[0].name == "Numeric");
        }
    }
    ASTRAL_CHECK(Framework::ParseInputMap(NumericInputMap(std::numeric_limits<float>::max(), 1e-40), contexts, error));
    ASTRAL_CHECK(contexts[0].actions[0].bindings[0].axisContribution.x == std::numeric_limits<float>::max());
    ASTRAL_CHECK(contexts[0].actions[0].bindings[0].axisContribution.y > 0);
}

ASTRAL_TEST(HostRejectsInvalidNumericInputWithoutReplacingProject) {
    const std::string root = TempDir("astral_host_reject_nonfinite_input");
    WriteText(root + "/neutral.scene.json", R"({"entities":[{"name":"Sentinel"}]})");
    Core::JsonValue json = Parse(R"({"format":"astral-project","name":"Current","startupScene":"neutral.scene.json"})");
    json.Set("input", NumericInputMap());
    Framework::ProjectDesc project;
    std::string error;
    ASTRAL_CHECK(Framework::ParseProject(json, project, error));
    Framework::HostSettings settings;
    settings.contentRoot = root;
    settings.workerThreads = 0;
    settings.audio = false;
    GameHost host(settings);
    ASTRAL_CHECK(host.ApplyProject(project, error));
    host.Update(kFrame, Hold({"W"}));
    const std::string bindings = host.Input().SaveBindings();
    Framework::ProjectDesc rejected = project;
    rejected.name = "Rejected";
    rejected.config = {{"host.showStats", "true"}};
    rejected.input[0].actions[0].bindings[0].axisContribution.x = std::numeric_limits<float>::quiet_NaN();
    ASTRAL_CHECK(!host.ApplyProject(rejected, error) && error.find("input.contexts[0]") != std::string::npos);
    ASTRAL_CHECK(host.Project().name == "Current" && host.Input().SaveBindings() == bindings);
    ASTRAL_CHECK(!host.Commands().Find("host.showStats")->GetBool() && !host.World().Find("Sentinel").IsNull());
    json.Set("input", NumericInputMap(1e100));
    WriteText(root + "/invalid.json", Core::WriteJson(json));
    ASTRAL_CHECK(!host.LoadProject("invalid.json", error) && error.find("axis[0]") != std::string::npos);
    ASTRAL_CHECK(host.Project().name == "Current" && host.Input().SaveBindings() == bindings);
    host.Update(kFrame, Hold({"W"}));
    ASTRAL_CHECK(host.Input().Axis("Move").x == 1 && !host.Input().Pressed("Move"));
    std::filesystem::remove_all(root);
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

ASTRAL_TEST(HotReloadUpdatesReferencedPrefab) {
    const std::string root = TempDir("astral_host_reload_prefab");
    WriteText(root + "/prefab.json", R"({"entity":{"name":"PrefabBefore","transform":{"position":[1,2,3]}}})");
    WriteText(root + "/scene.json", R"({"entities":[{"prefab":"prefab.json"}]})");
    auto host = MakeHost(root, true);
    std::string error;
    ASTRAL_CHECK(host->LoadScene("scene.json", error));
    ASTRAL_CHECK(!host->World().Find("PrefabBefore").IsNull());
    const auto prefab = host->Assets().Load<Core::JsonValue>("prefab.json");
    const auto scene = host->Assets().Load<Core::JsonValue>("scene.json");
    const std::uint32_t sceneVersion = scene.Version();

    WriteText(root + "/prefab.json", R"({"entity":{"name":"PrefabAfter","transform":{"position":[4,5,6]}}})");
    Touch(root + "/prefab.json", 5);
    Run(*host, 15);
    ASTRAL_CHECK(prefab.Version() == 2 && prefab->operator[]("entity").String("name") == "PrefabAfter");
    ASTRAL_CHECK(scene.Version() == sceneVersion); // only the dependency changed
    ASTRAL_CHECK(!host->World().Find("PrefabAfter").IsNull());
    ASTRAL_CHECK(host->World().Find("PrefabBefore").IsNull());
    ASTRAL_CHECK_NEAR(PositionOf(*host, "PrefabAfter").x, 4.0f, 1.0e-6f);
    ASTRAL_CHECK(host->Stats().hotReloads == 1 && host->LastError().empty());
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(HotReloadReportsMalformedSceneJson) {
    const std::string root = TempDir("astral_host_reload_syntax");
    WriteText(root + "/scene.json", R"({"entities":[{"name":"LastGood"}]})");
    auto host = MakeHost(root, true);
    std::string error;
    ASTRAL_CHECK(host->LoadScene("scene.json", error));
    const auto scene = host->Assets().Load<Core::JsonValue>("scene.json");
    const std::uint32_t version = scene.Version();
    const std::size_t lines = host->Log().size();

    WriteText(root + "/scene.json", "{\"entities\":[");
    Touch(root + "/scene.json", 5);
    Run(*host, 15);
    ASTRAL_CHECK(scene.Ready() && scene.Version() == version && !scene.Error().empty());
    ASTRAL_CHECK(!host->World().Find("LastGood").IsNull() && host->Stats().hotReloads == 0);
    ASTRAL_CHECK(!host->LastError().empty());
    ASTRAL_CHECK(host->Log().size() == lines + 1 && host->Log().back().find("error:") == 0);
    const std::string diagnostic = host->LastError();
    Run(*host, 30);
    ASTRAL_CHECK(host->LastError() == diagnostic && host->Log().size() == lines + 1);

    WriteText(root + "/scene.json", R"({"entities":[{"name":"Recovered"}]})");
    Touch(root + "/scene.json", 10);
    Run(*host, 15);
    ASTRAL_CHECK(host->LastError().empty() && host->Stats().hotReloads == 1);
    ASTRAL_CHECK(host->World().Find("LastGood").IsNull() && !host->World().Find("Recovered").IsNull());
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(HotReloadUpdatesNestedPrefabAndIgnoresUnrelatedErrors) {
    const std::string root = TempDir("astral_host_reload_nested");
    WriteText(root + "/inner.json", R"({"entity":{"name":"NestedBefore"}})");
    WriteText(root + "/outer.json", R"({"entity":{"prefab":"inner.json"}})");
    WriteText(root + "/scene.json", R"({"entities":[{"prefab":"outer.json"}]})");
    auto host = MakeHost(root, true);
    std::string error;
    ASTRAL_CHECK(host->LoadScene("scene.json", error));
    ASTRAL_CHECK(!host->World().Find("NestedBefore").IsNull());
    const std::size_t lines = host->Log().size();
    host->Assets().Load<Core::JsonValue>("unrelated.json");
    WriteText(root + "/unrelated.json", "{");
    Run(*host, 15);
    ASTRAL_CHECK(host->Stats().hotReloads == 0 && host->LastError().empty() && host->Log().size() == lines);
    WriteText(root + "/inner.json", R"({"entity":{"name":"NestedAfter"}})");
    Touch(root + "/inner.json", 5);
    Run(*host, 15);
    ASTRAL_CHECK(host->Stats().hotReloads == 1 && !host->World().Find("NestedAfter").IsNull());
    auto* world = &host->World();
    WriteText(root + "/inner.json", R"({"entity":{"components":{"Unknown":{}}}})");
    Touch(root + "/inner.json", 10);
    Run(*host, 15);
    ASTRAL_CHECK(&host->World() == world && !host->LastError().empty() && host->Stats().hotReloads == 1);
    const auto diagnostics = host->Log().size();
    Run(*host, 30);
    ASTRAL_CHECK(host->Log().size() == diagnostics);
    WriteText(root + "/inner.json", R"({"entity":{"name":"NestedRecovered"}})");
    Touch(root + "/inner.json", 15);
    Run(*host, 15);
    ASTRAL_CHECK(host->LastError().empty() && host->Stats().hotReloads == 2);
    ASTRAL_CHECK(!host->World().Find("NestedRecovered").IsNull());
    world = &host->World();
    std::filesystem::remove(root + "/inner.json");
    Run(*host, 15);
    ASTRAL_CHECK(&host->World() == world && !host->LastError().empty());
    WriteText(root + "/inner.json", R"({"entity":{"name":"NestedRecreated"}})");
    Run(*host, 15);
    ASTRAL_CHECK(host->LastError().empty() && host->Stats().hotReloads == 3);
    ASTRAL_CHECK(!host->World().Find("NestedRecreated").IsNull());
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(HotReloadPreservesWorldWhenSceneFileIsDeletedAndRecreated) {
    const std::string root = TempDir("astral_host_reload_deleted_scene");
    WriteText(root + "/scene.json", R"({"entities":[{"name":"BeforeDelete"}]})");
    auto host = MakeHost(root, true);
    std::string error;
    ASTRAL_CHECK(host->LoadScene("scene.json", error));
    auto* world = &host->World();
    const auto scene = host->Assets().Load<Core::JsonValue>("scene.json");
    std::filesystem::remove(root + "/scene.json");
    Run(*host, 15);
    ASTRAL_CHECK(&host->World() == world && scene.Version() == 1 && !host->LastError().empty());
    const auto lines = host->Log().size();
    Run(*host, 30);
    ASTRAL_CHECK(host->Log().size() == lines);
    WriteText(root + "/scene.json", R"({"entities":[{"name":"AfterCreate"}]})");
    Run(*host, 15);
    ASTRAL_CHECK(host->LastError().empty() && scene.Version() == 2 && host->Stats().hotReloads == 1);
    ASTRAL_CHECK(!host->World().Find("AfterCreate").IsNull());
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(HotReloadUpdatesDirectTextureAndWavConsumersTransactionally) {
    const std::string root = TempDir("astral_host_reload_texture_audio");
    std::string error;
    Graphics::ImageRgba8 image;
    image.Resize(2, 2, {255, 0, 0, 255});
    ASTRAL_CHECK(Graphics::WritePng(root + "/texture.png", image, error));
    ASTRAL_CHECK(Audio::WriteWav(root + "/tone.wav", std::vector<float>(400, 0.1f), 48000, error));
    WriteText(root + "/scene.json", R"({"materials":{"M":{"texture":"texture.png"}},"entities":[
        {"name":"Surface","components":{"MeshRenderer":{"mesh":"primitive:cube","material":"M"}}},
        {"name":"Sound","components":{"AudioSource":{"clip":"tone.wav","loop":true,"spatial":false}}}]})");
    auto host = MakeHost(root, true);
    ASTRAL_CHECK(host->LoadScene("scene.json", error));
    Run(*host, 2);
    const auto surface = [&] { return host->World().Get<Framework::MeshRenderer>(host->World().Find("Surface")); };
    const auto sound = [&] { return host->World().Get<Framework::AudioSource>(host->World().Find("Sound")); };
    ASTRAL_CHECK(surface()->material->baseTexture->Fetch(0, 0, 0).x > 0.99f);
    ASTRAL_CHECK(sound()->clip->samples.size() == 200 && host->Audio()->IsPlaying(sound()->voice));
    const auto oldVoice = sound()->voice;
    const auto oldMaterial = surface()->material;
    image.Resize(3, 3, {0, 255, 0, 255});
    ASTRAL_CHECK(Graphics::WritePng(root + "/texture.png", image, error));
    Touch(root + "/texture.png", 5);
    Run(*host, 15);
    ASTRAL_CHECK(host->Stats().hotReloads == 1 && surface()->material->baseTexture->Width() == 3);
    ASTRAL_CHECK(surface()->material->baseTexture->Fetch(0, 0, 0).y > 0.99f);
    ASTRAL_CHECK(oldMaterial->baseTexture->Fetch(0, 0, 0).x > 0.99f); // old resource lifetime survives replacement
    ASTRAL_CHECK(!host->Audio()->IsPlaying(oldVoice) && host->Audio()->IsPlaying(sound()->voice));
    ASTRAL_CHECK(Audio::WriteWav(root + "/tone.wav", std::vector<float>(800, 0.6f), 48000, error));
    Touch(root + "/tone.wav", 5);
    Run(*host, 15);
    ASTRAL_CHECK(host->Stats().hotReloads == 2 && sound()->clip->samples.size() == 400);
    ASTRAL_CHECK_NEAR(sound()->clip->samples[0], 0.6f, 0.001f);
    auto* goodWorld = &host->World();
    WriteText(root + "/tone.wav", "not a WAV");
    Touch(root + "/tone.wav", 10);
    Run(*host, 15);
    ASTRAL_CHECK(&host->World() == goodWorld && !host->LastError().empty() && sound()->clip->samples.size() == 400);
    const auto failures = host->Log().size();
    Run(*host, 30);
    ASTRAL_CHECK(host->Log().size() == failures);
    ASTRAL_CHECK(Audio::WriteWav(root + "/tone.wav", std::vector<float>(1000, 0.3f), 48000, error));
    Touch(root + "/tone.wav", 15);
    Run(*host, 15);
    ASTRAL_CHECK(host->LastError().empty() && sound()->clip->samples.size() == 500);
    goodWorld = &host->World();
    std::filesystem::remove(root + "/texture.png");
    Run(*host, 15);
    ASTRAL_CHECK(&host->World() == goodWorld && !host->LastError().empty());
    ASTRAL_CHECK(Graphics::WritePng(root + "/texture.png", image, error));
    Run(*host, 15);
    ASTRAL_CHECK(host->LastError().empty() && host->Stats().hotReloads == 4);
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(HotReloadUpdatesModelAndExternalBufferImageConsumers) {
    const std::string root = TempDir("astral_host_reload_models");
    std::string error;
    ASTRAL_CHECK(Assets::ExportGltf(ReloadModel(), root + "/model.glb", Assets::GltfContainer::Binary, error));
    WriteText(root + "/scene.json", R"({"entities":[{"model":"model.glb"}]})");
    auto host = MakeHost(root, true);
    ASTRAL_CHECK(host->LoadScene("scene.json", error));
    ASTRAL_CHECK_NEAR(ReloadMeshExtent(*host), 1.0f, 1.0e-6f);
    ASTRAL_CHECK(Assets::ExportGltf(ReloadModel(2), root + "/model.glb", Assets::GltfContainer::Binary, error));
    Touch(root + "/model.glb", 5);
    Run(*host, 15);
    ASTRAL_CHECK_NEAR(ReloadMeshExtent(*host), 2.0f, 1.0e-6f);
    const auto model = host->Assets().Load<Assets::GltfDocument>("model.glb");
    const auto modelVersion = model.Version();
    auto* goodWorld = &host->World();
    WriteText(root + "/model.glb", "{");
    Run(*host, 15);
    ASTRAL_CHECK(&host->World() == goodWorld && model.Version() == modelVersion && !host->LastError().empty());
    ASTRAL_CHECK(Assets::ExportGltf(ReloadModel(3), root + "/model.glb", Assets::GltfContainer::Binary, error));
    Run(*host, 15);
    ASTRAL_CHECK(host->LastError().empty() && model.Version() == modelVersion + 1);
    ASTRAL_CHECK_NEAR(ReloadMeshExtent(*host), 3.0f, 1.0e-6f);

    auto external = ExternalReloadModel();
    WriteReloadBytes(root + "/mesh.bin", external.buffer);
    WriteReloadBytes(root + "/image.png", external.image);
    WriteText(root + "/external.gltf", Core::WriteJson(external.json));
    WriteText(root + "/scene.json", R"({"entities":[{"model":"external.gltf"}]})");
    Touch(root + "/scene.json", 5);
    Run(*host, 15);
    const auto scene = host->Assets().Load<Core::JsonValue>("scene.json");
    const auto sceneVersion = scene.Version();
    const auto gltf = host->Assets().Load<Assets::GltfDocument>("external.gltf");
    const auto enlarged = ExternalReloadModel(4);
    WriteReloadBytes(root + "/mesh.bin", enlarged.buffer);
    Touch(root + "/mesh.bin", 5);
    Run(*host, 15);
    ASTRAL_CHECK_NEAR(ReloadMeshExtent(*host), 4.0f, 1.0e-6f);
    const auto bufferVersion = gltf.Version();
    goodWorld = &host->World();
    std::filesystem::remove(root + "/mesh.bin");
    Run(*host, 15);
    ASTRAL_CHECK(&host->World() == goodWorld && gltf.Version() == bufferVersion && !host->LastError().empty());
    const auto bufferMessages = host->Log().size();
    Run(*host, 30);
    ASTRAL_CHECK(host->Log().size() == bufferMessages);
    WriteReloadBytes(root + "/mesh.bin", enlarged.buffer);
    Run(*host, 15);
    ASTRAL_CHECK(host->LastError().empty() && gltf.Version() == bufferVersion + 1);
    ASTRAL_CHECK_NEAR(ReloadMeshExtent(*host), 4.0f, 1.0e-6f);
    Graphics::ImageRgba8 green;
    green.Resize(3, 3, {0, 255, 0, 255});
    ASTRAL_CHECK(Graphics::WritePng(root + "/image.png", green, error));
    Touch(root + "/image.png", 5);
    Run(*host, 15);
    ASTRAL_CHECK(ReloadMesh(*host)->material->baseTexture->Width() == 3);
    ASTRAL_CHECK(ReloadMesh(*host)->material->baseTexture->Fetch(0, 0, 0).y > 0.99f);
    ASTRAL_CHECK(scene.Version() == sceneVersion);
    const auto goodVersion = gltf.Version();
    goodWorld = &host->World();
    WriteText(root + "/image.png", "corrupt PNG replacement");
    Run(*host, 15);
    ASTRAL_CHECK(&host->World() == goodWorld && gltf.Version() == goodVersion && !host->LastError().empty());
    ASTRAL_CHECK(ReloadMesh(*host)->material->baseTexture->Width() == 3);
    ASTRAL_CHECK(Graphics::WritePng(root + "/image.png", green, error));
    Run(*host, 15);
    ASTRAL_CHECK(host->LastError().empty() && gltf.Version() == goodVersion + 1);

    // Failed candidate introduces a model/image absent from the good manifest.
    // Fixing only that new image must recover, even after asset collection.
    external.json.Find("images")->Items()[0].Set("uri", "future.png");
    WriteText(root + "/new.gltf", Core::WriteJson(external.json));
    WriteText(root + "/scene.json", R"({"entities":[{"name":"RecoveredNewModel","model":"new.gltf"}]})");
    Touch(root + "/scene.json", 10);
    goodWorld = &host->World();
    Run(*host, 15);
    ASTRAL_CHECK(&host->World() == goodWorld && !host->LastError().empty());
    const auto messages = host->Log().size();
    host->Assets().Collect();
    Run(*host, 30);
    ASTRAL_CHECK(host->Log().size() == messages);
    ASTRAL_CHECK(Graphics::WritePng(root + "/future.png", green, error));
    Run(*host, 15);
    ASTRAL_CHECK(host->LastError().empty() && !host->World().Find("RecoveredNewModel").IsNull());
    std::filesystem::remove_all(root);
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

namespace {

Framework::ProjectDesc TransactionProject(const std::string& name, const std::string& scene, std::uint16_t key) {
    Framework::ProjectDesc project;
    project.name = name;
    project.startupScene = scene;
    Input::InputContext context;
    context.name = name;
    Input::ActionDesc move;
    move.name = "Move";
    move.type = Input::ActionType::Axis2D;
    move.bufferSeconds = 0.2f;
    move.bindings.push_back({key, {0, 1}});
    context.actions.push_back(std::move(move));
    project.input.push_back(std::move(context));
    return project;
}

struct ProjectTransactionFixture {
    std::string root;
    int notifications{};
    std::unique_ptr<GameHost> host;
    Framework::ProjectDesc original;
    Framework::GameWorld* world{};
    std::string bindings;
    float held{};

    explicit ProjectTransactionFixture(const char* name) : root(TempDir(name)) {
        WriteText(root + "/good.json", R"({"prefabs":{"OriginalPrefab":{"name":"Spawned"}},
            "entities":[{"name":"Keep","components":{"Camera":{}}}]})");
        WriteText(root + "/next.json", R"({"entities":[{"name":"Replacement"}]})");
        Framework::HostSettings settings;
        settings.contentRoot = root;
        settings.workerThreads = 0;
        settings.audio = false;
        host = std::make_unique<GameHost>(settings);
        host->Commands().RegisterInt("project.observed", 1, "original neutral test setting");
        host->Commands().RegisterInt("project.locked", 3, "read-only fixture", Core::CVarReadOnly);
        host->Commands().RegisterInt("project.cheat", 4, "cheat fixture", Core::CVarCheat);
        original = TransactionProject("Original", "good.json", 'W');
        original.config = {{"r.Quality", "1"}, {"host.showStats", "false"}, {"project.observed", "1"}};
        std::string error;
        ASTRAL_CHECK(host->ApplyProject(original, error));
        host->Commands().Find("r.Quality")->OnChanged([this](const Core::ConsoleVariable&) { ++notifications; });
        host->Commands().Find("project.observed")->OnChanged([this](const Core::ConsoleVariable&) { ++notifications; });
        host->Update(kFrame, Hold({"W"}));
        world = &host->World();
        bindings = host->Input().SaveBindings();
        held = host->Input().HeldSeconds("Move");
        ASTRAL_CHECK(host->Input().Down("Move") && host->Input().Pressed("Move"));
    }

    ~ProjectTransactionFixture() {
        host.reset();
        std::filesystem::remove_all(root);
    }

    Framework::ProjectDesc Candidate(const std::string& scene) const {
        auto project = TransactionProject("Candidate", scene, 'S');
        project.config = {{"r.Quality", "0"}, {"host.showStats", "true"}, {"project.observed", "9"}};
        return project;
    }

    void CheckPreserved() {
        std::printf("project transaction: worldSame=%d project=%s quality=%d observed=%d notifications=%d moveDown=%d\n",
            &host->World() == world, host->Project().name.c_str(), host->Commands().Find("r.Quality")->GetInt(),
            host->Commands().Find("project.observed")->GetInt(), notifications, host->Input().Down("Move"));
        std::fflush(stdout);
        ASTRAL_CHECK(&host->World() == world && !host->World().Find("Keep").IsNull());
        ASTRAL_CHECK(host->ScenePath() == "good.json" && host->Document().prefabs.count("OriginalPrefab") == 1);
        ASTRAL_CHECK(host->Project().name == original.name && host->Project().startupScene == original.startupScene);
        ASTRAL_CHECK(host->Project().config == original.config);
        ASTRAL_CHECK(host->Input().SaveBindings() == bindings);
        ASTRAL_CHECK(host->Input().Down("Move") && host->Input().Pressed("Move"));
        ASTRAL_CHECK(host->Input().HeldSeconds("Move") == held && host->Input().Axis("Move").y == 1.0f);
        ASTRAL_CHECK(host->Commands().Find("r.Quality")->GetInt() == 1);
        ASTRAL_CHECK(!host->Commands().Find("host.showStats")->GetBool());
        ASTRAL_CHECK(host->Commands().Find("project.observed")->GetInt() == 1);
        ASTRAL_CHECK(host->Commands().Find("project.locked")->GetInt() == 3);
        ASTRAL_CHECK(host->Commands().Find("project.cheat")->GetInt() == 4);
        ASTRAL_CHECK(host->Renderer().Settings().maxShadowResolution == Graphics::RendererSettings::Preset(1).maxShadowResolution);
        ASTRAL_CHECK(notifications == 0); // restoring values cannot undo callback side effects
    }

    void CheckRecovery() {
        std::string error;
        ASTRAL_CHECK(host->ApplyProject(Candidate("next.json"), error));
        ASTRAL_CHECK(host->LastError().empty() && host->Project().name == "Candidate");
        ASTRAL_CHECK(host->ScenePath() == "next.json" && !host->World().Find("Replacement").IsNull());
        ASTRAL_CHECK(host->World().Find("Keep").IsNull());
        ASTRAL_CHECK(host->Commands().Find("r.Quality")->GetInt() == 0);
        ASTRAL_CHECK(host->Commands().Find("host.showStats")->GetBool());
        ASTRAL_CHECK(host->Commands().Find("project.observed")->GetInt() == 9 && notifications == 2);
        host->Update(kFrame, Hold({"S"}));
        ASTRAL_CHECK(host->Input().Down("Move") && host->Input().Axis("Move").y == 1.0f);
    }
};

} // namespace

ASTRAL_TEST(FailedProjectScenePreservesRunningProject) {
    for (int failure = 0; failure != 4; ++failure) {
        ProjectTransactionFixture fixture("astral_project_scene_transaction");
        const std::string path = "bad.json";
        if (failure == 1) WriteText(fixture.root + "/" + path, "{malformed");
        if (failure == 2) WriteText(fixture.root + "/" + path, R"({"entities":[{"components":{"Unknown":{}}}]})");
        if (failure == 3) WriteText(fixture.root + "/" + path, R"({"entities":[{"prefab":"missing.prefab.json"}]})");
        std::string error;
        ASTRAL_CHECK(!fixture.host->ApplyProject(fixture.Candidate(path), error));
        ASTRAL_CHECK(!error.empty() && !fixture.host->LastError().empty());
        fixture.CheckPreserved();
        fixture.CheckRecovery();
    }
}

ASTRAL_TEST(FailedProjectConfigPreservesRunningProject) {
    for (const auto& invalid : std::vector<std::pair<std::string, std::string>>{
             {"project.unknown", "1"}, {"project.locked", "8"}, {"project.cheat", "8"}, {"project.observed", "bad-int"}}) {
        ProjectTransactionFixture fixture("astral_project_config_transaction");
        auto candidate = fixture.Candidate("next.json");
        candidate.config.push_back(invalid); // late failure follows valid observable settings
        std::string error;
        ASTRAL_CHECK(!fixture.host->ApplyProject(candidate, error));
        ASTRAL_CHECK(!error.empty() && !fixture.host->LastError().empty());
        fixture.CheckPreserved();
        fixture.CheckRecovery();
    }
}

ASTRAL_TEST(ProjectSettingsAdapterIsAtomicWithoutLoadingItsStartupScene) {
    ProjectTransactionFixture fixture("astral_project_settings_transaction");
    auto invalid = fixture.Candidate("deliberately-unloaded.json");
    invalid.config.push_back({"project.observed", "bad-int"});
    std::string error;
    ASTRAL_CHECK(!fixture.host->ApplyProjectSettings(invalid, error));
    fixture.CheckPreserved();
    ASTRAL_CHECK(fixture.host->ApplyProjectSettings(fixture.Candidate("deliberately-unloaded.json"), error));
    ASTRAL_CHECK(error.empty() && fixture.host->LastError().empty());
    ASTRAL_CHECK(&fixture.host->World() == fixture.world && !fixture.world->Find("Keep").IsNull());
    ASTRAL_CHECK(fixture.host->ScenePath() == "good.json" && fixture.host->Document().prefabs.count("OriginalPrefab") == 1);
    ASTRAL_CHECK(fixture.host->Project().name == "Candidate"
        && fixture.host->Project().startupScene == "deliberately-unloaded.json");
    ASTRAL_CHECK(fixture.host->Commands().Find("project.observed")->GetInt() == 9 && fixture.notifications == 2);
    fixture.host->Update(kFrame, Hold({"S"}));
    ASTRAL_CHECK(fixture.host->Input().Down("Move"));
}

ASTRAL_TEST(ProjectTransactionsPreserveInvalidInputAndAcceptAliasedMetadata) {
    ProjectTransactionFixture fixture("astral_project_input_transaction");
    auto invalid = fixture.Candidate("next.json");
    invalid.input[0].actions[0].bindings[0].axisContribution.x = std::numeric_limits<float>::infinity();
    std::string error;
    ASTRAL_CHECK(!fixture.host->ApplyProject(invalid, error));
    fixture.CheckPreserved();
    ASTRAL_CHECK(!fixture.host->ApplyProjectSettings(invalid, error));
    fixture.CheckPreserved();
    ASTRAL_CHECK(!fixture.host->ApplyProject(fixture.Candidate(""), error));
    fixture.CheckPreserved();
    ASTRAL_CHECK(fixture.host->ApplyProject(fixture.host->Project(), error));
    ASTRAL_CHECK(error.empty() && fixture.host->LastError().empty() && fixture.host->Project().name == "Original");
    ASTRAL_CHECK(!fixture.host->World().Find("Keep").IsNull() && fixture.notifications == 0);
    ASTRAL_CHECK(fixture.host->Input().SaveBindings() == fixture.bindings);
}

namespace {
struct ProjectCreateProbe final : Framework::Behaviour {
    static GameHost* host;
    static int creates;
    void OnCreate() override {
        ++creates;
        ASTRAL_CHECK(host && &World() == &host->World());
        ASTRAL_CHECK(host->Project().name == "Candidate" && host->Project().startupScene == "next.json");
        ASTRAL_CHECK(World().Input()->SaveBindings().find("Candidate Move 0 83") != std::string::npos); // S
        ASTRAL_CHECK(host->Commands().Find("r.Quality")->GetInt() == 0);
        ASTRAL_CHECK(host->Commands().Find("project.observed")->GetInt() == 9);
        ASTRAL_CHECK(host->Renderer().Settings().maxShadowResolution == Graphics::RendererSettings::Preset(0).maxShadowResolution);
    }
};
GameHost* ProjectCreateProbe::host{};
int ProjectCreateProbe::creates{};
} // namespace

ASTRAL_TEST(ProjectSceneOnCreateSeesFinalProjectSettings) {
    ProjectTransactionFixture fixture("astral_project_create_transaction");
    Framework::BehaviourRegistry::Global().Register<ProjectCreateProbe>("ProjectTransactionProbe");
    WriteText(fixture.root + "/next.json", R"({"entities":[{"name":"Replacement",
        "behaviours":[{"type":"ProjectTransactionProbe"}]}]})");
    ProjectCreateProbe::host = fixture.host.get();
    ProjectCreateProbe::creates = 0;
    fixture.CheckRecovery();
    ASTRAL_CHECK(ProjectCreateProbe::creates == 1);
    ProjectCreateProbe::host = nullptr;
}

ASTRAL_TEST_MAIN("EngineSampleTests")
