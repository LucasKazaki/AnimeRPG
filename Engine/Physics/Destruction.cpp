#include "Engine/Physics/Destruction.h"

#include "Engine/Core/Random.h"

#include <algorithm>
#include <cmath>

namespace Astral::Physics {

using namespace Math;

std::vector<FracturePiece> FractureBox(Vec3 half, int nx, int ny, int nz, std::uint32_t seed, float jitter) {
    std::vector<FracturePiece> pieces;
    if (!IsFinite(half) || half.x <= 0.0f || half.y <= 0.0f || half.z <= 0.0f) return pieces;
    const int counts[3] = {std::clamp(nx, 1, 16), std::clamp(ny, 1, 16), std::clamp(nz, 1, 16)};
    jitter = Clamp(jitter, 0.0f, 0.45f);
    Core::Random random(seed);
    std::vector<float> cuts[3];
    const float extents[3] = {half.x, half.y, half.z};
    for (int axis = 0; axis < 3; ++axis) {
        const int n = counts[axis];
        const float size = 2.0f * extents[axis];
        const float cell = size / static_cast<float>(n);
        cuts[axis].push_back(-extents[axis]);
        for (int i = 1; i < n; ++i) {
            const float offset = (random.NextFloat() * 2.0f - 1.0f) * jitter * cell;
            cuts[axis].push_back(-extents[axis] + cell * static_cast<float>(i) + offset);
        }
        cuts[axis].push_back(extents[axis]);
    }
    for (int z = 0; z < counts[2]; ++z) {
        for (int y = 0; y < counts[1]; ++y) {
            for (int x = 0; x < counts[0]; ++x) {
                const Vec3 minimum{cuts[0][static_cast<std::size_t>(x)], cuts[1][static_cast<std::size_t>(y)], cuts[2][static_cast<std::size_t>(z)]};
                const Vec3 maximum{cuts[0][static_cast<std::size_t>(x + 1)], cuts[1][static_cast<std::size_t>(y + 1)], cuts[2][static_cast<std::size_t>(z + 1)]};
                pieces.push_back({(minimum + maximum) * 0.5f, (maximum - minimum) * 0.5f});
            }
        }
    }
    return pieces;
}

Destructible::Destructible(PhysicsWorld& world, const Pose& pose, Vec3 halfExtents, const DestructibleSettings& settings)
    : world_(world), pose_(pose), halfExtents_(halfExtents), settings_(settings) {
    pieces_ = FractureBox(halfExtents, settings.piecesX, settings.piecesY, settings.piecesZ, settings.seed);
    Reset();
}

Destructible::~Destructible() { Clear(); }

void Destructible::Clear() {
    if (!intact_.IsNull()) world_.DestroyBody(intact_);
    intact_ = {};
    for (BodyId id : debris_) world_.DestroyBody(id);
    debris_.clear();
}

void Destructible::Reset() {
    Clear();
    BodyDesc desc;
    desc.type = BodyType::Static;
    desc.shape = Shape::Box(halfExtents_);
    desc.position = pose_.position;
    desc.rotation = pose_.rotation;
    desc.layer = settings_.layer;
    intact_ = world_.CreateBody(desc);
    health_ = std::max(0.0f, settings_.health);
    broken_ = false;
    debrisAge_ = 0.0f;
}

bool Destructible::ApplyDamage(float amount, Vec3 hitPoint, Vec3 hitDirection) {
    if (broken_ || !std::isfinite(amount) || amount <= 0.0f) return false;
    health_ = std::max(0.0f, health_ - amount);
    if (health_ > 0.0f) return false;
    broken_ = true;
    if (!intact_.IsNull()) world_.DestroyBody(intact_);
    intact_ = {};
    const Vec3 push = Normalize(hitDirection, {0, 0, 1});
    if (!IsFinite(hitPoint)) hitPoint = pose_.position;
    for (const FracturePiece& piece : pieces_) {
        BodyDesc desc;
        desc.type = BodyType::Dynamic;
        desc.shape = Shape::Box(piece.halfExtents * 0.98f);
        desc.position = pose_.TransformPoint(piece.localCenter);
        desc.rotation = pose_.rotation;
        desc.mass = std::max(0.05f, settings_.density * 8.0f * piece.halfExtents.x * piece.halfExtents.y * piece.halfExtents.z);
        desc.friction = 0.7f;
        desc.restitution = 0.1f;
        desc.layer = settings_.layer;
        // Radial burst from the impact plus the hit direction.
        const Vec3 away = Normalize(desc.position - hitPoint, push);
        desc.linearVelocity = (away * 0.6f + push * 0.4f) * settings_.breakImpulse + Vec3{0.0f, settings_.breakImpulse * 0.35f, 0.0f};
        desc.angularVelocity = Cross(push, away) * 3.0f;
        const BodyId id = world_.CreateBody(desc);
        if (!id.IsNull()) debris_.push_back(id);
    }
    debrisAge_ = 0.0f;
    return true;
}

void Destructible::Update(float dt) {
    if (!broken_ || debris_.empty() || !(dt > 0.0f)) return;
    debrisAge_ += dt;
    if (debrisAge_ >= settings_.debrisLifetime) {
        for (BodyId id : debris_) world_.DestroyBody(id);
        debris_.clear();
    }
}

} // namespace Astral::Physics
