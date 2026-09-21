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
    std::uint32_t FixedSimulationHz() const noexcept { return simulationTimeStep_.FixedHz(); }
    FrameTimingEnvironmentStatus FrameTimingStatus() const noexcept { return frameTimingStatus_; }
    const FrameTimingCaptureConfig& FrameTimingConfig() const noexcept {
        return frameTimingCapture_.Config();
    }
    ProcessMemoryEnvironmentStatus ProcessMemoryStatus() const noexcept {
        return processMemoryStatus_;
    }
    const ProcessMemoryCaptureConfig& ProcessMemoryConfig() const noexcept {
        return processMemoryCapture_.Config();
    }

private:
    std::chrono::steady_clock::time_point lastTick_;
    std::chrono::steady_clock::time_point start_;
    std::uint64_t frameIndex_ = 0;
    FrameTimingCapture frameTimingCapture_;
    ProcessMemoryCapture processMemoryCapture_;
    SimulationTimeStep simulationTimeStep_;
    FrameTimingEnvironmentStatus frameTimingStatus_ = FrameTimingEnvironmentStatus::NotRequested;
    ProcessMemoryEnvironmentStatus processMemoryStatus_ = ProcessMemoryEnvironmentStatus::NotRequested;
    bool processMemoryFailureReported_ = false;
};

} // namespace Astral::Core
