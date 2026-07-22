#pragma once

#include <chrono>

namespace Astral::Core {

class Clock {
public:
    Clock();
    float Tick();
    double ElapsedSeconds() const;

private:
    std::chrono::steady_clock::time_point lastTick_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace Astral::Core
