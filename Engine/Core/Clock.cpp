#include "Engine/Core/Clock.h"

#include <cstdio>
#include <string>

namespace Astral::Core {

Clock::Clock() : lastTick_(std::chrono::steady_clock::now()), start_(lastTick_) {
    std::string error;
    const auto status = frameTimingCapture_.ConfigureFromEnvironment(error);
    if (status == FrameTimingEnvironmentStatus::Invalid) {
        std::fprintf(stderr, "Astral frame timing capture configuration rejected: %s\n", error.c_str());
    }
}

Clock::~Clock() {
    if (!frameTimingCapture_.Enabled()) return;
    std::string error;
    if (!frameTimingCapture_.Flush(error)) {
        std::fprintf(stderr, "Astral frame timing capture was not published: %s\n", error.c_str());
    }
}

float Clock::Tick() {
    const auto now = std::chrono::steady_clock::now();
    const auto delta = std::chrono::duration<float>(now - lastTick_).count();
    lastTick_ = now;
    if (frameTimingCapture_.Enabled()) {
        frameTimingCapture_.Record(frameIndex_, static_cast<double>(delta) * 1000.0);
    }
    ++frameIndex_;
    return delta;
}

double Clock::ElapsedSeconds() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
}

} // namespace Astral::Core
