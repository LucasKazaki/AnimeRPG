#include "Engine/Scene/Transform.h"

#include <cmath>
#include <limits>

namespace Astral::Scene {

bool Transform::TryWorldPosition(Math::Vec3& output) const {
    // Detect cycles without allocation. This cannot validate dangling raw pointers;
    // scene ownership still must keep every referenced transform alive.
    const Transform* slow = this;
    const Transform* fast = this;
    while (fast != nullptr && fast->parent != nullptr) {
        slow = slow->parent;
        fast = fast->parent->parent;
        if (slow == fast) return false;
    }

    double x = 0.0, y = 0.0, z = 0.0;
    for (const Transform* node = this; node != nullptr; node = node->parent) {
        if (!Math::IsFinite(node->localPosition)) return false;
        x += node->localPosition.x;
        y += node->localPosition.y;
        z += node->localPosition.z;
    }
    constexpr double limit = std::numeric_limits<float>::max();
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)
        || std::abs(x) > limit || std::abs(y) > limit || std::abs(z) > limit) return false;
    output = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
    return true;
}

Math::Vec3 Transform::WorldPosition() const {
    Math::Vec3 output{};
    if (TryWorldPosition(output)) return output;
    const float invalid = std::numeric_limits<float>::quiet_NaN();
    return {invalid, invalid, invalid};
}

} // namespace Astral::Scene
