#include "Engine/Core/Clock.h"

#include <cstdio>
#include <stdexcept>
#include <string>

namespace Astral::Core {

Clock::Clock() : lastTick_(std::chrono::steady_clock::now()), start_(lastTick_) {
    std::string error;
    frameTimingStatus_ = frameTimingCapture_.ConfigureFromEnvironment(error);
    if (frameTimingStatus_ == FrameTimingEnvironmentStatus::Invalid) {
        std::fprintf(stderr, "Astral frame timing capture configuration rejected: %s\n", error.c_str());
    }

    error.clear();
    processMemoryStatus_ = processMemoryCapture_.ConfigureFromEnvironment(error);
    if (processMemoryStatus_ == ProcessMemoryEnvironmentStatus::Invalid) {
        std::fprintf(stderr, "Astral process memory capture configuration rejected: %s\n",
            error.c_str());
    }

    error.clear();
    const auto timeStepStatus = simulationTimeStep_.ConfigureFromEnvironment(error);
    if (timeStepStatus == SimulationTimeStepEnvironmentStatus::Invalid) {
        throw std::runtime_error("Astral simulation time-step configuration rejected: " + error);
    }
}

Clock::~Clock() {
    if (frameTimingCapture_.Enabled()) {
        std::string error;
        if (!frameTimingCapture_.Flush(error)) {
            std::fprintf(stderr, "Astral frame timing capture was not published: %s\n",
                error.c_str());
        }
    }

    if (processMemoryCapture_.Enabled()) {
        std::string error;
        if (!processMemoryCapture_.Flush(error)) {
            std::fprintf(stderr, "Astral process memory capture was not published: %s\n",
                error.c_str());
        }
    }
}

float Clock::Tick() {
    const auto now = std::chrono::steady_clock::now();
    const auto wallDelta = std::chrono::duration<float>(now - lastTick_).count();
    lastTick_ = now;
    if (frameTimingCapture_.Enabled()) {
        frameTimingCapture_.Record(frameIndex_, static_cast<double>(wallDelta) * 1000.0);
    }
    if (processMemoryCapture_.Enabled()
        && !processMemoryCapture_.RecordCurrentProcess(frameIndex_)
        && !processMemoryFailureReported_) {
        std::fprintf(stderr,
            "Astral process memory sampler failed; final memory evidence will not be published\n");
        processMemoryFailureReported_ = true;
    }
    ++frameIndex_;
    return simulationTimeStep_.Resolve(wallDelta);
}

double Clock::ElapsedSeconds() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
}

} // namespace Astral::Core
