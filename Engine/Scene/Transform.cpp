#include "Engine/Scene/Transform.h"

namespace Astral::Scene {

Math::Vec3 Transform::WorldPosition() const {
    if (parent == nullptr) {
        return localPosition;
    }
    const Math::Vec3 parentPosition = parent->WorldPosition();
    return {parentPosition.x + localPosition.x, parentPosition.y + localPosition.y,
        parentPosition.z + localPosition.z};
}

} // namespace Astral::Scene
