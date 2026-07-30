#pragma once

#include "Engine/Math/Math.h"

#include <array>

namespace Astral::Scene {

enum class LandmarkKind {
    LincolnMemorial,
    ReflectingPool,
    WashingtonMonument,
};

struct LandmarkProxy {
    LandmarkKind kind{};
    Math::Vec3 position{};
    Math::Vec3 dimensions{};
};

struct GroundGrid {
    float minimumX{-20.0f};
    float maximumX{20.0f};
    float minimumZ{-5.0f};
    float maximumZ{80.0f};
    float spacing{2.0f};
};

class WorldBlockout {
public:
    const GroundGrid& Grid() const { return grid_; }
    const std::array<LandmarkProxy, 3>& Landmarks() const { return landmarks_; }
    Math::Vec3 GroundPosition(const Math::Vec3& controllerPosition) const;

private:
    GroundGrid grid_{};
    std::array<LandmarkProxy, 3> landmarks_{{
        {LandmarkKind::LincolnMemorial, {-8.0f, 0.0f, 18.0f}, {12.0f, 5.0f, 7.0f}},
        {LandmarkKind::ReflectingPool, {4.0f, 0.0f, 39.0f}, {10.0f, 0.35f, 20.0f}},
        {LandmarkKind::WashingtonMonument, {5.0f, 0.0f, 68.0f}, {5.0f, 22.0f, 5.0f}},
    }};
};

} // namespace Astral::Scene