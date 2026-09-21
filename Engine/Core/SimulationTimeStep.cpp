#include "Engine/Core/SimulationTimeStep.h"

#include <charconv>
#include <cstdlib>

namespace {
constexpr const char* kFixedHzEnvironment = "ASTRAL_SIMULATION_FIXED_HZ";
constexpr std::uint32_t kMinimumFixedHz = 1;
constexpr std::uint32_t kMaximumFixedHz = 1000;
} // namespace

namespace Astral::Core {

SimulationTimeStepEnvironmentStatus SimulationTimeStep::ConfigureFromEnvironment(std::string& error) {
    fixedHz_ = 0;
    error.clear();

    const char* text = std::getenv(kFixedHzEnvironment);
    if (!text) {
        return SimulationTimeStepEnvironmentStatus::Variable;
    }
    if (*text == '\0' || *text == '+' || *text == '-' || *text == ' ' || *text == '\t') {
        error = "ASTRAL_SIMULATION_FIXED_HZ must be an integer in [1, 1000]";
        return SimulationTimeStepEnvironmentStatus::Invalid;
    }

    std::uint32_t parsed = 0;
    const char* end = text;
    while (*end != '\0') {
        ++end;
    }
    const auto result = std::from_chars(text, end, parsed, 10);
    if (result.ec != std::errc{} || result.ptr != end
        || parsed < kMinimumFixedHz || parsed > kMaximumFixedHz) {
        error = "ASTRAL_SIMULATION_FIXED_HZ must be an integer in [1, 1000]";
        return SimulationTimeStepEnvironmentStatus::Invalid;
    }

    fixedHz_ = parsed;
    return SimulationTimeStepEnvironmentStatus::Fixed;
}

float SimulationTimeStep::FixedDeltaSeconds() const noexcept {
    return fixedHz_ == 0 ? 0.0f : 1.0f / static_cast<float>(fixedHz_);
}

float SimulationTimeStep::Resolve(float wallDeltaSeconds) const noexcept {
    return fixedHz_ == 0 ? wallDeltaSeconds : FixedDeltaSeconds();
}

} // namespace Astral::Core
