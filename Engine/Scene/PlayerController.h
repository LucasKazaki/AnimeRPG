#pragma once

#include "Engine/Scene/Transform.h"

namespace Astral::Scene {

struct MovementInput {
    bool forward{};
    bool backward{};
    bool left{};
    bool right{};
};

struct MovementBounds {
    float minX{-9.0f};
    float maxX{9.0f};
    float minY{-5.0f};
    float maxY{5.0f};
};

class PlayerController {
public:
    // Throws std::invalid_argument for non-finite/negative speed or invalid bounds.
    // Zero speed and zero-width finite bounds are valid.
    explicit PlayerController(float movementSpeed = 5.0f, MovementBounds bounds = {});

    // Invalid time/position inputs are no-ops; finite movement clamps to bounds.
    void Update(const MovementInput& input, float deltaSeconds);
    void SetPosition(const Math::Vec3& position);
    const Transform& TransformState() const { return transform_; }
    bool IsWithinBounds() const;

private:
    void ClampToBounds();

    Transform transform_{};
    float movementSpeed_{};
    MovementBounds bounds_{};
};

} // namespace Astral::Scene
