#include "Engine/Scene/PlayerController.h"

#include <algorithm>
#include <cmath>

namespace Astral::Scene {

PlayerController::PlayerController(float movementSpeed, MovementBounds bounds)
    : movementSpeed_(movementSpeed), bounds_(bounds) {
    transform_.localPosition = {
        (bounds_.minX + bounds_.maxX) * 0.5f,
        (bounds_.minY + bounds_.maxY) * 0.5f,
        0.0f,
    };
    ClampToBounds();
}

void PlayerController::Update(const MovementInput& input, float deltaSeconds) {
    if (deltaSeconds <= 0.0f) {
        return;
    }

    float horizontal = 0.0f;
    float vertical = 0.0f;
    if (input.left) horizontal -= 1.0f;
    if (input.right) horizontal += 1.0f;
    if (input.backward) vertical -= 1.0f;
    if (input.forward) vertical += 1.0f;

    const float length = std::sqrt(horizontal * horizontal + vertical * vertical);
    if (length > 0.0f) {
        horizontal /= length;
        vertical /= length;
    }

    transform_.localPosition.x += horizontal * movementSpeed_ * deltaSeconds;
    transform_.localPosition.y += vertical * movementSpeed_ * deltaSeconds;
    ClampToBounds();
}

void PlayerController::SetPosition(const Math::Vec3& position) {
    transform_.localPosition = position;
    ClampToBounds();
}

bool PlayerController::IsWithinBounds() const {
    const Math::Vec3 position = transform_.localPosition;
    return position.x >= bounds_.minX && position.x <= bounds_.maxX
        && position.y >= bounds_.minY && position.y <= bounds_.maxY;
}

void PlayerController::ClampToBounds() {
    transform_.localPosition.x = std::clamp(transform_.localPosition.x, bounds_.minX, bounds_.maxX);
    transform_.localPosition.y = std::clamp(transform_.localPosition.y, bounds_.minY, bounds_.maxY);
}

} // namespace Astral::Scene
