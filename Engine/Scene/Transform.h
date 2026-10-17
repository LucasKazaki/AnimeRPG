#pragma once

#include "Engine/Math/Math.h"

namespace Astral::Scene {

struct Transform {
    Math::Vec3 localPosition{};
    Math::Vec3 localScale{1.0f, 1.0f, 1.0f};
    // Borrowed parent: it must stay alive throughout hierarchy queries.
    const Transform* parent{};

    // Translation-only hierarchy. Reject cycles, nonfinite data and final sums
    // outside float range without modifying output. Hierarchy mutation must not
    // race a query. The compatibility wrapper returns three NaNs on failure.
    bool TryWorldPosition(Math::Vec3& output) const;
    Math::Vec3 WorldPosition() const;
};

} // namespace Astral::Scene
