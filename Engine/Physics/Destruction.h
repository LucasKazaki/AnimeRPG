#pragma once

// Destructible props: an intact static collider that, once its health is spent,
// is replaced by deterministic fractured debris (jittered-grid chunks) launched
// away from the impact, with a reset path for encounters and retries. This is
// the engine foundation for the game's destruction milestone (breakable props,
// mana crystals, boss pillars).

#include "Engine/Physics/PhysicsWorld.h"

#include <vector>

namespace Astral::Physics {

struct FracturePiece {
    Vec3 localCenter{};
    Vec3 halfExtents{};
};

// Splits a box of the given half extents into nx*ny*nz chunks whose interior
// cut planes are jittered (0 = regular grid, <0.5 keeps pieces non-degenerate).
std::vector<FracturePiece> FractureBox(Vec3 halfExtents, int nx, int ny, int nz, std::uint32_t seed, float jitter = 0.3f);

struct DestructibleSettings {
    float health{100.0f};
    int piecesX{3}, piecesY{3}, piecesZ{2};
    std::uint32_t seed{1};
    float density{400.0f};     // kg per cubic metre
    float breakImpulse{4.0f};  // outward speed added to debris (m/s)
    float debrisLifetime{8.0f};
    std::uint32_t layer{1u};
};

class Destructible {
public:
    Destructible(PhysicsWorld& world, const Pose& pose, Vec3 halfExtents, const DestructibleSettings& settings);
    ~Destructible();
    Destructible(const Destructible&) = delete;
    Destructible& operator=(const Destructible&) = delete;

    // Returns true when this hit broke the prop.
    bool ApplyDamage(float amount, Vec3 hitPoint, Vec3 hitDirection);
    void Update(float dt); // expires debris after its lifetime
    void Reset();          // removes debris, restores the intact collider and health

    bool Broken() const { return broken_; }
    float Health() const { return health_; }
    BodyId IntactBody() const { return intact_; }
    const std::vector<BodyId>& Debris() const { return debris_; }
    const std::vector<FracturePiece>& Pieces() const { return pieces_; }
    const Pose& GetPose() const { return pose_; }
    Vec3 HalfExtents() const { return halfExtents_; }

private:
    void Clear();

    PhysicsWorld& world_;
    Pose pose_;
    Vec3 halfExtents_;
    DestructibleSettings settings_;
    std::vector<FracturePiece> pieces_;
    BodyId intact_{};
    std::vector<BodyId> debris_;
    float health_{};
    float debrisAge_{};
    bool broken_{};
};

} // namespace Astral::Physics
