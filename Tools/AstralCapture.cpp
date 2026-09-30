// Headless Astral-mode capture: plays a scripted National Mall route through the
// real gameplay domains (ShowcaseSession mirrors the Win32 frame order) and
// writes PNG frames, the mixed audio as WAV and a Chrome trace of the run.
//
//   AstralCapture <output-dir> [width height] [--quality N]
//
// Every shot is also summarised on stdout (gameplay state, image hash, timing)
// so the run doubles as a reproducible presentation smoke.

#include "Engine/Core/Profiler.h"
#include "Engine/Graphics/Image.h"
#include "Game/Showcase/ShowcaseSession.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace {

using namespace Astral;
using Showcase::SessionInput;

constexpr float kFrameSeconds = 1.0f / 60.0f;

class CaptureScript {
public:
    CaptureScript(std::string outputDir, int width, int height, Showcase::ShowcaseSettings settings)
        : outputDir_(std::move(outputDir)), width_(width), height_(height), jobs_(-1), session_(&jobs_, settings) {}

    Showcase::ShowcaseSession& Session() { return session_; }

    void Run(int frames, const SessionInput& held = {}) {
        for (int i = 0; i < frames; ++i) Frame(held);
    }
    // Presses a key for one frame (edge-triggered in the gameplay loop), then releases it.
    void Tap(void (*press)(SessionInput&), int settleFrames = 0) {
        SessionInput input;
        press(input);
        Frame(input);
        Run(settleFrames);
    }
    void Thought(const std::string& text) {
        SessionInput input;
        input.typedThought = text;
        Frame(input);
    }
    // Holds `held` until `done` or the frame cap; returns whether it finished.
    bool HoldUntil(const SessionInput& held, const std::function<bool()>& done, int maxFrames) {
        for (int i = 0; i < maxFrames; ++i) {
            if (done()) return true;
            Frame(held);
        }
        return done();
    }
    void Shot(const std::string& name) {
        const auto start = std::chrono::steady_clock::now();
        const Graphics::ImageRgba8& image = session_.Showcase().Render(width_, height_);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        renderMs_.push_back(ms);
        std::string error;
        const std::string path = outputDir_ + "/" + name + ".png";
        if (!Graphics::WritePng(path, image, error)) {
            std::fprintf(stderr, "write %s failed: %s\n", path.c_str(), error.c_str());
            failed_ = true;
        }
        const Math::Vec3 p = session_.Player().TransformState().WorldPosition();
        const auto& stats = session_.Showcase().Stats();
        std::printf("shot %-26s hash %016llx render %6.1f ms tris %6zu particles %4zu | pos (%.1f, %.1f) res %.0f "
                    "guard %d focus %d dummy %d/%d visited %zu encounter %d\n",
            name.c_str(), static_cast<unsigned long long>(Graphics::HashImage(image)), ms,
            stats.render.trianglesRasterized, stats.particles, static_cast<double>(p.x), static_cast<double>(p.y),
            static_cast<double>(session_.Shadowblade().Resource()), session_.Shadowblade().IsGuarding() ? 1 : 0,
            session_.Thought().IsFocusActive() ? 1 : 0, session_.Combat().Dummy().health,
            session_.Combat().Dummy().maximumHealth, session_.Interaction().VisitedCount(),
            static_cast<int>(session_.Encounter().State()));
        ++shots_;
    }
    bool Check(bool condition, const char* what) {
        if (!condition) {
            std::fprintf(stderr, "route check failed: %s\n", what);
            failed_ = true;
        }
        return condition;
    }
    bool Finish() {
        std::string error;
        if (!Audio::WriteWav(outputDir_ + "/astral_capture.wav", audio_, 48000, error)) {
            std::fprintf(stderr, "wav: %s\n", error.c_str());
            failed_ = true;
        }
        double total = 0.0, worst = 0.0;
        for (double ms : renderMs_) {
            total += ms;
            worst = std::max(worst, ms);
        }
        std::printf("frames %d shots %d audio %.1f s render avg %.1f ms max %.1f ms (%dx%d, %d workers)\n", frames_,
            shots_, static_cast<double>(audio_.size() / 2) / 48000.0, renderMs_.empty() ? 0.0 : total / static_cast<double>(renderMs_.size()),
            worst, width_, height_, jobs_.WorkerCount());
        return !failed_;
    }

private:
    void Frame(const SessionInput& input) {
        session_.Step(input, kFrameSeconds);
        // Mix exactly the audio this frame covers (fractional frames carried).
        audioCarry_ += kFrameSeconds * 48000.0;
        const int samples = static_cast<int>(audioCarry_);
        audioCarry_ -= samples;
        const std::size_t offset = audio_.size();
        audio_.resize(offset + static_cast<std::size_t>(samples) * 2u);
        session_.Showcase().Mixer().Mix(audio_.data() + offset, samples);
        ++frames_;
    }

    std::string outputDir_;
    int width_, height_;
    Core::JobSystem jobs_;
    Showcase::ShowcaseSession session_;
    std::vector<float> audio_;
    double audioCarry_{};
    std::vector<double> renderMs_;
    int frames_{}, shots_{};
    bool failed_{};
};

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: AstralCapture <output-dir> [width height] [--quality N]\n");
        return 2;
    }
    int width = 1280, height = 720;
    Showcase::ShowcaseSettings settings;
    settings.dynamicResolution = false; // deterministic captures at full resolution
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--quality" && i + 1 < argc) {
            settings.quality = std::atoi(argv[++i]);
        } else if (i + 1 < argc && width == 1280 && std::atoi(argv[i]) > 0) {
            width = std::atoi(argv[i]);
            height = std::atoi(argv[++i]);
        }
    }
    if (width < 64 || height < 36 || width > 7680 || height > 4320 || settings.quality < 0 || settings.quality > 3) {
        std::fprintf(stderr, "invalid resolution or quality\n");
        return 2;
    }

    std::error_code directoryError;
    std::filesystem::create_directories(argv[1], directoryError);
    if (directoryError) {
        std::fprintf(stderr, "cannot create output directory '%s': %s\n", argv[1], directoryError.message().c_str());
        return 2;
    }

    Core::Profiler::Instance().BeginCapture();
    CaptureScript script(argv[1], width, height, settings);
    auto& s = script.Session();
    const auto dummyHealth = [&] { return s.Combat().Dummy().health; };

    // 1. Arrival beside the Rift Wraith training target at the foot of the Mall.
    script.Run(45);
    script.Shot("01_arrival");

    // 2. Light attack: swing trail, sparks on the Hit notify, damage number.
    const int before = dummyHealth();
    script.Tap([](SessionInput& in) { in.light = true; }, 11);
    script.Shot("02_light_attack");
    script.Run(20);
    script.Check(dummyHealth() < before, "light attack damaged the dummy");

    // 3. Guard: shimmer, bubble and the HUD state.
    SessionInput guard;
    guard.guard = true;
    script.Run(30, guard);
    script.Check(s.Shadowblade().IsGuarding(), "guard active while Shift is held");
    script.Shot("03_guard");
    script.Run(10);

    // 4. Thought Commands: typed prompt, then focus slow time x0.35.
    s.Showcase().SetPrompt(true, "focu");
    script.Run(8);
    script.Shot("04_thought_prompt");
    s.Showcase().SetPrompt(false, "");
    script.Thought("focus");
    script.Check(s.Thought().IsFocusActive(), "typed focus activates Thought Focus");
    script.Run(45);
    script.Shot("05_thought_focus");
    script.Thought("focus");
    script.Check(!s.Thought().IsFocusActive(), "second focus returns to normal time");
    script.Run(40);

    // 5. Shadowblade dash forward along the Mall: afterimages, streaks, FOV kick.
    const float startY = s.Player().TransformState().WorldPosition().y;
    script.Tap([](SessionInput& in) { in.dash = true; }, 5);
    script.Shot("06_shadow_dash");
    script.Check(s.Player().TransformState().WorldPosition().y > startY + 5.0f, "dash travels forward");
    script.Run(30);

    // 6. Walk north until the Lincoln Memorial is selectable, discover it.
    SessionInput north;
    north.forward = true;
    script.Check(script.HoldUntil(north, [&] { return s.Interaction().HasSelection(); }, 600), "Lincoln selectable");
    script.Run(20);
    script.Shot("07_lincoln_prompt");
    script.Tap([](SessionInput& in) { in.interact = true; }, 24);
    script.Check(s.Encounter().State() == Scene::LandmarkEncounterState::Active, "Lincoln discovery activates the encounter");
    script.Shot("08_lincoln_discovered");

    // 7. Reflecting Pool: rift over the water, reflections.
    script.Check(script.HoldUntil(north, [&] {
        return s.Interaction().HasSelection() && s.Interaction().SelectedKind() == Scene::LandmarkKind::ReflectingPool;
    }, 900), "Reflecting Pool selectable");
    script.Run(12, north);
    script.Run(20);
    script.Shot("09_reflecting_pool");
    script.Tap([](SessionInput& in) { in.interact = true; }, 10);

    // 8. Washington Monument plaza.
    SessionInput northEast;
    northEast.forward = true;
    northEast.right = true;
    script.HoldUntil(northEast, [&] { return s.Player().TransformState().WorldPosition().x >= 4.5f; }, 300);
    script.Check(script.HoldUntil(north, [&] {
        return s.Interaction().HasSelection() && s.Interaction().SelectedKind() == Scene::LandmarkKind::WashingtonMonument;
    }, 900), "Washington Monument selectable");
    script.Run(20);
    script.Tap([](SessionInput& in) { in.interact = true; }, 24);
    script.Shot("10_washington_monument");
    script.Check(s.Interaction().VisitedCount() == 3, "all three landmarks visited");

    // 9. Return to the training target and finish the encounter.
    SessionInput south;
    south.backward = true;
    SessionInput southWest;
    southWest.backward = true;
    southWest.left = true;
    script.HoldUntil(southWest, [&] { return s.Player().TransformState().WorldPosition().x <= 0.5f; }, 300);
    script.Check(script.HoldUntil(south, [&] { return s.Player().TransformState().WorldPosition().y <= 0.0f; }, 1500),
        "returned to the training ground");
    script.Run(60);
    script.Tap([](SessionInput& in) { in.heavy = true; }, 20);
    script.Shot("11_heavy_attack");
    script.Run(70);
    script.Tap([](SessionInput& in) { in.fatal = true; }, 26);
    script.Shot("12_fatal_strike");
    for (int attempt = 0; attempt < 40 && !s.Combat().Dummy().IsDefeated(); ++attempt) {
        script.Tap([](SessionInput& in) { in.light = true; }, 40);
    }
    script.Run(30);
    script.Check(s.Encounter().State() == Scene::LandmarkEncounterState::Completed, "encounter completes on defeat");
    script.Shot("13_encounter_complete");

    // 10. Engine statistics overlay and the low scalability preset.
    s.Showcase().ToggleStats();
    script.Run(2);
    script.Shot("14_stats_overlay");
    s.Showcase().ToggleStats();
    while (s.Showcase().Quality() != 0) s.Showcase().CycleQuality();
    script.Run(2);
    script.Shot("15_quality_low");

    Core::Profiler::Instance().EndCapture();
    std::string error;
    if (!Core::Profiler::Instance().WriteChromeTrace(std::string(argv[1]) + "/astral_trace.json", error)) {
        std::fprintf(stderr, "trace: %s\n", error.c_str());
    }
    for (const auto& zone : Core::Profiler::Instance().Summarize()) {
        if (zone.count > 0) {
            std::printf("zone %-28s count %6llu avg %7.2f ms max %7.2f ms\n", zone.name.c_str(),
                static_cast<unsigned long long>(zone.count), zone.totalMs / static_cast<double>(zone.count), zone.maxMs);
        }
    }
    return script.Finish() ? 0 : 1;
}
