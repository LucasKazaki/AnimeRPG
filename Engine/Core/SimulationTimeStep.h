#pragma once

#include <cstdint>
#include <string>

namespace Astral::Core {

enum class SimulationTimeStepEnvironmentStatus {
    Variable,
    Fixed,
    Invalid,
};

class SimulationTimeStep {
public:
    SimulationTimeStepEnvironmentStatus ConfigureFromEnvironment(std::string& error);

    bool Fixed() const noexcept { return fixedHz_ != 0; }
    std::uint32_t FixedHz() const noexcept { return fixedHz_; }
    float FixedDeltaSeconds() const noexcept;
    float Resolve(float wallDeltaSeconds) const noexcept;

private:
    std::uint32_t fixedHz_{};
};

} // namespace Astral::Core
