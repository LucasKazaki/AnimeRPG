#include "Engine/Framework/GameHost.h"

#include "Engine/Graphics/Canvas.h"
#include "Engine/Graphics/RenderScene.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <utility>

namespace Astral::Framework {

using Core::JsonValue;

namespace {

constexpr const char* kProjectFormat = "astral-project";
constexpr int kProjectVersion = 1;

using Clock = std::chrono::steady_clock;

double MillisecondsSince(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

bool Fail(std::string& error, const std::string& path, const std::string& message) {
    error = path.empty() ? message : path + ": " + message;
    return false;
}

std::string Join(const std::string& path, const std::string& key) { return path.empty() ? key : path + "." + key; }
std::string At(const std::string& path, std::size_t index) { return path + "[" + std::to_string(index) + "]"; }

bool OnlyKeys(const JsonValue& json, std::initializer_list<const char*> allowed, const std::string& path,
    std::string& error) {
    for (const std::string& key : json.Keys()) {
        if (std::none_of(allowed.begin(), allowed.end(), [&](const char* name) { return key == name; })) {
            return Fail(error, Join(path, key), "unknown key");
        }
    }
    return true;
}

bool ReadSeconds(const JsonValue& json, const std::string& key, const std::string& path, float& out, std::string& error) {
    const JsonValue* value = json.Find(key);
    if (!value) return true;
    if (!value->IsNumber() || !(value->AsNumber() >= 0.0 && value->AsNumber() <= 10.0)) {
        return Fail(error, Join(path, key), "must be a number of seconds in [0, 10]");
    }
    out = value->AsFloat();
    return true;
}

bool ParseKey(const JsonValue& json, const std::string& path, std::uint16_t& out, std::string& error) {
    if (json.IsString()) {
        out = Input::KeyFromName(json.AsString());
        if (out == 0) return Fail(error, path, "unknown key name '" + json.AsString() + "'");
        return true;
    }
    if (json.IsNumber() && json.AsNumber() >= 1.0 && json.AsNumber() <= 255.0
        && std::floor(json.AsNumber()) == json.AsNumber()) {
        out = static_cast<std::uint16_t>(json.AsInt());
        return true;
    }
    return Fail(error, path, "must be a key name or a virtual-key code in [1, 255]");
}

bool ParseAction(const JsonValue& json, const std::string& path, Input::ActionDesc& out, std::string& error) {
    if (!json.IsObject()) return Fail(error, path, "must be an object");
    if (!OnlyKeys(json, {"name", "type", "bindings", "bufferSeconds", "doubleTapSeconds", "tapMaxSeconds"}, path, error)) {
        return false;
    }
    const JsonValue* name = json.Find("name");
    if (!name || !name->IsString() || name->AsString().empty()) return Fail(error, Join(path, "name"), "must be a non-empty string");
    out.name = name->AsString();
    if (const JsonValue* type = json.Find("type")) {
        if (type->IsString() && type->AsString() == "Button") {
            out.type = Input::ActionType::Button;
        } else if (type->IsString() && type->AsString() == "Axis2D") {
            out.type = Input::ActionType::Axis2D;
        } else {
            return Fail(error, Join(path, "type"), "must be \"Button\" or \"Axis2D\"");
        }
    }
    if (!ReadSeconds(json, "bufferSeconds", path, out.bufferSeconds, error)
        || !ReadSeconds(json, "doubleTapSeconds", path, out.doubleTapSeconds, error)
        || !ReadSeconds(json, "tapMaxSeconds", path, out.tapMaxSeconds, error)) {
        return false;
    }
    const JsonValue* bindings = json.Find("bindings");
    if (!bindings || !bindings->IsArray() || bindings->Size() == 0) {
        return Fail(error, Join(path, "bindings"), "must be a non-empty array");
    }
    for (std::size_t i = 0; i < bindings->Size(); ++i) {
        const JsonValue& source = (*bindings)[i];
        const std::string bindingPath = At(Join(path, "bindings"), i);
        if (!source.IsObject()) return Fail(error, bindingPath, "must be an object");
        if (!OnlyKeys(source, {"key", "axis"}, bindingPath, error)) return false;
        Input::Binding binding;
        const JsonValue* key = source.Find("key");
        if (!key) return Fail(error, Join(bindingPath, "key"), "is required");
        if (!ParseKey(*key, Join(bindingPath, "key"), binding.key, error)) return false;
        const JsonValue* axis = source.Find("axis");
        if (out.type == Input::ActionType::Axis2D) {
            if (!axis || !axis->IsArray() || axis->Size() != 2 || !(*axis)[0].IsNumber() || !(*axis)[1].IsNumber()) {
                return Fail(error, Join(bindingPath, "axis"), "Axis2D bindings need \"axis\": [x, y]");
            }
            binding.axisContribution = {(*axis)[0].AsFloat(), (*axis)[1].AsFloat()};
        } else if (axis) {
            return Fail(error, Join(bindingPath, "axis"), "only Axis2D actions take an axis");
        }
        out.bindings.push_back(binding);
    }
    return true;
}

std::string ConfigValue(const JsonValue& value) {
    if (value.IsBool()) return value.AsBool() ? "1" : "0";
    if (value.IsString()) return value.AsString();
    const double number = value.AsNumber();
    if (std::floor(number) == number && std::fabs(number) < 1.0e15) return std::to_string(static_cast<long long>(number));
    char text[32];
    std::snprintf(text, sizeof(text), "%.9g", number);
    return text;
}

} // namespace

// ------------------------------------------------------------------ project files

bool ParseInputMap(const JsonValue& json, std::vector<Input::InputContext>& out, std::string& error) {
    std::vector<Input::InputContext> contexts;
    if (!json.IsObject()) return Fail(error, "input", "must be an object");
    if (!OnlyKeys(json, {"contexts"}, "input", error)) return false;
    const JsonValue* list = json.Find("contexts");
    if (!list || !list->IsArray()) return Fail(error, "input.contexts", "must be an array");
    for (std::size_t i = 0; i < list->Size(); ++i) {
        const JsonValue& source = (*list)[i];
        const std::string path = At("input.contexts", i);
        if (!source.IsObject()) return Fail(error, path, "must be an object");
        if (!OnlyKeys(source, {"name", "priority", "enabled", "consumesKeys", "actions"}, path, error)) return false;
        Input::InputContext context;
        const JsonValue* name = source.Find("name");
        if (!name || !name->IsString() || name->AsString().empty()) {
            return Fail(error, Join(path, "name"), "must be a non-empty string");
        }
        context.name = name->AsString();
        for (const Input::InputContext& other : contexts) {
            if (other.name == context.name) return Fail(error, Join(path, "name"), "duplicate context '" + context.name + "'");
        }
        if (const JsonValue* priority = source.Find("priority")) {
            if (!priority->IsNumber() || std::floor(priority->AsNumber()) != priority->AsNumber()
                || std::fabs(priority->AsNumber()) > 1000000.0) {
                return Fail(error, Join(path, "priority"), "must be an integer");
            }
            context.priority = static_cast<int>(priority->AsInt());
        }
        if (const JsonValue* enabled = source.Find("enabled")) {
            if (!enabled->IsBool()) return Fail(error, Join(path, "enabled"), "must be true or false");
            context.enabled = enabled->AsBool();
        }
        if (const JsonValue* consumes = source.Find("consumesKeys")) {
            if (!consumes->IsBool()) return Fail(error, Join(path, "consumesKeys"), "must be true or false");
            context.consumesKeys = consumes->AsBool();
        }
        const JsonValue* actions = source.Find("actions");
        if (!actions || !actions->IsArray()) return Fail(error, Join(path, "actions"), "must be an array");
        for (std::size_t a = 0; a < actions->Size(); ++a) {
            Input::ActionDesc action;
            if (!ParseAction((*actions)[a], At(Join(path, "actions"), a), action, error)) return false;
            for (const Input::ActionDesc& other : context.actions) {
                if (other.name == action.name) {
                    return Fail(error, Join(At(Join(path, "actions"), a), "name"), "duplicate action '" + action.name + "'");
                }
            }
            context.actions.push_back(std::move(action));
        }
        contexts.push_back(std::move(context));
    }
    out = std::move(contexts);
    return true;
}

bool ParseProject(const JsonValue& json, ProjectDesc& out, std::string& error) {
    ProjectDesc project;
    if (!json.IsObject()) return Fail(error, "", "a project must be a JSON object");
    if (!OnlyKeys(json, {"format", "version", "name", "startupScene", "input", "config"}, "", error)) return false;
    const JsonValue* format = json.Find("format");
    if (!format || !format->IsString() || format->AsString() != kProjectFormat) {
        return Fail(error, "format", std::string("must be \"") + kProjectFormat + "\"");
    }
    if (const JsonValue* version = json.Find("version");
        version && (!version->IsNumber() || version->AsNumber() < 1 || version->AsNumber() > kProjectVersion)) {
        return Fail(error, "version", "unsupported version (this build reads version " + std::to_string(kProjectVersion) + ")");
    }
    if (const JsonValue* name = json.Find("name")) {
        if (!name->IsString()) return Fail(error, "name", "must be a string");
        project.name = name->AsString();
    }
    const JsonValue* startup = json.Find("startupScene");
    if (!startup || !startup->IsString() || startup->AsString().empty()) {
        return Fail(error, "startupScene", "must name a scene file");
    }
    project.startupScene = startup->AsString();
    if (const JsonValue* input = json.Find("input")) {
        if (!ParseInputMap(*input, project.input, error)) return false;
    }
    if (const JsonValue* config = json.Find("config")) {
        if (!config->IsObject()) return Fail(error, "config", "must be an object");
        for (const std::string& key : config->Keys()) {
            const JsonValue& value = (*config)[key];
            if (!value.IsBool() && !value.IsNumber() && !value.IsString()) {
                return Fail(error, Join("config", key), "must be a bool, number or string");
            }
            project.config.emplace_back(key, ConfigValue(value));
        }
    }
    out = std::move(project);
    return true;
}

// ------------------------------------------------------------------ host

GameHost::GameHost(HostSettings settings)
    : settings_(std::move(settings)),
      jobs_(settings_.workerThreads != 0 ? std::make_unique<Core::JobSystem>(settings_.workerThreads) : nullptr),
      assets_(std::make_unique<Assets::AssetManager>(settings_.contentRoot, jobs_.get())),
      mixer_(settings_.audio ? std::make_unique<Audio::AudioMixer>() : nullptr),
      renderer_(jobs_.get()) {
    assets_->RegisterDefaultLoaders();
    UI::CanvasPanel& hud = ui_.Root().Add<UI::CanvasPanel>("HUD");
    hud.anchorMin = {0.0f, 0.0f}; // fills the viewport
    hud.anchorMax = {1.0f, 1.0f};
    hud_ = &hud;
    console_ = std::make_unique<UI::ConsoleOverlay>(ui_, commands_); // above the HUD
    RegisterCommands();
    ResetWorld();
}

GameHost::~GameHost() {
    // Behaviours may touch HUD widgets and host services while they are destroyed.
    loadSubscription_.Reset();
    quitSubscription_.Reset();
    world_.reset();
}

std::unique_ptr<GameWorld> GameHost::MakeWorld() {
    auto world = std::make_unique<GameWorld>(settings_.world, mixer_.get(), assets_.get());
    world->SetInput(&input_);
    world->SetHud(hud_);
    return world;
}

void GameHost::ResetWorld() {
    loadSubscription_.Reset();
    quitSubscription_.Reset();
    world_.reset(); // OnDestroy may remove the scene's HUD widgets
    hud_->ClearChildren();
    world_ = MakeWorld();
    loadSubscription_ = world_->Events().SubscribeScoped<LoadSceneRequest>(
        [this](const LoadSceneRequest& request) { pendingLoads_.push_back(request.path); });
    quitSubscription_ = world_->Events().SubscribeScoped<QuitRequest>([this](const QuitRequest&) { quitRequested_ = true; });
    document_ = {};
    scenePath_.clear();
    sceneHandle_ = {};
    sceneVersion_ = 0;
}

void GameHost::Activate(SceneDocument document, const std::string& path) {
    ResetWorld();
    document_ = std::move(document);
    scenePath_ = path;
    SceneSerializer(assets_.get()).Instantiate(document_, *world_);
    ++stats_.sceneLoads;
    lastError_.clear();
}

bool GameHost::Report(const std::string& error) {
    lastError_ = error;
    console_->Print("error: " + error);
    return false;
}

bool GameHost::LoadScene(const std::string& requested, std::string& error) {
    const std::string path = requested; // may alias scenePath_, which activation resets
    if (path.empty()) return Report(error = "no scene path");
    const auto handle = assets_->Load<JsonValue>(path);
    if (!handle.Ready()) return Report(error = path + ": " + handle.Error());
    SceneDocument document;
    std::string parseError;
    if (!SceneSerializer(assets_.get()).Parse(*handle.Get(), document, parseError)) {
        return Report(error = path + ": " + parseError);
    }
    Activate(std::move(document), path);
    sceneHandle_ = handle;
    sceneVersion_ = handle.Version();
    console_->Print("loaded " + path);
    return true;
}

bool GameHost::LoadSceneJson(const JsonValue& json, std::string& error) {
    SceneDocument document;
    std::string parseError;
    if (!SceneSerializer(assets_.get()).Parse(json, document, parseError)) return Report(error = parseError);
    Activate(std::move(document), {});
    return true;
}

bool GameHost::ApplyProject(const ProjectDesc& project, std::string& error) {
    for (const auto& [name, value] : project.config) {
        std::string setError;
        if (!commands_.Set(name, value, setError)) return Report(error = "config." + name + ": " + setError);
    }
    input_ = Input::InputSystem();
    for (const Input::InputContext& context : project.input) input_.AddContext(context);
    project_ = project;
    return LoadScene(project.startupScene, error);
}

bool GameHost::LoadProject(const std::string& path, std::string& error) {
    const auto handle = assets_->Load<JsonValue>(path);
    if (!handle.Ready()) return Report(error = path + ": " + handle.Error());
    ProjectDesc project;
    std::string parseError;
    if (!ParseProject(*handle.Get(), project, parseError)) return Report(error = path + ": " + parseError);
    return ApplyProject(project, error);
}

Entity GameHost::Spawn(const std::string& prefab, const Math::TRS& transform, Entity parent) {
    return SceneSerializer(assets_.get()).InstantiatePrefab(document_, prefab, *world_, transform, parent);
}

void GameHost::Update(float dt, const HostInput& input) {
    const Clock::time_point start = Clock::now();
    ++stats_.frames;
    dt = std::max(0.0f, dt);

    // UI first: the console and a focused text box take the keyboard.
    UI::UIInput ui = input.ui;
    console_->PreprocessInput(ui);
    ui_.Update(ui, viewport_);
    Input::InputSnapshot keys = input.keys;
    if (ui_.WantsKeyboard()) keys = {};
    keys.dt = dt;
    input_.Update(keys);

    if (!paused_) world_->Tick(dt);
    if (assets_) assets_->Update();
    ProcessRequests();

    if (settings_.hotReload && !scenePath_.empty()) {
        sinceReloadCheck_ += dt;
        if (sinceReloadCheck_ >= settings_.hotReloadInterval) {
            sinceReloadCheck_ = 0.0f;
            assets_->PollChanges();
            if (sceneHandle_.Valid() && sceneHandle_.Version() != sceneVersion_) {
                sceneVersion_ = sceneHandle_.Version(); // a broken edit is reported once, not every check
                std::string error;
                if (LoadScene(scenePath_, error)) {
                    ++stats_.hotReloads;
                    console_->Print("hot reloaded " + scenePath_);
                }
            }
        }
    }
    stats_.updateMs = MillisecondsSince(start);
}

void GameHost::ProcessRequests() {
    // Loads requested during the frame run between frames, never mid-tick.
    std::vector<std::string> loads;
    loads.swap(pendingLoads_);
    if (loads.empty()) return;
    const std::string path = loads.back().empty() ? scenePath_ : loads.back(); // the last request wins
    std::string error;
    if (path.empty()) {
        Report("reload requested but no scene file is loaded");
        return;
    }
    LoadScene(path, error);
}

const Graphics::ImageRgba8& GameHost::Render(int width, int height) {
    const Clock::time_point start = Clock::now();
    width = std::max(1, width);
    height = std::max(1, height);
    viewport_ = {static_cast<float>(width), static_cast<float>(height)};
    Graphics::RenderScene scene;
    Graphics::RenderView view;
    target_.Resize(width, height);
    if (world_->BuildRenderScene(scene, view, width, height)) {
        renderer_.Render(scene, view, target_);
        stats_.render = renderer_.Stats();
    } else {
        target_.output.Resize(width, height, {18, 16, 30, 255});
        stats_.render = {};
        Graphics::Canvas canvas(target_.output);
        canvas.Text(16, 16, "No active camera", {});
    }
    Graphics::Canvas canvas(target_.output);
    ui_.Draw(canvas);
    if (showStats_) DrawStats(target_.output);
    stats_.renderMs = MillisecondsSince(start);
    return target_.output;
}

void GameHost::DrawStats(Graphics::ImageRgba8& image) const {
    const GameWorldStats& world = world_->Stats();
    char text[256];
    std::snprintf(text, sizeof(text),
        "frame %llu  update %.2f ms  render %.2f ms\nentities %zu  bodies %zu  behaviours %zu\ndraws %zu  tris %zu%s",
        static_cast<unsigned long long>(world.frame), stats_.updateMs, stats_.renderMs, world.entities, world.bodies,
        world.behaviours, stats_.render.drawsVisible, stats_.render.trianglesRasterized, paused_ ? "  PAUSED" : "");
    Graphics::Canvas canvas(image);
    Graphics::TextStyle style;
    style.color = {255, 230, 140, 255};
    style.scale = image.width >= 960 ? 2 : 1;
    // Bottom-left, clear of HUDs that anchor to the top.
    canvas.Text(8, image.height - 8 - 3 * Graphics::Canvas::LineHeight(style.scale), text, style);
}

void GameHost::RegisterCommands() {
    using Arguments = std::vector<std::string>;
    commands_.RegisterCommand("open", "open <scene>: load a scene after this frame", [this](const Arguments& arguments) {
        if (arguments.size() != 1) return std::string("usage: open <scene>");
        pendingLoads_.push_back(arguments[0]);
        return "opening " + arguments[0];
    });
    commands_.RegisterCommand("restart", "reload the current scene", [this](const Arguments&) {
        if (scenePath_.empty()) return std::string("no scene file is loaded");
        pendingLoads_.push_back(scenePath_);
        return "restarting " + scenePath_;
    });
    commands_.RegisterCommand("pause", "pause or resume the simulation", [this](const Arguments&) {
        paused_ = !paused_;
        return std::string(paused_ ? "paused" : "resumed");
    });
    commands_.RegisterCommand("slomo", "slomo <scale>: game time scale (1 = normal)", [this](const Arguments& arguments) {
        if (arguments.size() != 1) return std::string("usage: slomo <scale>");
        char* end = nullptr;
        const float scale = std::strtof(arguments[0].c_str(), &end);
        if (end == arguments[0].c_str() || *end != '\0' || !(scale >= 0.0f && scale <= 100.0f)) {
            return std::string("scale must be a number in [0, 100]");
        }
        world_->Settings().timeScale = scale;
        return "time scale " + arguments[0];
    });
    commands_.RegisterCommand("stat", "toggle the frame statistics overlay", [this](const Arguments&) {
        showStats_ = !showStats_;
        return std::string(showStats_ ? "stats on" : "stats off");
    });
    commands_.RegisterCommand("spawn", "spawn <prefab> [x y z]: instantiate a prefab of the scene",
        [this](const Arguments& arguments) {
            if (arguments.size() != 1 && arguments.size() != 4) return std::string("usage: spawn <prefab> [x y z]");
            Math::TRS transform;
            if (arguments.size() == 4) {
                float xyz[3];
                for (int i = 0; i < 3; ++i) {
                    char* end = nullptr;
                    const std::string& text = arguments[static_cast<std::size_t>(i) + 1];
                    xyz[i] = std::strtof(text.c_str(), &end);
                    if (end == text.c_str() || *end != '\0' || !std::isfinite(xyz[i])) return "bad coordinate '" + text + "'";
                }
                transform.translation = {xyz[0], xyz[1], xyz[2]};
            }
            const Entity entity = Spawn(arguments[0], transform);
            if (entity.IsNull()) return "unknown prefab '" + arguments[0] + "'";
            return "spawned " + arguments[0];
        });
    commands_.RegisterCommand("entities", "list the root entities", [this](const Arguments&) {
        std::string out = std::to_string(world_->Stats().entities) + " entities";
        const World::Registry& registry = world_->Registry();
        int listed = 0;
        for (Entity root : world_->Roots()) {
            if (++listed > 24) {
                out += "\n...";
                break;
            }
            const World::Name* name = registry.Get<World::Name>(root);
            out += "\n  " + (name && !name->value.empty() ? name->value : std::string("(unnamed)"));
        }
        return out;
    });
    commands_.RegisterCommand("quit", "exit the player", [this](const Arguments&) {
        quitRequested_ = true;
        return std::string("quitting");
    });
    Core::ConsoleVariable* quality = commands_.RegisterInt("r.Quality", 2, "renderer preset: 0 low, 1 medium, 2 high, 3 epic");
    quality->SetRange(0, 3);
    renderer_.Settings() = Graphics::RendererSettings::Preset(2);
    quality->OnChanged([this](const Core::ConsoleVariable& variable) {
        renderer_.Settings() = Graphics::RendererSettings::Preset(variable.GetInt());
    });
    Core::ConsoleVariable* stats = commands_.RegisterBool("host.showStats", false, "draw the frame statistics overlay");
    stats->OnChanged([this](const Core::ConsoleVariable& variable) { showStats_ = variable.GetBool(); });
}

} // namespace Astral::Framework
