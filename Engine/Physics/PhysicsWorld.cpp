#include "Engine/Physics/PhysicsWorld.h"

#include "Engine/Core/Profiler.h"

#include <algorithm>
#include <cmath>

namespace Astral::Physics {

using namespace Math;

namespace {
Vec3 ClampLength(Vec3 v, float maximum) {
    const float length = Length(v);
    return length > maximum && length > 0.0f ? v * (maximum / length) : v;
}
} // namespace

std::uint64_t PhysicsWorld::PairKey(BodyId a, BodyId b) {
    const std::uint32_t lo = std::min(a.index, b.index), hi = std::max(a.index, b.index);
    return (static_cast<std::uint64_t>(lo) << 32) | hi;
}

bool PhysicsWorld::ShouldCollide(const Body& a, const Body& b) const {
    if (!(a.layer & b.mask) || !(b.layer & a.mask)) return false;
    const bool aDynamic = a.type == BodyType::Dynamic, bDynamic = b.type == BodyType::Dynamic;
    if (!aDynamic && !bDynamic && !a.isTrigger && !b.isTrigger) return false;
    if (!a.awake && !b.awake) return false;
    return true;
}

void PhysicsWorld::UpdateInertia(Body& body) const {
    const Mat3 r = Mat3::FromQuat(body.pose.rotation);
    body.inverseInertiaWorld = r * Mat3::Diagonal(body.inverseInertiaLocal) * r.Transposed();
}

BodyId PhysicsWorld::CreateBody(const BodyDesc& desc) {
    if (!desc.shape.IsValid() || !IsFinite(desc.position) || !IsFinite(desc.linearVelocity)
        || !IsFinite(desc.angularVelocity)) {
        return {};
    }
    if (desc.type == BodyType::Dynamic && !(desc.mass > 0.0f && std::isfinite(desc.mass))) return {};
    Body body;
    body.type = desc.type;
    body.shape = desc.shape;
    body.pose = {desc.position, Normalize(desc.rotation)};
    body.linearVelocity = desc.type == BodyType::Static ? Vec3{} : desc.linearVelocity;
    body.angularVelocity = desc.type == BodyType::Static ? Vec3{} : desc.angularVelocity;
    body.friction = std::max(0.0f, desc.friction);
    body.restitution = Clamp(desc.restitution, 0.0f, 1.0f);
    body.linearDamping = std::max(0.0f, desc.linearDamping);
    body.angularDamping = std::max(0.0f, desc.angularDamping);
    body.gravityScale = desc.gravityScale;
    body.layer = desc.layer;
    body.mask = desc.mask;
    body.isTrigger = desc.isTrigger;
    body.lockRotation = desc.lockRotation;
    body.userData = desc.userData;
    if (desc.type == BodyType::Dynamic) {
        body.inverseMass = 1.0f / desc.mass;
        if (!desc.lockRotation) {
            const Vec3 inertia = desc.shape.InertiaDiagonal(desc.mass);
            body.inverseInertiaLocal = {1.0f / inertia.x, 1.0f / inertia.y, 1.0f / inertia.z};
        }
    }
    const BodyId id = bodies_.Emplace(body);
    Body* stored = bodies_.Get(id);
    stored->id = id;
    UpdateInertia(*stored);
    stored->proxy = tree_.CreateProxy(stored->shape.WorldBounds(stored->pose), id.index);
    if (proxyToBody_.size() <= static_cast<std::size_t>(stored->proxy)) proxyToBody_.resize(static_cast<std::size_t>(stored->proxy) + 1);
    proxyToBody_[static_cast<std::size_t>(stored->proxy)] = id;
    return id;
}

bool PhysicsWorld::DestroyBody(BodyId id) {
    Body* body = bodies_.Get(id);
    if (!body) return false;
    tree_.DestroyProxy(body->proxy);
    proxyToBody_[static_cast<std::size_t>(body->proxy)] = {};
    // Emit End events for anything it was touching.
    for (auto it = touchingBodies_.begin(); it != touchingBodies_.end();) {
        if (it->second.first == id || it->second.second == id) {
            events_.push_back({ContactEventType::End, it->second.first, it->second.second, false, {}, {}});
            touching_.erase(it->first);
            contactCache_.erase(it->first);
            it = touchingBodies_.erase(it);
        } else {
            ++it;
        }
    }
    return bodies_.Remove(id);
}

void PhysicsWorld::SetPose(BodyId id, const Pose& pose) {
    Body* body = bodies_.Get(id);
    if (!body || !IsFinite(pose.position)) return;
    const Vec3 displacement = pose.position - body->pose.position;
    body->pose = {pose.position, Normalize(pose.rotation)};
    UpdateInertia(*body);
    tree_.MoveProxy(body->proxy, body->shape.WorldBounds(body->pose), displacement);
    body->awake = true;
    body->sleepTimer = 0.0f;
}

void PhysicsWorld::SetLinearVelocity(BodyId id, Vec3 velocity) {
    Body* body = bodies_.Get(id);
    if (!body || body->type == BodyType::Static || !IsFinite(velocity)) return;
    body->linearVelocity = velocity;
    body->awake = true;
    body->sleepTimer = 0.0f;
}

void PhysicsWorld::ApplyImpulse(BodyId id, Vec3 impulse, Vec3 worldPoint) {
    Body* body = bodies_.Get(id);
    if (!body || body->type != BodyType::Dynamic || !IsFinite(impulse)) return;
    body->linearVelocity += impulse * body->inverseMass;
    body->angularVelocity += body->inverseInertiaWorld * Cross(worldPoint - body->pose.position, impulse);
    body->awake = true;
    body->sleepTimer = 0.0f;
}

void PhysicsWorld::ApplyForce(BodyId id, Vec3 force) {
    Body* body = bodies_.Get(id);
    if (!body || body->type != BodyType::Dynamic || !IsFinite(force)) return;
    body->force += force;
    body->awake = true;
}

void PhysicsWorld::MoveKinematic(BodyId id, const Pose& target, float dt) {
    Body* body = bodies_.Get(id);
    if (!body || body->type != BodyType::Kinematic || !(dt > 0.0f)) return;
    body->linearVelocity = (target.position - body->pose.position) / dt;
    const Quat delta = Normalize(target.rotation * Conjugate(body->pose.rotation));
    const float w = Clamp(delta.w, -1.0f, 1.0f);
    const float angle = 2.0f * std::acos(std::fabs(w));
    const Vec3 axis = Normalize(Vec3{delta.x, delta.y, delta.z} * (w < 0.0f ? -1.0f : 1.0f), {0, 1, 0});
    body->angularVelocity = angle > 1.0e-6f ? axis * (angle / dt) : Vec3{};
    body->awake = true;
}

void PhysicsWorld::WakeUp(BodyId id) {
    if (Body* body = bodies_.Get(id)) {
        body->awake = true;
        body->sleepTimer = 0.0f;
    }
}

void PhysicsWorld::SolveContact(ContactConstraint& c) const {
    Body& a = *c.a;
    Body& b = *c.b;
    for (int i = 0; i < c.count; ++i) {
        auto& p = c.points[i];
        // Friction (clamped by the current normal impulse).
        for (int t = 0; t < 2; ++t) {
            const Vec3 dv = b.linearVelocity + Cross(b.angularVelocity, p.rb) - a.linearVelocity - Cross(a.angularVelocity, p.ra);
            const float vt = Dot(dv, c.tangent[t]);
            float lambda = -vt * p.tangentMass[t];
            const float maxFriction = c.friction * p.normalImpulse;
            const float old = p.tangentImpulse[t];
            p.tangentImpulse[t] = Clamp(old + lambda, -maxFriction, maxFriction);
            lambda = p.tangentImpulse[t] - old;
            const Vec3 impulse = c.tangent[t] * lambda;
            a.linearVelocity -= impulse * a.inverseMass;
            a.angularVelocity -= a.inverseInertiaWorld * Cross(p.ra, impulse);
            b.linearVelocity += impulse * b.inverseMass;
            b.angularVelocity += b.inverseInertiaWorld * Cross(p.rb, impulse);
        }
        const Vec3 dv = b.linearVelocity + Cross(b.angularVelocity, p.rb) - a.linearVelocity - Cross(a.angularVelocity, p.ra);
        const float vn = Dot(dv, c.normal);
        float lambda = (p.bias - vn) * p.normalMass;
        const float old = p.normalImpulse;
        p.normalImpulse = std::max(0.0f, old + lambda);
        lambda = p.normalImpulse - old;
        const Vec3 impulse = c.normal * lambda;
        a.linearVelocity -= impulse * a.inverseMass;
        a.angularVelocity -= a.inverseInertiaWorld * Cross(p.ra, impulse);
        b.linearVelocity += impulse * b.inverseMass;
        b.angularVelocity += b.inverseInertiaWorld * Cross(p.rb, impulse);
    }
}

void PhysicsWorld::Step(float dt) {
    ASTRAL_PROFILE_SCOPE("Physics.Step");
    events_.clear();
    if (!(dt > 0.0f) || !std::isfinite(dt)) return;
    dt = std::min(dt, 0.1f);

    // 1. Integrate forces into velocities.
    bodies_.ForEach([&](Core::Handle, Body& body) {
        if (body.type != BodyType::Dynamic || !body.awake) return;
        body.linearVelocity += (settings_.gravity * body.gravityScale + body.force * body.inverseMass) * dt;
        body.angularVelocity += body.inverseInertiaWorld * body.torque * dt;
        body.linearVelocity *= 1.0f / (1.0f + dt * body.linearDamping);
        body.angularVelocity *= 1.0f / (1.0f + dt * body.angularDamping);
        body.force = {};
        body.torque = {};
    });

    // 2. Broadphase pairs (sorted, unique).
    std::vector<std::pair<BodyId, BodyId>> pairs;
    bodies_.ForEach([&](Core::Handle id, Body& body) {
        if (body.type == BodyType::Static) return;
        if (!body.awake && body.type == BodyType::Dynamic) return;
        tree_.MoveProxy(body.proxy, body.shape.WorldBounds(body.pose), body.linearVelocity * dt);
        tree_.Query(tree_.FatBounds(body.proxy), [&](int proxy) {
            const BodyId other = proxyToBody_[static_cast<std::size_t>(proxy)];
            if (other == id) return true;
            const Body* otherBody = bodies_.Get(other);
            if (!otherBody || !ShouldCollide(body, *otherBody)) return true;
            pairs.push_back(id.index < other.index ? std::make_pair(id, other) : std::make_pair(other, id));
            return true;
        });
    });
    std::sort(pairs.begin(), pairs.end(), [](const auto& x, const auto& y) {
        return x.first.index != y.first.index ? x.first.index < y.first.index : x.second.index < y.second.index;
    });
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
    stats_.broadphasePairs = pairs.size();

    // 3. Narrowphase and constraint preparation.
    constraints_.clear();
    std::set<std::uint64_t> nowTouching;
    std::map<std::uint64_t, std::pair<BodyId, BodyId>> nowTouchingBodies;
    std::map<std::uint64_t, std::vector<CachedPoint>> newCache;
    stats_.contactPoints = 0;
    for (const auto& pair : pairs) {
        Body* a = bodies_.Get(pair.first);
        Body* b = bodies_.Get(pair.second);
        if (!a || !b) continue;
        if (!a->shape.WorldBounds(a->pose).Overlaps(b->shape.WorldBounds(b->pose))) continue;
        Manifold manifold;
        if (!Collide(a->shape, a->pose, b->shape, b->pose, manifold) || manifold.count == 0) continue;
        const std::uint64_t key = PairKey(a->id, b->id);
        nowTouching.insert(key);
        nowTouchingBodies[key] = {a->id, b->id};
        if (!touching_.count(key)) {
            events_.push_back({ContactEventType::Begin, a->id, b->id, a->isTrigger || b->isTrigger,
                manifold.points[0].position, manifold.normal});
        }
        if (a->isTrigger || b->isTrigger) continue;
        // Wake a sleeping body touched by an awake one.
        if (a->type == BodyType::Dynamic && !a->awake && b->awake) {
            a->awake = true;
            a->sleepTimer = 0.0f;
        }
        if (b->type == BodyType::Dynamic && !b->awake && a->awake) {
            b->awake = true;
            b->sleepTimer = 0.0f;
        }
        ContactConstraint c;
        c.a = a;
        c.b = b;
        c.key = key;
        c.normal = manifold.normal;
        OrthonormalBasis(c.normal, c.tangent[0], c.tangent[1]);
        c.friction = std::sqrt(a->friction * b->friction);
        const float restitution = std::max(a->restitution, b->restitution);
        const auto cached = contactCache_.find(key);
        c.count = manifold.count;
        for (int i = 0; i < manifold.count; ++i) {
            auto& p = c.points[i];
            p.position = manifold.points[i].position;
            p.penetration = manifold.points[i].penetration;
            p.ra = p.position - a->pose.position;
            p.rb = p.position - b->pose.position;
            auto effectiveMass = [&](Vec3 axis) {
                const Vec3 raCross = Cross(p.ra, axis), rbCross = Cross(p.rb, axis);
                const float k = a->inverseMass + b->inverseMass + Dot(raCross, a->inverseInertiaWorld * raCross)
                    + Dot(rbCross, b->inverseInertiaWorld * rbCross);
                return k > 1.0e-12f ? 1.0f / k : 0.0f;
            };
            p.normalMass = effectiveMass(c.normal);
            p.tangentMass[0] = effectiveMass(c.tangent[0]);
            p.tangentMass[1] = effectiveMass(c.tangent[1]);
            const Vec3 dv = b->linearVelocity + Cross(b->angularVelocity, p.rb) - a->linearVelocity - Cross(a->angularVelocity, p.ra);
            const float vn = Dot(dv, c.normal);
            p.bias = settings_.baumgarte / dt * std::max(0.0f, p.penetration - settings_.penetrationSlop);
            if (vn < -settings_.restitutionThreshold) p.bias = std::max(p.bias, -restitution * vn);
            // Warm start from the closest cached point of this pair.
            if (cached != contactCache_.end()) {
                float best = 0.02f * 0.02f;
                for (const CachedPoint& old : cached->second) {
                    const float d = DistanceSquared(old.position, p.position);
                    if (d < best) {
                        best = d;
                        p.normalImpulse = old.normalImpulse;
                        p.tangentImpulse[0] = old.tangentImpulse[0];
                        p.tangentImpulse[1] = old.tangentImpulse[1];
                    }
                }
            }
        }
        constraints_.push_back(c);
        stats_.contactPoints += static_cast<std::size_t>(manifold.count);
    }
    for (const auto& entry : touchingBodies_) {
        if (nowTouching.count(entry.first)) continue;
        const Body* a = bodies_.Get(entry.second.first);
        const Body* b = bodies_.Get(entry.second.second);
        // Resting contacts between sleeping bodies (or sleeping vs static) persist
        // without End events, and keep their warm-start impulses for waking.
        const bool dormant = a && b && !(a->type != BodyType::Static && a->awake) && !(b->type != BodyType::Static && b->awake);
        if (dormant) {
            nowTouching.insert(entry.first);
            nowTouchingBodies[entry.first] = entry.second;
            const auto cached = contactCache_.find(entry.first);
            if (cached != contactCache_.end()) newCache[entry.first] = cached->second;
            continue;
        }
        const bool trigger = (a && a->isTrigger) || (b && b->isTrigger);
        events_.push_back({ContactEventType::End, entry.second.first, entry.second.second, trigger, {}, {}});
    }
    touching_.swap(nowTouching);
    touchingBodies_.swap(nowTouchingBodies);
    stats_.contactPairs = constraints_.size();

    // 4. Warm start, then iterate.
    for (ContactConstraint& c : constraints_) {
        for (int i = 0; i < c.count; ++i) {
            const auto& p = c.points[i];
            const Vec3 impulse = c.normal * p.normalImpulse + c.tangent[0] * p.tangentImpulse[0] + c.tangent[1] * p.tangentImpulse[1];
            c.a->linearVelocity -= impulse * c.a->inverseMass;
            c.a->angularVelocity -= c.a->inverseInertiaWorld * Cross(p.ra, impulse);
            c.b->linearVelocity += impulse * c.b->inverseMass;
            c.b->angularVelocity += c.b->inverseInertiaWorld * Cross(p.rb, impulse);
        }
    }
    for (int iteration = 0; iteration < settings_.velocityIterations; ++iteration) {
        for (ContactConstraint& c : constraints_) SolveContact(c);
    }
    for (const ContactConstraint& c : constraints_) {
        std::vector<CachedPoint>& cache = newCache[c.key];
        for (int i = 0; i < c.count; ++i) {
            const auto& p = c.points[i];
            cache.push_back({p.position, p.normalImpulse, {p.tangentImpulse[0], p.tangentImpulse[1]}});
        }
    }
    contactCache_.swap(newCache);

    // 5. Integrate positions and handle sleep.
    stats_.awakeBodies = 0;
    stats_.bodies = bodies_.Size();
    bodies_.ForEach([&](Core::Handle, Body& body) {
        if (body.type == BodyType::Static) return;
        if (body.type == BodyType::Dynamic && !body.awake) return;
        body.linearVelocity = ClampLength(body.linearVelocity, settings_.maxLinearSpeed);
        body.angularVelocity = ClampLength(body.angularVelocity, settings_.maxAngularSpeed);
        if (body.lockRotation) body.angularVelocity = {};
        body.pose.position += body.linearVelocity * dt;
        const Vec3 w = body.angularVelocity;
        const Quat spin{w.x, w.y, w.z, 0.0f};
        Quat q = body.pose.rotation;
        const Quat dq = spin * q;
        q = {q.x + 0.5f * dt * dq.x, q.y + 0.5f * dt * dq.y, q.z + 0.5f * dt * dq.z, q.w + 0.5f * dt * dq.w};
        body.pose.rotation = Normalize(q);
        UpdateInertia(body);
        if (body.type == BodyType::Dynamic && settings_.allowSleep) {
            if (LengthSquared(body.linearVelocity) < settings_.sleepLinearSpeed * settings_.sleepLinearSpeed
                && LengthSquared(body.angularVelocity) < settings_.sleepAngularSpeed * settings_.sleepAngularSpeed) {
                body.sleepTimer += dt;
                if (body.sleepTimer >= settings_.sleepTime) {
                    body.awake = false;
                    body.linearVelocity = {};
                    body.angularVelocity = {};
                }
            } else {
                body.sleepTimer = 0.0f;
            }
        }
        if (body.awake) ++stats_.awakeBodies;
    });
    stats_.treeHeight = tree_.Height();
}

bool PhysicsWorld::Raycast(const Ray& ray, float maxDistance, RaycastHit& hit, std::uint32_t mask, BodyId ignore,
    bool includeTriggers) const {
    if (!IsFinite(ray.origin) || !IsFinite(ray.direction) || !(maxDistance > 0.0f)) return false;
    const Ray unit{ray.origin, Normalize(ray.direction, {0, 0, 1})};
    bool found = false;
    tree_.RayCast(unit, maxDistance, [&](int proxy, float clip) {
        const BodyId id = proxyToBody_[static_cast<std::size_t>(proxy)];
        const Body* body = bodies_.Get(id);
        if (!body || id == ignore || !(body->layer & mask) || (body->isTrigger && !includeTriggers)) return -1.0f;
        float t;
        Vec3 normal;
        if (!RaycastShape(body->shape, body->pose, unit, clip, t, normal)) return -1.0f;
        found = true;
        hit = {id, unit.At(t), normal, t, body->userData};
        return t;
    });
    return found;
}

bool PhysicsWorld::ShapeCast(const Shape& shape, const Pose& start, Vec3 motion, ShapeCastHit& hit, std::uint32_t mask,
    BodyId ignore, bool includeTriggers) const {
    if (shape.type == ShapeType::Box || !shape.IsValid() || !IsFinite(motion) || !IsFinite(start.position)) return false;
    Math::AABB swept = shape.WorldBounds(start);
    swept.Expand(shape.WorldBounds({start.position + motion, start.rotation}));
    const float length = Length(motion);
    bool found = false;
    float best = 2.0f;
    tree_.Query(swept, [&](int proxy) {
        const BodyId id = proxyToBody_[static_cast<std::size_t>(proxy)];
        const Body* body = bodies_.Get(id);
        if (!body || id == ignore || !(body->layer & mask) || (body->isTrigger && !includeTriggers)) return true;
        ShapeCastResult result;
        if (!Physics::ShapeCast(shape, start, motion, body->shape, body->pose, result)) return true;
        if (result.fraction < best || (result.fraction == best && id.index < hit.body.index)) {
            best = result.fraction;
            found = true;
            hit = {id, result.fraction * length, result.point, result.normal, result.startPenetrating, body->userData};
        }
        return true;
    });
    return found;
}

void PhysicsWorld::Overlap(const Shape& shape, const Pose& pose, std::vector<BodyId>& out, std::uint32_t mask,
    bool includeTriggers) const {
    out.clear();
    if (!shape.IsValid()) return;
    tree_.Query(shape.WorldBounds(pose), [&](int proxy) {
        const BodyId id = proxyToBody_[static_cast<std::size_t>(proxy)];
        const Body* body = bodies_.Get(id);
        if (!body || !(body->layer & mask) || (body->isTrigger && !includeTriggers)) return true;
        Manifold m;
        if (Collide(shape, pose, body->shape, body->pose, m) && m.count > 0) out.push_back(id);
        return true;
    });
    std::sort(out.begin(), out.end(), [](BodyId a, BodyId b) { return a.index < b.index; });
}

bool PhysicsWorld::ContactWith(const Shape& shape, const Pose& pose, BodyId id, Manifold& manifold) const {
    const Body* body = bodies_.Get(id);
    return body && Collide(shape, pose, body->shape, body->pose, manifold) && manifold.count > 0;
}

} // namespace Astral::Physics
