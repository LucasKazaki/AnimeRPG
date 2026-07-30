#include "Engine/Scene/WorldBlockout.h"

namespace Astral::Scene {

Math::Vec3 WorldBlockout::GroundPosition(const Math::Vec3& controllerPosition) const {
    return {controllerPosition.x, 0.0f, controllerPosition.y};
}

} // namespace Astral::Scene