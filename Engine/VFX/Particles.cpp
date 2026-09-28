#include "Engine/VFX/Particles.h"

#include <algorithm>
#include <cmath>

namespace Astral::VFX {

using namespace Math;

ParticleEmitter::ParticleEmitter(const EmitterSettings& settings, Vec3 at) : position(at), settings_(settings) {
    settings_.maxParticles = std::clamp(settings_.maxParticles, 1, 100000);
    settings_.lifetimeMin = std::max(0.01f, settings_.lifetimeMin);
    settings_.lifetimeMax = std::max(settings_.lifetimeMin, settings_.lifetimeMax);
    if (settings_.colors.empty()) settings_.colors.push_back({0.0f, {1, 1, 1, 1}});
    std::sort(settings_.colors.begin(), settings_.colors.end(), [](const ColorKey& a, const ColorKey& b) { return a.time < b.time; });
    random_.Seed(settings_.seed);
    particles_.reserve(static_cast<std::size_t>(std::min(settings_.maxParticles, 4096)));
    if (settings_.burst > 0) Burst(settings_.burst);
    if (settings_.rate <= 0.0f) emitting_ = false; // pure burst emitters finish when particles die
}

void ParticleEmitter::Spawn() {
    if (static_cast<int>(particles_.size()) >= settings_.maxParticles) return;
    Particle p;
    // Uniform direction inside a cone around settings.direction.
    const Vec3 axis = Normalize(settings_.direction, {0, 1, 0});
    Vec3 t, b;
    OrthonormalBasis(axis, t, b);
    const float cosMax = std::cos(Radians(Clamp(settings_.spreadDegrees, 0.0f, 180.0f)));
    const float cosTheta = Lerp(1.0f, cosMax, random_.NextFloat());
    const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
    const float phi = random_.NextFloat() * kTwoPi;
    const Vec3 dir = axis * cosTheta + (t * std::cos(phi) + b * std::sin(phi)) * sinTheta;
    p.velocity = dir * random_.Range(settings_.speedMin, settings_.speedMax) + velocityInherit;
    Vec3 offset{};
    if (settings_.spawnRadius > 0.0f) {
        // Rejection sampling inside the unit sphere (bounded attempts).
        for (int attempt = 0; attempt < 8; ++attempt) {
            const Vec3 r{random_.Range(-1, 1), random_.Range(-1, 1), random_.Range(-1, 1)};
            if (LengthSquared(r) <= 1.0f) {
                offset = r * settings_.spawnRadius;
                break;
            }
        }
    }
    offset += Vec3{random_.Range(-1, 1) * settings_.spawnExtents.x, random_.Range(-1, 1) * settings_.spawnExtents.y,
        random_.Range(-1, 1) * settings_.spawnExtents.z};
    p.position = position + offset;
    p.age = 0.0f;
    p.lifetime = random_.Range(settings_.lifetimeMin, settings_.lifetimeMax);
    p.rotation = random_.Range(0.0f, kTwoPi);
    particles_.push_back(p);
}

void ParticleEmitter::Burst(int count) {
    count = std::clamp(count, 0, settings_.maxParticles);
    for (int i = 0; i < count; ++i) Spawn();
}

void ParticleEmitter::Update(float dt) {
    if (!(dt > 0.0f) || !std::isfinite(dt)) return;
    elapsed_ += dt;
    if (emitting_ && settings_.duration >= 0.0f && elapsed_ >= settings_.duration) emitting_ = false;
    if (emitting_ && settings_.rate > 0.0f) {
        spawnAccumulator_ += settings_.rate * dt;
        const int spawns = static_cast<int>(std::min(spawnAccumulator_, static_cast<float>(settings_.maxParticles)));
        spawnAccumulator_ -= static_cast<float>(spawns);
        for (int i = 0; i < spawns; ++i) Spawn();
    }
    const float damping = 1.0f / (1.0f + settings_.drag * dt);
    for (Particle& p : particles_) {
        p.age += dt;
        Vec3 acceleration = settings_.gravity;
        if (settings_.swirl != 0.0f) {
            const Vec3 radial = Horizontal(p.position - position);
            acceleration += Normalize(Cross({0, 1, 0}, radial), {}) * settings_.swirl;
        }
        p.velocity = (p.velocity + acceleration * dt) * damping;
        p.position += p.velocity * dt;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                         [](const Particle& p) { return p.age >= p.lifetime; }),
        particles_.end());
}

Vec4 ParticleEmitter::ColorAt(float t) const {
    const auto& keys = settings_.colors;
    if (t <= keys.front().time) return keys.front().color;
    if (t >= keys.back().time) return keys.back().color;
    for (std::size_t i = 1; i < keys.size(); ++i) {
        if (t <= keys[i].time) {
            const float span = keys[i].time - keys[i - 1].time;
            return Lerp(keys[i - 1].color, keys[i].color, span > 0.0f ? (t - keys[i - 1].time) / span : 1.0f);
        }
    }
    return keys.back().color;
}

void ParticleEmitter::EmitBillboards(std::vector<Graphics::Billboard>& out) const {
    for (const Particle& p : particles_) {
        const float t = Saturate(p.age / p.lifetime);
        Graphics::Billboard b;
        b.position = p.position;
        b.size = Lerp(settings_.sizeStart, settings_.sizeEnd, t);
        b.rotation = p.rotation;
        b.color = ColorAt(t);
        b.blend = settings_.blend;
        b.stretch = settings_.stretch;
        b.velocity = p.velocity;
        if (b.size > 0.0f && b.color.w > 0.0f) out.push_back(b);
    }
}

EmitterId ParticleWorld::Spawn(const EmitterSettings& settings, Vec3 position) {
    return emitters_.Emplace(settings, position);
}

void ParticleWorld::Stop(EmitterId id) {
    if (ParticleEmitter* e = emitters_.Get(id)) e->Stop();
}

void ParticleWorld::Update(float dt) {
    std::vector<EmitterId> finished;
    emitters_.ForEach([&](Core::Handle id, ParticleEmitter& e) {
        e.Update(dt);
        if (e.Finished()) finished.push_back(id);
    });
    for (EmitterId id : finished) emitters_.Remove(id);
}

void ParticleWorld::Collect(std::vector<Graphics::Billboard>& out) const {
    emitters_.ForEach([&](Core::Handle, ParticleEmitter& e) { e.EmitBillboards(out); });
}

std::size_t ParticleWorld::ParticleCount() const {
    std::size_t count = 0;
    emitters_.ForEach([&](Core::Handle, ParticleEmitter& e) { count += e.Alive(); });
    return count;
}

void RibbonTrail::AddSample(Vec3 base, Vec3 tip, float time) {
    if (!IsFinite(base) || !IsFinite(tip) || !std::isfinite(time)) return;
    if (!samples_.empty() && DistanceSquared(samples_.back().tip, tip) < 1.0e-6f) {
        samples_.back().time = time;
        return;
    }
    samples_.push_back({base, tip, time});
    if (samples_.size() > 256) samples_.erase(samples_.begin());
}

void RibbonTrail::Prune(float time) {
    samples_.erase(std::remove_if(samples_.begin(), samples_.end(),
                       [&](const Sample& s) { return time - s.time > lifetime_; }),
        samples_.end());
}

void RibbonTrail::BuildMesh(float time, Vec4 headColor, Vec4 tailColor, Graphics::MeshData& out) const {
    out.vertices.clear();
    out.indices.clear();
    out.skin.clear();
    if (samples_.size() < 2) {
        out.ComputeBounds();
        return;
    }
    for (std::size_t i = 0; i < samples_.size(); ++i) {
        const Sample& s = samples_[i];
        const float age = Saturate((time - s.time) / std::max(lifetime_, 1.0e-4f));
        Vec4 color = Lerp(headColor, tailColor, age);
        color.w *= (1.0f - age) * (1.0f - age);
        const float v = static_cast<float>(i) / static_cast<float>(samples_.size() - 1);
        Vec3 normal = Cross(s.tip - s.base, i + 1 < samples_.size() ? samples_[i + 1].tip - s.tip : s.tip - samples_[i - 1].tip);
        normal = Normalize(normal, {0, 1, 0});
        Graphics::Vertex base{s.base, normal, {0.0f, v}, color};
        Vec4 tipColor = color;
        tipColor.w = std::min(1.0f, color.w * 1.4f);
        Graphics::Vertex tip{s.tip, normal, {1.0f, v}, tipColor};
        base.color.w *= 0.15f; // fade toward the hilt for a crescent look
        out.vertices.push_back(base);
        out.vertices.push_back(tip);
    }
    for (std::uint32_t i = 0; i + 1 < samples_.size(); ++i) {
        const std::uint32_t a = i * 2, b = a + 1, c = a + 2, d = a + 3;
        out.indices.insert(out.indices.end(), {a, b, d, a, d, c});
    }
    out.ComputeBounds();
}

namespace Presets {

EmitterSettings SlashSparks(Vec3 direction) {
    EmitterSettings s;
    s.burst = 36;
    s.direction = direction;
    s.spreadDegrees = 55.0f;
    s.speedMin = 3.0f;
    s.speedMax = 9.0f;
    s.lifetimeMin = 0.15f;
    s.lifetimeMax = 0.4f;
    s.gravity = {0, -9.0f, 0};
    s.drag = 2.0f;
    s.sizeStart = 0.035f;
    s.sizeEnd = 0.01f;
    s.stretch = 4.0f;
    s.colors = {{0.0f, {4.0f, 3.2f, 6.0f, 1}}, {0.5f, {1.8f, 0.8f, 3.0f, 0.8f}}, {1.0f, {0.4f, 0.1f, 0.8f, 0}}};
    return s;
}

EmitterSettings HitBurst() {
    EmitterSettings s;
    s.burst = 24;
    s.spreadDegrees = 180.0f;
    s.speedMin = 1.0f;
    s.speedMax = 3.5f;
    s.lifetimeMin = 0.12f;
    s.lifetimeMax = 0.3f;
    s.drag = 5.0f;
    s.sizeStart = 0.18f;
    s.sizeEnd = 0.02f;
    s.colors = {{0.0f, {6.0f, 5.0f, 4.0f, 1}}, {1.0f, {2.0f, 0.5f, 0.2f, 0}}};
    return s;
}

EmitterSettings ShadowSmoke() {
    EmitterSettings s;
    s.burst = 20;
    s.spreadDegrees = 70.0f;
    s.speedMin = 0.3f;
    s.speedMax = 1.2f;
    s.spawnRadius = 0.35f;
    s.lifetimeMin = 0.5f;
    s.lifetimeMax = 0.9f;
    s.gravity = {0, 0.6f, 0};
    s.drag = 1.5f;
    s.sizeStart = 0.25f;
    s.sizeEnd = 0.6f;
    s.blend = Graphics::BlendMode::AlphaBlend;
    s.colors = {{0.0f, {0.12f, 0.06f, 0.2f, 0.7f}}, {1.0f, {0.05f, 0.02f, 0.1f, 0.0f}}};
    return s;
}

EmitterSettings ManaMotes(float radius) {
    EmitterSettings s;
    s.rate = 14.0f;
    s.spreadDegrees = 25.0f;
    s.speedMin = 0.2f;
    s.speedMax = 0.6f;
    s.spawnExtents = {radius, 0.2f, radius};
    s.lifetimeMin = 2.5f;
    s.lifetimeMax = 4.0f;
    s.swirl = 0.35f;
    s.drag = 0.3f;
    s.sizeStart = 0.07f;
    s.sizeEnd = 0.0f;
    s.maxParticles = 128;
    s.colors = {{0.0f, {0.3f, 1.4f, 2.0f, 0}}, {0.2f, {0.4f, 1.6f, 2.4f, 1}}, {1.0f, {0.8f, 0.6f, 2.4f, 0}}};
    return s;
}

EmitterSettings FocusAura() {
    EmitterSettings s;
    s.rate = 40.0f;
    s.spreadDegrees = 15.0f;
    s.speedMin = 0.6f;
    s.speedMax = 1.6f;
    s.spawnExtents = {0.5f, 0.0f, 0.5f};
    s.lifetimeMin = 0.6f;
    s.lifetimeMax = 1.2f;
    s.swirl = 1.5f;
    s.sizeStart = 0.06f;
    s.sizeEnd = 0.0f;
    s.stretch = 2.0f;
    s.maxParticles = 160;
    s.colors = {{0.0f, {1.2f, 0.6f, 3.0f, 0}}, {0.3f, {1.4f, 0.8f, 3.4f, 1}}, {1.0f, {0.6f, 0.3f, 2.0f, 0}}};
    return s;
}

EmitterSettings GuardShimmer() {
    EmitterSettings s;
    s.rate = 30.0f;
    s.spreadDegrees = 180.0f;
    s.speedMin = 0.05f;
    s.speedMax = 0.2f;
    s.spawnRadius = 0.9f;
    s.lifetimeMin = 0.3f;
    s.lifetimeMax = 0.6f;
    s.sizeStart = 0.05f;
    s.sizeEnd = 0.0f;
    s.maxParticles = 64;
    s.colors = {{0.0f, {3.0f, 2.4f, 0.6f, 1}}, {1.0f, {1.5f, 1.0f, 0.2f, 0}}};
    return s;
}

EmitterSettings DebrisDust() {
    EmitterSettings s;
    s.burst = 40;
    s.spreadDegrees = 80.0f;
    s.speedMin = 0.5f;
    s.speedMax = 2.5f;
    s.spawnRadius = 0.6f;
    s.lifetimeMin = 0.8f;
    s.lifetimeMax = 1.6f;
    s.gravity = {0, -1.0f, 0};
    s.drag = 1.2f;
    s.sizeStart = 0.2f;
    s.sizeEnd = 0.7f;
    s.blend = Graphics::BlendMode::AlphaBlend;
    s.colors = {{0.0f, {0.75f, 0.72f, 0.68f, 0.6f}}, {1.0f, {0.7f, 0.68f, 0.64f, 0.0f}}};
    return s;
}

EmitterSettings DashStreaks(Vec3 direction) {
    EmitterSettings s;
    s.burst = 30;
    s.direction = -direction;
    s.spreadDegrees = 12.0f;
    s.speedMin = 6.0f;
    s.speedMax = 14.0f;
    s.spawnExtents = {0.3f, 0.8f, 0.3f};
    s.lifetimeMin = 0.08f;
    s.lifetimeMax = 0.2f;
    s.sizeStart = 0.03f;
    s.sizeEnd = 0.01f;
    s.stretch = 8.0f;
    s.colors = {{0.0f, {2.0f, 1.2f, 4.0f, 1}}, {1.0f, {0.5f, 0.2f, 1.5f, 0}}};
    return s;
}

} // namespace Presets

} // namespace Astral::VFX
