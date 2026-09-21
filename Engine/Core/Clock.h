#pragma once

#include "Engine/Core/FrameTimingCapture.h"
#include "Engine/Core/ProcessMemoryCapture.h"
#include "Engine/Core/SimulationTimeStep.h"

#include <chrono>
#include <cstdint>

namespace Astral::Core {

class Clock {
public:
    Clock();
    ~Clock();
    float Tick();
    double ElapsedSeconds() const;

private:
    std::chrono::steady_clock::time_point lastTick_;
    std::chrono::steady_clock::time_point start_;
    std::uint64_t frameIndex_ = 0;
    FrameTimingCapture frameTimingCapture_;
    ProcessMemoryCapture processMemoryCapture_;
    SimulationTimeStep simulationTimeStep_;
    bool processMemoryFailureReported_ = false;
};

} // namespace Astral::Core
