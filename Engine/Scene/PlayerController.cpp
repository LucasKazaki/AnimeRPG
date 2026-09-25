#include "Engine/Scene/PlayerController.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Astral::Scene {

PlayerController::PlayerController(float movementSpeed, MovementBounds bounds)
    : movementSpeed_(movementSpeed), bounds_(bounds) {
    if (!std::isfinite(movementSpeed_) || movementSpeed_ < 0.0f
        || !std::isfinite(bounds_.minX) || !std::isfinite(bounds_.maxX)
        || !std::isfinite(bounds_.minY) || !std::isfinite(bounds_.maxY)
        || bounds_.minX > bounds_.maxX || bounds_.minY > bounds_.maxY) {
        throw std::invalid_argument("PlayerController requires finite speed and ordered finite bounds");
    }
    transform_.localPosition = {
        static_cast<float>((static_cast<double>(bounds_.minX) + bounds_.maxX) * 0.5),
        static_cast<float>((static_cast<double>(bounds_.minY) + bounds_.maxY) * 0.5),
        0.0f,
    };
    ClampToBounds();
}

void PlayerController::Update(const MovementInput& input, float deltaSeconds) {
    if (deltaSeconds <= 0.0f || !std::isfinite(deltaSeconds)) {
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

    // Multiplication/addition in double avoids overflow before the finite bounds clamp.
    const double distance = static_cast<double>(movementSpeed_) * deltaSeconds;
    transform_.localPosition.x = static_cast<float>(std::clamp(
        static_cast<double>(transform_.localPosition.x) + horizontal * distance,
        static_cast<double>(bounds_.minX), static_cast<double>(bounds_.maxX)));
    transform_.localPosition.y = static_cast<float>(std::clamp(
        static_cast<double>(transform_.localPosition.y) + vertical * distance,
        static_cast<double>(bounds_.minY), static_cast<double>(bounds_.maxY)));
}

void PlayerController::SetPosition(const Math::Vec3& position) {
    if (!Math::IsFinite(position)) return;
    transform_.localPosition = position;
    ClampToBounds();
}

bool PlayerController::IsWithinBounds() const {
    const Math::Vec3 position = transform_.localPosition;
    return Math::IsFinite(position) && position.x >= bounds_.minX && position.x <= bounds_.maxX
        && position.y >= bounds_.minY && position.y <= bounds_.maxY;
}

void PlayerController::ClampToBounds() {
    transform_.localPosition.x = std::clamp(transform_.localPosition.x, bounds_.minX, bounds_.maxX);
    transform_.localPosition.y = std::clamp(transform_.localPosition.y, bounds_.minY, bounds_.maxY);
}

} // namespace Astral::Scene
