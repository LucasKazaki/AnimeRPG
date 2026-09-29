#pragma once

// Opt-in Astral renderer for the Win32 game loop. Off by default: the GDI
// wireframe renderer stays the default and every native smoke runs unchanged.
// Enable with F2 at runtime or ASTRAL_RENDER_MODE=astral at launch.
//
// Win32Application owns the gameplay domains and calls this presenter at its
// existing action sites; every hook is a no-op while Astral mode is off.
// Controls in Astral mode: F3 stats, F4 quality preset, Enter opens the typed
// Thought prompt (Enter submits, Escape closes it instead of quitting).

#include "Engine/Core/JobSystem.h"
#include "Game/Showcase/GameplayBridge.h"
#include "Game/Showcase/MallShowcase.h"

#include <windows.h>
#include <mmsystem.h> // waveOut (not pulled in by WIN32_LEAN_AND_MEAN)

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace Astral::Showcase {

// waveOut stream fed by the showcase mixer on its own thread. Construction
// failure leaves it inactive (silent); the game never depends on audio.
class Win32WaveOutput {
public:
    explicit Win32WaveOutput(Audio::AudioMixer& mixer);
    ~Win32WaveOutput();
    Win32WaveOutput(const Win32WaveOutput&) = delete;
    Win32WaveOutput& operator=(const Win32WaveOutput&) = delete;
    bool Active() const { return device_ != nullptr; }

private:
    static constexpr int kBufferCount = 4;
    static constexpr int kFramesPerBuffer = 1024; // ~21 ms at 48 kHz
    void Fill(int index);
    void Pump();

    Audio::AudioMixer& mixer_;
    HWAVEOUT device_{};
    HANDLE event_{};
    WAVEHDR headers_[kBufferCount]{};
    std::vector<std::int16_t> pcm_[kBufferCount];
    std::vector<float> scratch_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

class Win32AstralPresenter {
public:
    Win32AstralPresenter();
    ~Win32AstralPresenter();

    static bool RequestedByEnvironment();
    bool Enabled() const { return enabled_ && showcase_ != nullptr; }
    // Lazily builds the showcase on first enable; on failure stays in GDI mode.
    bool SetEnabled(bool enabled, const Scene::WorldBlockout& world);
    // Edge-triggered F2 (toggle), F3 (stats), F4 (quality).
    void HandleFunctionKeys(bool f2, bool f3, bool f4, const Scene::WorldBlockout& world);

    // Typed Thought prompt (WM_CHAR). Returns true when the character was consumed.
    bool HandleChar(wchar_t character);
    bool PromptOpen() const { return Enabled() && promptOpen_; }
    // Gameplay keys are ignored while typing, and the Escape that closed the
    // prompt is held back until released so it never also quits the game.
    bool SuppressKey(int virtualKey);
    // Returns (and clears) a thought submitted with Enter this frame.
    std::string TakeSubmittedThought();

    // Per-frame gameplay hooks (no-ops while disabled).
    void BeginFrame();
    void AfterWalk(Scene::PlayerController& player, Math::Vec3 before);
    void OnAttack(bool heavy, const Scene::AttackReport& report);
    void OnDash(Scene::PlayerController& player, Math::Vec3 from, const Scene::ShadowActionReport& report);
    void OnFatalStrike(const Scene::ShadowActionReport& report);
    void OnThought(const Scene::ThoughtCommandReport& report);
    void OnInteract(const Scene::LandmarkInteractionReport& report);
    void OnEncounterChanged();
    void Update(const GameplayView& view, Math::Vec3 playerPosition, float realDeltaSeconds);
    // Draws the frame into the client area. Returns false when nothing was drawn.
    bool Present(HDC deviceContext, const RECT& viewport);

private:
    std::unique_ptr<Core::JobSystem> jobs_;
    std::unique_ptr<MallShowcase> showcase_;
    std::unique_ptr<Win32WaveOutput> audio_; // destroyed before the mixer it reads
    FrameEvents events_{};
    std::vector<std::uint32_t> bgra_;
    std::string promptText_;
    std::string submitted_;
    bool enabled_{};
    bool promptOpen_{};
    bool escapeLatched_{};
    bool f2Held_{}, f3Held_{}, f4Held_{};
};

} // namespace Astral::Showcase
