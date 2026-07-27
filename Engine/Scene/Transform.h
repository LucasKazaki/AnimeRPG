#pragma once

#include "Engine/Math/Math.h"

namespace Astral::Scene {

struct Transform {
    Math::Vec3 localPosition{};
    Math::Vec3 localScale{1.0f, 1.0f, 1.0f};
    const Transform* parent{};

    Math::Vec3 WorldPosition() const;
};

} // namespace Astral::Scene
