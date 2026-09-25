#pragma once

#include "Engine/Math/Math.h"

namespace Astral::Scene {

struct Transform {
    Math::Vec3 localPosition{};
    Math::Vec3 localScale{1.0f, 1.0f, 1.0f};
    const Transform* parent{};

    // Translation-only hierarchy. Parents must outlive their children.
    // Checked resolution rejects cycles, non-finite data and unrepresentable sums
    // without modifying output; the legacy wrapper returns NaNs on failure.
    bool TryWorldPosition(Math::Vec3& output) const;
    Math::Vec3 WorldPosition() const;
};

} // namespace Astral::Scene
