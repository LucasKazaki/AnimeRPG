#include "Engine/Core/Clock.h"
#include "Engine/Core/SimulationTimeStep.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
int failures = 0;

void Check(bool condition, const std::string& name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

void SetEnvironment(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value ? value : "");
#else
    if (value) {
        setenv(name, value, 1);
    } else {
        unsetenv(name);
    }
#endif
}

void SetFixedHz(const char* value) {
    SetEnvironment("ASTRAL_SIMULATION_FIXED_HZ", value);
}

void ClearCaptureEnvironment() {
    SetEnvironment("ASTRAL_FRAME_TIMING_CSV", nullptr);
    SetEnvironment("ASTRAL_FRAME_TIMING_WARMUP_FRAMES", nullptr);
    SetEnvironment("ASTRAL_FRAME_TIMING_MAX_SAMPLES", nullptr);
    SetEnvironment("ASTRAL_PROCESS_MEMORY_CSV", nullptr);
    SetEnvironment("ASTRAL_PROCESS_MEMORY_WARMUP_FRAMES", nullptr);
    SetEnvironment("ASTRAL_PROCESS_MEMORY_SAMPLE_EVERY_FRAMES", nullptr);
    SetEnvironment("ASTRAL_PROCESS_MEMORY_MAX_SAMPLES", nullptr);
}

void TestVariableByDefault() {
    SetFixedHz(nullptr);
    Astral::Core::SimulationTimeStep step;
    std::string error = "stale";
    Check(step.ConfigureFromEnvironment(error)
        == Astral::Core::SimulationTimeStepEnvironmentStatus::Variable,
        "unset environment selects variable time");
    Check(!step.Fixed(), "unset environment is not fixed");
    Check(step.FixedHz() == 0, "variable time reports zero fixed Hz");
    Check(step.FixedDeltaSeconds() == 0.0f, "variable time reports no fixed delta");
    Check(std::abs(step.Resolve(0.0375f) - 0.0375f) < 1e-7f,
        "variable time preserves wall delta");
    Check(error.empty(), "successful configuration clears stale error");
}

void TestValidFixedRates() {
    for (const auto& value : {std::pair<const char*, std::uint32_t>{"1", 1},
                              {"60", 60}, {"240", 240}, {"1000", 1000}}) {
        SetFixedHz(value.first);
        Astral::Core::SimulationTimeStep step;
        std::string error;
        Check(step.ConfigureFromEnvironment(error)
            == Astral::Core::SimulationTimeStepEnvironmentStatus::Fixed,
            std::string("valid fixed rate accepted: ") + value.first);
        Check(step.Fixed(), std::string("fixed flag set: ") + value.first);
        Check(step.FixedHz() == value.second, std::string("fixed Hz retained: ") + value.first);
        const float expected = 1.0f / static_cast<float>(value.second);
        Check(std::abs(step.FixedDeltaSeconds() - expected) < 1e-7f,
            std::string("fixed delta derived: ") + value.first);
        Check(std::abs(step.Resolve(0.5f) - expected) < 1e-7f,
            std::string("fixed delta ignores wall timing: ") + value.first);
    }
}

void TestInvalidRatesFailClosedAtParserBoundary() {
#ifndef _WIN32
    SetFixedHz("");
    Astral::Core::SimulationTimeStep emptyStep;
    std::string emptyError;
    Check(emptyStep.ConfigureFromEnvironment(emptyError)
        == Astral::Core::SimulationTimeStepEnvironmentStatus::Invalid,
        "empty fixed rate is rejected when representable in the process environment");
#endif

    const char* invalid[] = {"0", "-1", "+1", " 60", "60 ", "60hz", "1001",
                             "4294967296", "999999999999999999999999999999"};
    for (const char* value : invalid) {
        SetFixedHz(value);
        Astral::Core::SimulationTimeStep step;
        std::string error;
        Check(step.ConfigureFromEnvironment(error)
            == Astral::Core::SimulationTimeStepEnvironmentStatus::Invalid,
            std::string("invalid rate rejected: ") + value);
        Check(!step.Fixed(), std::string("invalid rate leaves variable state: ") + value);
        Check(step.FixedHz() == 0, std::string("invalid rate clears Hz: ") + value);
        Check(!error.empty(), std::string("invalid rate reports error: ") + value);
    }
}

void TestFixedResolutionIndependentOfWallDelta() {
    SetFixedHz("120");
    Astral::Core::SimulationTimeStep step;
    std::string error;
    Check(step.ConfigureFromEnvironment(error)
        == Astral::Core::SimulationTimeStepEnvironmentStatus::Fixed,
        "120 Hz configuration accepted");
    const float expected = 1.0f / 120.0f;
    for (float wall : {0.0001f, 0.005f, 0.016f, 0.050f, 2.0f}) {
        Check(std::abs(step.Resolve(wall) - expected) < 1e-7f,
            "fixed step remains independent of wall delta");
    }
}

void TestReconfigureResetsState() {
    SetFixedHz("60");
    Astral::Core::SimulationTimeStep step;
    std::string error;
    Check(step.ConfigureFromEnvironment(error)
        == Astral::Core::SimulationTimeStepEnvironmentStatus::Fixed,
        "initial fixed configuration accepted");

    SetFixedHz(nullptr);
    Check(step.ConfigureFromEnvironment(error)
        == Astral::Core::SimulationTimeStepEnvironmentStatus::Variable,
        "reconfigure to variable accepted");
    Check(!step.Fixed() && step.FixedHz() == 0, "variable reconfigure clears fixed state");

    SetFixedHz("bad");
    Check(step.ConfigureFromEnvironment(error)
        == Astral::Core::SimulationTimeStepEnvironmentStatus::Invalid,
        "invalid reconfigure rejected");
    Check(!step.Fixed() && step.FixedHz() == 0, "invalid reconfigure leaves no stale fixed state");
}

void TestClockReturnsFixedSimulationDelta() {
    ClearCaptureEnvironment();
    SetFixedHz("60");
    Astral::Core::Clock clock;
    const float expected = 1.0f / 60.0f;
    Check(std::abs(clock.Tick() - expected) < 1e-7f,
        "Clock returns configured fixed simulation delta on first tick");
    Check(std::abs(clock.Tick() - expected) < 1e-7f,
        "Clock returns configured fixed simulation delta independent of wall interval");
    SetFixedHz(nullptr);
}

void TestClockRejectsMalformedFixedSimulationConfiguration() {
    ClearCaptureEnvironment();
    SetFixedHz("60oops");
    bool rejected = false;
    try {
        Astral::Core::Clock clock;
        (void)clock;
    } catch (const std::runtime_error& error) {
        rejected = std::string(error.what()).find("ASTRAL_SIMULATION_FIXED_HZ") != std::string::npos;
    }
    Check(rejected, "Clock fails closed on malformed fixed simulation configuration");
    SetFixedHz(nullptr);
}
} // namespace

int main() {
    ClearCaptureEnvironment();
    TestVariableByDefault();
    TestValidFixedRates();
    TestInvalidRatesFailClosedAtParserBoundary();
    TestFixedResolutionIndependentOfWallDelta();
    TestReconfigureResetsState();
    TestClockReturnsFixedSimulationDelta();
    TestClockRejectsMalformedFixedSimulationConfiguration();
    SetFixedHz(nullptr);
    ClearCaptureEnvironment();
    if (failures != 0) {
        std::cerr << failures << " simulation time-step checks failed\n";
        return 1;
    }
    std::cout << "SimulationTimeStepTests: 7 groups passed\n";
    return 0;
}
