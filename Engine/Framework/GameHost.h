#pragma once

// Game host: the runtime shell around a GameWorld (the role of Unity's Player
// loop and Unreal's GameInstance + GameViewportClient). It owns the services a
// game needs (assets, audio mixer, action input, the world, the UI with the
// developer console, the renderer) and turns one frame of platform input into
// a simulated, rendered frame:
//
//   console (toggle key, typing) -> UI (focus, pointer) -> input actions (none
//   while the UI has the keyboard) -> GameWorld::Tick -> scene change and
//   hot-reload requests -> Render: the world from its active camera, then the
//   HUD and console on top.
//
// Platform layers (the Win32 player window, the headless AstralPlayer, tests)
// only translate their events into HostInput and present the returned image.
//
// A project file names the startup scene, the input map and console settings
// (Unity's Project Settings and Input Actions asset; UE's GameDefaultMap,
// DefaultInput.ini and DefaultEngine.ini):
//
//   {
//     "format": "astral-project", "version": 1,
//     "name": "Playground",
//     "startupScene": "scenes/playground.scene.json",
//     "config": {"host.showStats": true},
//     "input": {"contexts": [
//       {"name": "Gameplay", "priority": 0, "actions": [
//         {"name": "Move", "type": "Axis2D", "bindings": [
//           {"key": "W", "axis": [0, 1]}, {"key": "S", "axis": [0, -1]},
//           {"key": "A", "axis": [-1, 0]}, {"key": "D", "axis": [1, 0]}]},
//         {"name": "Jump", "bindings": [{"key": "Space"}], "bufferSeconds": 0.15}]}]}
//   }
//
// Behaviours reach the host's services through their world: World().Input()
// for actions, World().Hud() for HUD widgets (cleared with the scene), and
// World().Events().Queue(LoadSceneRequest{...}) to change scenes after the frame.

#include "Engine/Assets/AssetManager.h"
#include "Engine/Audio/AudioMixer.h"
#include "Engine/Core/Console.h"
#include "Engine/Core/JobSystem.h"
#include "Engine/Core/Json.h"
#include "Engine/Framework/GameWorld.h"
#include "Engine/Framework/Scene.h"
#include "Engine/Graphics/RenderTarget.h"
#include "Engine/Graphics/SceneRenderer.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/UI/ConsoleOverlay.h"
#include "Engine/UI/Widgets.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Astral::Framework {

// Queue on GameWorld::Events() to switch scenes once the frame ends (UE's
// OpenLevel, Unity's SceneManager.LoadScene). An empty path reloads the
// current scene. A failed load keeps the current scene and logs the error.
struct LoadSceneRequest {
    std::string path;
};

// Queue on GameWorld::Events() to ask the platform layer to exit.
struct QuitRequest {};

struct HostSettings {
    std::string contentRoot{"."};
    int workerThreads{-1}; // job system for rendering and async loads (0 = run inline)
    bool audio{true};      // create a mixer (the platform layer streams it)
    GameWorldSettings world;
    // Re-read the scene (and assets it references) when its file changes.
    bool hotReload{false};
    float hotReloadInterval{0.5f}; // seconds between file checks
};

// One frame of platform input.
struct HostInput {
    Input::InputSnapshot keys; // held keys and mouse delta (dt is set by Update)
    UI::UIInput ui;            // pointer, key presses this frame, typed text
};

struct HostStats {
    std::uint64_t frames{};
    std::size_t sceneLoads{};
    std::size_t hotReloads{};
    double updateMs{};
    double renderMs{};
    Graphics::RenderStats render;
};

// Project settings (see the header comment).
struct ProjectDesc {
    std::string name;
    std::string startupScene;
    std::vector<Input::InputContext> input;
    std::vector<std::pair<std::string, std::string>> config; // console name, value
};

// Parses a project document; errors name the JSON path.
bool ParseProject(const Core::JsonValue& json, ProjectDesc& out, std::string& error);
// Parses {"contexts": [...]} (the project "input" object).
bool ParseInputMap(const Core::JsonValue& json, std::vector<Input::InputContext>& out, std::string& error);

class GameHost {
public:
    explicit GameHost(HostSettings settings = {});
    ~GameHost();
    GameHost(const GameHost&) = delete;
    GameHost& operator=(const GameHost&) = delete;

    // Applies a project (input map, config) and loads its startup scene.
    bool LoadProject(const std::string& path, std::string& error);
    bool ApplyProject(const ProjectDesc& project, std::string& error);
    const ProjectDesc& Project() const { return project_; }

    // Replaces the world with a scene (content path). Transactional: on
    // failure the current world keeps running and `error` says why.
    bool LoadScene(const std::string& path, std::string& error);
    bool LoadSceneJson(const Core::JsonValue& json, std::string& error); // an unnamed scene
    // Starts an empty world (the HUD is cleared).
    void ResetWorld();
    const std::string& ScenePath() const { return scenePath_; }
    // The loaded scene document (prefabs for spawning at runtime).
    const SceneDocument& Document() const { return document_; }
    // Spawns a prefab of the loaded scene (UE SpawnActor / Unity Instantiate).
    Entity Spawn(const std::string& prefab, const Math::TRS& transform, Entity parent = {});

    // Runs one frame. A paused host still updates the UI and console.
    void Update(float dt, const HostInput& input);
    // Renders the world from its active camera, then the UI. The image stays
    // valid until the next Render.
    const Graphics::ImageRgba8& Render(int width, int height);

    bool Paused() const { return paused_; }
    void SetPaused(bool paused) { paused_ = paused; }
    bool QuitRequested() const { return quitRequested_; }
    // Last scene-loading error ("" when the last load succeeded).
    const std::string& LastError() const { return lastError_; }

    GameWorld& World() { return *world_; }
    const GameWorld& World() const { return *world_; }
    Input::InputSystem& Input() { return input_; }
    UI::UIRoot& UI() { return ui_; }
    UI::CanvasPanel& Hud() { return *hud_; }
    UI::ConsoleOverlay& Console() { return *console_; }
    Core::ConsoleRegistry& Commands() { return commands_; }
    Audio::AudioMixer* Audio() { return mixer_.get(); }
    Assets::AssetManager& Assets() { return *assets_; }
    Graphics::SceneRenderer& Renderer() { return renderer_; }
    const HostStats& Stats() const { return stats_; }
    // The message log (console output, load errors), newest last.
    const std::vector<std::string>& Log() const { return console_->Lines(); }

private:
    void RegisterCommands();
    std::unique_ptr<GameWorld> MakeWorld();
    void Activate(SceneDocument document, const std::string& path);
    bool Report(const std::string& error); // logs a load error, returns false
    void ProcessRequests();
    void DrawStats(Graphics::ImageRgba8& image) const;

    HostSettings settings_;
    std::unique_ptr<Core::JobSystem> jobs_;
    std::unique_ptr<Assets::AssetManager> assets_;
    std::unique_ptr<Audio::AudioMixer> mixer_;
    Core::ConsoleRegistry commands_;
    Input::InputSystem input_;
    UI::UIRoot ui_;
    UI::CanvasPanel* hud_{};
    std::unique_ptr<UI::ConsoleOverlay> console_;
    Graphics::SceneRenderer renderer_;
    Graphics::RenderTarget target_;
    std::unique_ptr<GameWorld> world_;
    SceneDocument document_;
    std::string scenePath_;
    ProjectDesc project_;
    std::vector<std::string> pendingLoads_;
    Assets::AssetHandle<Core::JsonValue> sceneHandle_; // hot reload watches its version
    std::uint32_t sceneVersion_{};
    Math::Vec2 viewport_{1280.0f, 720.0f};
    float sinceReloadCheck_{};
    bool paused_{};
    bool quitRequested_{};
    bool showStats_{};
    std::string lastError_;
    HostStats stats_;
    Core::ScopedSubscription loadSubscription_;
    Core::ScopedSubscription quitSubscription_;
};

} // namespace Astral::Framework
