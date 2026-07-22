#include "Engine/Core/Clock.h"

namespace Astral::Core {

Clock::Clock() : lastTick_(std::chrono::steady_clock::now()), start_(lastTick_) {}

float Clock::Tick() {
    const auto now = std::chrono::steady_clock::now();
    const auto delta = std::chrono::duration<float>(now - lastTick_).count();
    lastTick_ = now;
    return delta;
}

double Clock::ElapsedSeconds() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
}

} // namespace Astral::Core
