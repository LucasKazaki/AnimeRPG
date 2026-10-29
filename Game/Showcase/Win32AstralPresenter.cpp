#include "Game/Showcase/Win32AstralPresenter.h"

#include <algorithm>
#include <cmath>
#include <cwctype>

namespace Astral::Showcase {

Win32AstralPresenter::Win32AstralPresenter() = default;

Win32AstralPresenter::~Win32AstralPresenter() {
    audio_.reset();
    showcase_.reset();
    jobs_.reset();
}

bool Win32AstralPresenter::RequestedByEnvironment() {
    wchar_t value[16]{};
    const DWORD length = GetEnvironmentVariableW(L"ASTRAL_RENDER_MODE", value, 16);
    if (length == 0 || length >= 16) return false;
    std::wstring mode(value, length);
    for (wchar_t& c : mode) c = static_cast<wchar_t>(std::towlower(c));
    return mode == L"astral";
}

bool Win32AstralPresenter::SetEnabled(bool enabled, const Scene::WorldBlockout& world) {
    if (!enabled) {
        if (promptOpen_) capturedKeys_.QuarantineAllUntilRelease();
        enabled_ = false;
        promptOpen_ = false;
        promptText_.clear();
        if (showcase_) {
            showcase_->SetPrompt(false, {});
            showcase_->Mixer().SetBusVolume(Audio::Bus::Master, 0.0f);
        }
        return false;
    }
    if (!showcase_) {
        try {
            jobs_ = std::make_unique<Core::JobSystem>(-1);
            showcase_ = std::make_unique<MallShowcase>(world, jobs_.get());
        } catch (...) {
            showcase_.reset();
            jobs_.reset();
            enabled_ = false;
            return false;
        }
        try {
            audio_ = std::make_unique<Win32WaveOutput>(showcase_->Mixer());
        } catch (...) {
            audio_.reset(); // silent is fine
        }
    }
    showcase_->Mixer().SetBusVolume(Audio::Bus::Master, 1.0f);
    enabled_ = true;
    return true;
}

void Win32AstralPresenter::HandleFunctionKeys(bool f2, bool f3, bool f4, const Scene::WorldBlockout& world) {
    if (f2 && !f2Held_) SetEnabled(!Enabled(), world);
    if (Enabled()) {
        if (f3 && !f3Held_) showcase_->ToggleStats();
        if (f4 && !f4Held_) showcase_->CycleQuality();
    }
    f2Held_ = f2;
    f3Held_ = f3;
    f4Held_ = f4;
}

bool Win32AstralPresenter::HandleChar(wchar_t character, bool repeated) {
    if (!Enabled()) return false;
    if (character == L'\r' && repeated) return true;
    if (!promptOpen_) {
        if (character != L'\r') return false;
        promptOpen_ = true;
        promptText_.clear();
    } else if (character == L'\r') {
        submitted_ = promptText_;
        capturedKeys_.QuarantineAllUntilRelease();
        promptOpen_ = false;
        promptText_.clear();
    } else if (character == 0x1B) {
        capturedKeys_.QuarantineAllUntilRelease();
        promptOpen_ = false;
        promptText_.clear();
    } else if (character == L'\b') {
        if (!promptText_.empty()) promptText_.pop_back();
    } else if (character >= 0x20 && character < 0x7F && promptText_.size() < 40) {
        promptText_.push_back(static_cast<char>(character));
    }
    showcase_->SetPrompt(promptOpen_, promptText_);
    return true;
}

void Win32AstralPresenter::OnFocusLost() {
    capturedKeys_.QuarantineAllUntilRelease();
    promptOpen_ = false;
    promptText_.clear();
    submitted_.clear();
    if (showcase_) showcase_->SetPrompt(false, {});
}

bool Win32AstralPresenter::SuppressKey(int virtualKey, bool physicallyDown, bool background) {
    return capturedKeys_.Suppress(virtualKey, physicallyDown, PromptOpen() || background);
}

std::string Win32AstralPresenter::TakeSubmittedThought() {
    std::string text;
    text.swap(submitted_);
    return text;
}

void Win32AstralPresenter::BeginFrame() { events_ = {}; }

void Win32AstralPresenter::AfterWalk(Scene::PlayerController& player, Math::Vec3 before) {
    if (Enabled()) ResolveWalk(player, before, *showcase_);
}

void Win32AstralPresenter::OnAttack(bool heavy, const Scene::AttackReport& report) {
    events_.lightAttack = !heavy;
    events_.heavyAttack = heavy;
    events_.attack = report;
}

void Win32AstralPresenter::OnDash(Scene::PlayerController& player, Math::Vec3 from, const Scene::ShadowActionReport& report) {
    if (report.result == Scene::ShadowActionResult::Activated) {
        ResolveDash(player, from, report, Enabled() ? showcase_.get() : nullptr, events_);
    } else {
        events_.dash = true;
        events_.dashReport = report;
    }
}

void Win32AstralPresenter::OnFatalStrike(const Scene::ShadowActionReport& report) {
    events_.fatalStrike = true;
    events_.fatal = report;
}

void Win32AstralPresenter::OnThought(const Scene::ThoughtCommandReport& report) {
    events_.thoughtCommand = true;
    events_.thought = report;
}

void Win32AstralPresenter::OnInteract(const Scene::LandmarkInteractionReport& report) {
    events_.interacted = true;
    events_.interaction = report;
}

void Win32AstralPresenter::OnEncounterChanged() { events_.encounterChanged = true; }

void Win32AstralPresenter::Update(const GameplayView& view, Math::Vec3 playerPosition, float realDeltaSeconds) {
    if (Enabled()) showcase_->Update(view, events_, playerPosition, realDeltaSeconds);
}

bool Win32AstralPresenter::Present(HDC deviceContext, const RECT& viewport) {
    if (!Enabled()) return false;
    const int width = viewport.right - viewport.left;
    const int height = viewport.bottom - viewport.top;
    if (width <= 0 || height <= 0) return false;
    const Graphics::ImageRgba8& image = showcase_->Render(width, height);
    const std::size_t count = static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height);
    bgra_.resize(count);
    const std::uint8_t* source = image.pixels.data();
    for (std::size_t index = 0; index < count; ++index, source += 4) {
        bgra_[index] = 0xFF000000u | (static_cast<std::uint32_t>(source[0]) << 16)
            | (static_cast<std::uint32_t>(source[1]) << 8) | static_cast<std::uint32_t>(source[2]);
    }
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = image.width;
    info.bmiHeader.biHeight = -image.height; // top-down rows
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    return StretchDIBits(deviceContext, viewport.left, viewport.top, width, height, 0, 0, image.width, image.height,
               bgra_.data(), &info, DIB_RGB_COLORS, SRCCOPY)
        != 0;
}

} // namespace Astral::Showcase
