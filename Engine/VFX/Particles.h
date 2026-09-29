#pragma once

// CPU particle systems (the role of UE Niagara / Unity VFX for this engine):
// rate and burst emission, cone/sphere spawn shapes, gravity, drag, curl-like
// swirl, size and colour over life, velocity-stretched sparks and deterministic
// seeding. Output is Graphics::Billboard, so any render backend draws them.
// Also: weapon ribbon trails and a preset library for the game's effects.

#include "Engine/Core/Random.h"
#include "Engine/Core/SlotMap.h"
#include "Engine/Graphics/RenderScene.h"

#include <vector>

namespace Astral::VFX {

using Math::Vec3;
using Math::Vec4;

struct ColorKey {
    float time{};        // normalised age 0..1
    Vec4 color{1, 1, 1, 1};
};

struct EmitterSettings {
    float rate{0.0f};             // particles per second while emitting
    int burst{0};                 // spawned once on start
    float duration{-1.0f};        // emission time; < 0 = forever
    float lifetimeMin{0.5f}, lifetimeMax{1.0f};
    Vec3 direction{0, 1, 0};
    float spreadDegrees{30.0f};   // cone half-angle around direction
    float speedMin{1.0f}, speedMax{2.0f};
    float spawnRadius{0.0f};      // random point in a sphere
    Vec3 spawnExtents{};          // additionally random point in a box
    Vec3 gravity{};
    float drag{0.0f};
    float swirl{0.0f};            // tangential acceleration around +Y (mana motes)
    float sizeStart{0.1f}, sizeEnd{0.0f};
    float stretch{1.0f};          // velocity stretch for sparks
    std::vector<ColorKey> colors{{0.0f, {1, 1, 1, 1}}, {1.0f, {1, 1, 1, 0}}};
    Graphics::BlendMode blend{Graphics::BlendMode::Additive};
    int maxParticles{512};
    std::uint32_t seed{1};
};

class ParticleEmitter {
public:
    ParticleEmitter() = default;
    ParticleEmitter(const EmitterSettings& settings, Vec3 position);

    void Update(float dt);
    void Burst(int count);
    void Stop() { emitting_ = false; }
    bool Finished() const { return !emitting_ && particles_.empty(); }
    void EmitBillboards(std::vector<Graphics::Billboard>& out) const;
    std::size_t Alive() const { return particles_.size(); }

    Vec3 position{};
    Vec3 velocityInherit{}; // added to newly spawned particles

private:
    struct Particle {
        Vec3 position, velocity;
        float age, lifetime;
        float rotation;
    };
    void Spawn();
    Vec4 ColorAt(float t) const;

    EmitterSettings settings_;
    std::vector<Particle> particles_;
    Core::Random random_;
    float spawnAccumulator_{};
    float elapsed_{};
    bool emitting_{true};
};

using EmitterId = Core::Handle;

class ParticleWorld {
public:
    EmitterId Spawn(const EmitterSettings& settings, Vec3 position);
    ParticleEmitter* Get(EmitterId id) { return emitters_.Get(id); }
    void Stop(EmitterId id);
    // Advances all emitters and removes finished ones.
    void Update(float dt);
    void Collect(std::vector<Graphics::Billboard>& out) const;
    std::size_t EmitterCount() const { return emitters_.Size(); }
    std::size_t ParticleCount() const;

private:
    mutable Core::SlotMap<ParticleEmitter> emitters_;
};

// Ribbon trail from (base, tip) samples, e.g. a blade edge during a swing.
class RibbonTrail {
public:
    explicit RibbonTrail(float lifetime = 0.18f) : lifetime_(lifetime) {}
    void AddSample(Vec3 base, Vec3 tip, float time);
    void Prune(float time);
    void Clear() { samples_.clear(); }
    bool Empty() const { return samples_.size() < 2; }
    // Double-sided strip; vertex alpha fades with age (additive material).
    void BuildMesh(float time, Vec4 headColor, Vec4 tailColor, Graphics::MeshData& out) const;

private:
    struct Sample {
        Vec3 base, tip;
        float time;
    };
    std::vector<Sample> samples_;
    float lifetime_;
};

// Effect presets for the Shadowblade kit and the supernatural Mall.
namespace Presets {
EmitterSettings SlashSparks(Vec3 direction);
EmitterSettings HitBurst();
EmitterSettings ShadowSmoke();
EmitterSettings ManaMotes(float radius);
EmitterSettings FocusAura();
EmitterSettings GuardShimmer();
EmitterSettings DebrisDust();
EmitterSettings DashStreaks(Vec3 direction);
} // namespace Presets

} // namespace Astral::VFX
