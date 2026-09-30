// Headless engine player: loads a project or scene through Framework::GameHost
// (the same host the Win32 player runs), plays it for a number of frames with
// scripted input, and writes PNG captures. The run doubles as a reproducible
// smoke of the whole runtime (scene loading, behaviours, physics, UI, audio mix).
//
//   AstralPlayer <content-root> <project-or-scene.json> [options]
//     --frames N        frames to run (default 300)
//     --dt SECONDS      fixed frame time (default 1/60)
//     --size WxH        capture size (default 640x360)
//     --capture DIR     write PNG captures into DIR
//     --every N         also capture every N frames (default: last frame only)
//     --script FILE     demo script (below); runs until it ends when --frames is not given
//     --replay FILE     input recording (ASTRAL_INPUT, e.g. from AstralPlayerWin32's F9);
//                       frames replay with their recorded keys, mouse motion and frame times
//     --exec COMMAND    console command before the first frame (repeatable)
//     --track ENTITY    print this entity's position with each capture (repeatable)
//
// Demo scripts hold keys for a number of frames, one step per line:
//     # comment
//     <frames> <key>... [| console command]    keys by name ("-" = none);
//                                               the command runs on the step's first frame
// Exit code: 0 on success, 1 on usage errors, 2 when loading fails, 3 when a
// capture cannot be written.

#include "Engine/Framework/GameHost.h"
#include "Engine/Graphics/Image.h"
#include "Game/Samples/Playground/PlaygroundBehaviours.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

using namespace Astral;

struct ScriptStep {
    int frames{};
    std::vector<std::uint16_t> keys;
    std::string command;
};

bool ParseScript(const std::string& path, std::vector<ScriptStep>& steps, std::string& error) {
    std::ifstream file(path);
    if (!file) {
        error = "cannot open script '" + path + "'";
        return false;
    }
    std::string line;
    int number = 0;
    while (std::getline(file, line)) {
        ++number;
        if (const std::size_t hash = line.find('#'); hash != std::string::npos) line.erase(hash);
        ScriptStep step;
        if (const std::size_t bar = line.find('|'); bar != std::string::npos) {
            step.command = line.substr(bar + 1);
            line.erase(bar);
            const std::size_t first = step.command.find_first_not_of(" \t");
            step.command = first == std::string::npos ? std::string() : step.command.substr(first);
            while (!step.command.empty() && (step.command.back() == ' ' || step.command.back() == '\t' || step.command.back() == '\r')) {
                step.command.pop_back();
            }
        }
        std::istringstream words(line);
        std::string word;
        if (!(words >> word)) {
            if (!step.command.empty()) {
                error = path + ":" + std::to_string(number) + ": a command needs a frame count";
                return false;
            }
            continue;
        }
        char* end = nullptr;
        const long frames = std::strtol(word.c_str(), &end, 10);
        if (end == word.c_str() || *end != '\0' || frames < 1 || frames > 1000000) {
            error = path + ":" + std::to_string(number) + ": expected a frame count, got '" + word + "'";
            return false;
        }
        step.frames = static_cast<int>(frames);
        while (words >> word) {
            if (word == "-") continue;
            const std::uint16_t key = Input::KeyFromName(word);
            if (key == 0) {
                error = path + ":" + std::to_string(number) + ": unknown key '" + word + "'";
                return false;
            }
            step.keys.push_back(key);
        }
        steps.push_back(std::move(step));
    }
    return true;
}

bool ParseSize(const std::string& text, int& width, int& height) {
    const std::size_t x = text.find('x');
    if (x == std::string::npos) return false;
    width = std::atoi(text.substr(0, x).c_str());
    height = std::atoi(text.substr(x + 1).c_str());
    return width >= 16 && height >= 16 && width <= 8192 && height <= 8192;
}

int Usage() {
    std::fprintf(stderr,
        "usage: AstralPlayer <content-root> <project-or-scene.json> [--frames N] [--dt S] [--size WxH]\n"
        "                    [--capture DIR] [--every N] [--script FILE] [--exec CMD]... [--track ENTITY]...\n");
    return 1;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) return Usage();
    const std::string contentRoot = argv[1];
    const std::string startPath = argv[2];
    int frames = -1;
    float dt = 1.0f / 60.0f;
    int width = 640, height = 360;
    int every = 0;
    std::string captureDir, scriptPath, replayPath;
    std::vector<std::string> commands, tracked;
    for (int i = 3; i < argc; ++i) {
        const std::string option = argv[i];
        auto value = [&]() -> const char* { return i + 1 < argc ? argv[++i] : nullptr; };
        const char* argument = value();
        if (!argument) return Usage();
        if (option == "--frames") {
            frames = std::atoi(argument);
            if (frames < 1) return Usage();
        } else if (option == "--dt") {
            dt = std::strtof(argument, nullptr);
            if (!(dt > 0.0f && dt <= 0.25f)) return Usage();
        } else if (option == "--size") {
            if (!ParseSize(argument, width, height)) return Usage();
        } else if (option == "--capture") {
            captureDir = argument;
        } else if (option == "--every") {
            every = std::atoi(argument);
            if (every < 1) return Usage();
        } else if (option == "--script") {
            scriptPath = argument;
        } else if (option == "--replay") {
            replayPath = argument;
        } else if (option == "--exec") {
            commands.emplace_back(argument);
        } else if (option == "--track") {
            tracked.emplace_back(argument);
        } else {
            return Usage();
        }
    }

    std::vector<ScriptStep> script;
    std::string error;
    if (!scriptPath.empty() && !ParseScript(scriptPath, script, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    Input::InputRecording replay;
    if (!replayPath.empty()) {
        std::ifstream file(replayPath, std::ios::binary);
        std::stringstream text;
        text << file.rdbuf();
        if (!file || !replay.Deserialize(text.str(), error)) {
            std::fprintf(stderr, "cannot replay '%s': %s\n", replayPath.c_str(), file ? error.c_str() : "cannot open");
            return 1;
        }
    }
    if (frames < 0) {
        frames = static_cast<int>(replay.FrameCount());
        for (const ScriptStep& step : script) frames += step.frames;
        if (frames == 0) frames = 300;
    }

    Samples::RegisterPlaygroundBehaviours();
    Framework::HostSettings settings;
    settings.contentRoot = contentRoot;
    Framework::GameHost host(settings);
    // A project file names its format; anything else is loaded as a scene.
    const auto probe = host.Assets().Load<Core::JsonValue>(startPath);
    const bool project = probe.Ready() && probe->String("format") == "astral-project";
    if (!(project ? host.LoadProject(startPath, error) : host.LoadScene(startPath, error))) {
        std::fprintf(stderr, "load failed: %s\n", error.c_str());
        return 2;
    }
    std::size_t entities = 0;
    host.World().Registry().ForEachEntity([&](Framework::Entity) { ++entities; });
    std::printf("loaded %s %s (%zu entities)\n", project ? "project" : "scene", startPath.c_str(), entities);
    for (const std::string& command : commands) std::printf("> %s\n%s\n", command.c_str(), host.Commands().Execute(command).c_str());

    int captures = 0;
    bool failed = false;
    double updateMs = 0.0, renderMs = 0.0;
    auto capture = [&](int frame) {
        const Graphics::ImageRgba8& image = host.Render(width, height);
        renderMs += host.Stats().renderMs;
        std::printf("frame %5d hash %016llx render %6.1f ms draws %4zu tris %7zu entities %4zu bodies %3zu\n", frame,
            static_cast<unsigned long long>(Graphics::HashImage(image)), host.Stats().renderMs, host.Stats().render.drawsVisible,
            host.Stats().render.trianglesRasterized, host.World().Stats().entities, host.World().Stats().bodies);
        for (const std::string& name : tracked) {
            const Framework::Entity entity = host.World().Find(name);
            if (entity.IsNull()) {
                std::printf("  %-14s (gone)\n", name.c_str());
            } else {
                const Math::Vec3 p = host.World().GetWorld(entity).translation;
                std::printf("  %-14s %8.3f %8.3f %8.3f\n", name.c_str(), static_cast<double>(p.x), static_cast<double>(p.y),
                    static_cast<double>(p.z));
            }
        }
        ++captures;
        if (captureDir.empty()) return;
        char name[64];
        std::snprintf(name, sizeof(name), "/frame_%05d.png", frame);
        std::string writeError;
        if (!Graphics::WritePng(captureDir + name, image, writeError)) {
            std::fprintf(stderr, "cannot write %s%s: %s\n", captureDir.c_str(), name, writeError.c_str());
            failed = true;
        }
    };

    std::size_t stepIndex = 0;
    int stepFrame = 0;
    std::vector<float> mix(2 * 800); // drain the mixer like an audio device would
    for (int frame = 1; frame <= frames; ++frame) {
        Framework::HostInput input;
        float frameDt = dt;
        const auto replayIndex = static_cast<std::size_t>(frame - 1);
        if (replayIndex < replay.FrameCount()) {
            const Input::InputSnapshot& recorded = replay.Frame(replayIndex);
            input.keys = recorded;
            if (recorded.dt > 0.0f) frameDt = std::min(recorded.dt, 0.25f);
            for (std::uint16_t key = 0; key < 256; ++key) {
                const bool before = replayIndex > 0 && replay.Frame(replayIndex - 1).Down(key);
                if (recorded.Down(key) && !before) input.ui.keysPressed.push_back(key);
            }
        } else if (stepIndex < script.size()) {
            const ScriptStep& step = script[stepIndex];
            for (std::uint16_t key : step.keys) input.keys.keys.set(key);
            if (stepFrame == 0 && !step.command.empty()) {
                std::printf("> %s\n%s\n", step.command.c_str(), host.Commands().Execute(step.command).c_str());
            }
            // Edge-triggered keys for the UI (the console toggle, text boxes).
            if (stepFrame == 0) input.ui.keysPressed.assign(step.keys.begin(), step.keys.end());
            if (++stepFrame >= step.frames) {
                ++stepIndex;
                stepFrame = 0;
            }
        }
        host.Update(frameDt, input);
        updateMs += host.Stats().updateMs;
        if (Audio::AudioMixer* mixer = host.Audio()) mixer->Mix(mix.data(), static_cast<int>(mix.size() / 2));
        const bool quit = host.QuitRequested();
        if ((every > 0 && frame % every == 0) || frame == frames || quit) capture(frame);
        if (quit) {
            std::printf("quit requested at frame %d\n", frame);
            break;
        }
    }
    std::printf("summary: frames %llu scene loads %zu hot reloads %zu entities %zu | update %.2f ms/frame, render %.1f ms/capture\n",
        static_cast<unsigned long long>(host.Stats().frames), host.Stats().sceneLoads, host.Stats().hotReloads,
        host.World().Stats().entities, updateMs / static_cast<double>(frames > 0 ? frames : 1),
        renderMs / static_cast<double>(captures > 0 ? captures : 1));
    if (!host.LastError().empty()) std::printf("last error: %s\n", host.LastError().c_str());
    return failed ? 3 : 0;
}
