#include "Engine/Framework/GameWorld.h"

#include "Engine/Animation/Skinning.h"
#include "Engine/Core/Profiler.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Astral::Framework {

using namespace Math;

namespace {

// Physics user data carries the owning entity; the tag bit keeps foreign
// bodies (user data 0) from aliasing entity 0.
constexpr std::uint64_t kEntityTag = 1ull << 63;

TRS ToLocal(const TRS& parentWorld, const TRS& world) {
    auto divide = [](float a, float b) { return std::fabs(b) > 1.0e-12f ? a / b : a; };
    const Quat inverse = Conjugate(parentWorld.rotation);
    const Vec3 offset = Rotate(inverse, world.translation - parentWorld.translation);
    TRS local;
    local.translation = {divide(offset.x, parentWorld.scale.x), divide(offset.y, parentWorld.scale.y),
        divide(offset.z, parentWorld.scale.z)};
    local.rotation = Normalize(inverse * world.rotation);
    local.scale = {divide(world.scale.x, parentWorld.scale.x), divide(world.scale.y, parentWorld.scale.y),
        divide(world.scale.z, parentWorld.scale.z)};
    return local;
}

float MaxAbs(Vec3 v) { return std::max(std::fabs(v.x), std::max(std::fabs(v.y), std::fabs(v.z))); }

bool SameRotation(Quat a, Quat b) {
    return std::fabs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w) > 1.0f - 1.0e-7f;
}

const Graphics::Material& DefaultMaterial() {
    static const Graphics::Material material = [] {
        Graphics::Material m;
        m.name = "Default";
        return m;
    }();
    return material;
}

} // namespace

GameWorld::GameWorld(GameWorldSettings settings, Audio::AudioMixer* audio, Assets::AssetManager* assets)
    : settings_(settings), physics_(settings.physics), audio_(audio), assets_(assets) {}

GameWorld::~GameWorld() {
    std::vector<Entity> roots = Roots();
    for (Entity root : roots) DestroyImmediate(root);
    // Entities without transforms (created directly in the registry).
    std::vector<Entity> rest;
    registry_.ForEachEntity([&](Entity entity) { rest.push_back(entity); });
    for (Entity entity : rest) DestroyNow(entity);
    pendingCreate_.clear();
}

// ------------------------------------------------------------------ entities

Entity GameWorld::CreateEntity(const std::string& name, const TRS& local, Entity parent) {
    const Entity entity = registry_.Create();
    registry_.Add<World::Name>(entity, World::Name{name});
    registry_.Add<World::LocalTransform>(entity, World::LocalTransform{local});
    registry_.Add<World::WorldTransform>(entity);
    if (registry_.Valid(parent)) World::SetParent(registry_, entity, parent);
    RefreshWorld(entity);
    return entity;
}

void GameWorld::RefreshWorld(Entity entity) {
    World::WorldTransform* world = registry_.Get<World::WorldTransform>(entity);
    const World::LocalTransform* local = registry_.Get<World::LocalTransform>(entity);
    if (!world || !local) return;
    const Entity parent = World::GetParent(registry_, entity);
    const World::WorldTransform* parentWorld = registry_.Get<World::WorldTransform>(parent);
    world->trs = parentWorld ? Combine(parentWorld->trs, local->trs) : local->trs;
    world->matrix = parentWorld ? parentWorld->matrix * ToMat4(local->trs) : ToMat4(local->trs);
}

void GameWorld::CollectSubtree(Entity root, std::vector<Entity>& out) const {
    std::vector<Entity> stack{root};
    while (!stack.empty()) {
        const Entity entity = stack.back();
        stack.pop_back();
        if (!registry_.Valid(entity)) continue;
        out.push_back(entity);
        const std::vector<Entity> children = World::GetChildren(registry_, entity);
        for (auto it = children.rbegin(); it != children.rend(); ++it) stack.push_back(*it);
    }
}

void GameWorld::Destroy(Entity entity) {
    if (!IsAlive(entity)) return;
    std::vector<Entity> subtree;
    CollectSubtree(entity, subtree);
    for (Entity e : subtree) {
        if (!registry_.Has<PendingDestroy>(e)) registry_.Add<PendingDestroy>(e);
    }
    pendingDestroy_.push_back(entity);
}

void GameWorld::DestroyImmediate(Entity entity) {
    if (!registry_.Valid(entity)) return;
    std::vector<Entity> subtree;
    CollectSubtree(entity, subtree);
    // Children first, so a parent's OnDestroy never sees half its hierarchy.
    for (auto it = subtree.rbegin(); it != subtree.rend(); ++it) DestroyNow(*it);
}

void GameWorld::DestroyNow(Entity entity) {
    if (!registry_.Valid(entity)) return;
    if (Behaviours* behaviours = registry_.Get<Behaviours>(entity)) {
        const std::vector<std::shared_ptr<Behaviour>> list = behaviours->list;
        for (const auto& behaviour : list) {
            if (behaviour->created_ && !behaviour->destroyed_) {
                behaviour->destroyed_ = true;
                behaviour->OnDestroy();
            }
            behaviour->destroyed_ = true;
        }
    }
    if (!registry_.Valid(entity)) return; // an OnDestroy destroyed it already
    if (Collider* collider = registry_.Get<Collider>(entity)) Release(entity, *collider);
    if (CharacterMover* mover = registry_.Get<CharacterMover>(entity)) Release(entity, *mover);
    if (AudioSource* source = registry_.Get<AudioSource>(entity)) Release(entity, *source);
    if (ParticleSystem* particles = registry_.Get<ParticleSystem>(entity)) Release(entity, *particles);
    timers_.ClearOwner(entity);
    registry_.Destroy(entity);
}

void GameWorld::FlushDestroyed() {
    std::size_t destroyed = 0;
    // OnDestroy may destroy more entities; bounded rounds.
    for (int round = 0; round < 16 && !pendingDestroy_.empty(); ++round) {
        std::vector<Entity> batch;
        batch.swap(pendingDestroy_);
        for (Entity entity : batch) {
            if (!registry_.Valid(entity)) continue;
            std::vector<Entity> subtree;
            CollectSubtree(entity, subtree);
            for (auto it = subtree.rbegin(); it != subtree.rend(); ++it) DestroyNow(*it);
            destroyed += subtree.size();
        }
    }
    stats_.destroyedLastFrame = destroyed;
}

Entity GameWorld::Find(const std::string& name) const {
    Entity best{};
    for (Entity entity : registry_.EntitiesWith<World::Name>()) {
        const World::Name* n = registry_.Get<World::Name>(entity);
        if (n && n->value == name && IsAlive(entity) && (best.IsNull() || entity.index < best.index)) best = entity;
    }
    return best;
}

std::vector<Entity> GameWorld::Roots() const {
    std::vector<Entity> roots;
    for (Entity entity : registry_.EntitiesWith<World::LocalTransform>()) {
        if (registry_.Valid(entity) && !registry_.Valid(World::GetParent(registry_, entity))) roots.push_back(entity);
    }
    std::sort(roots.begin(), roots.end());
    return roots;
}

bool GameWorld::SetParent(Entity child, Entity parent) {
    if (!registry_.Valid(child)) return false;
    const TRS world = GetWorld(child);
    if (!World::SetParent(registry_, child, parent)) return false;
    // Keep the world pose (Unity's worldPositionStays).
    const World::WorldTransform* parentWorld = registry_.Get<World::WorldTransform>(parent);
    SetLocal(child, parentWorld ? ToLocal(parentWorld->trs, world) : world);
    return true;
}

// ------------------------------------------------------------------ behaviours

Behaviour* GameWorld::AddBehaviour(Entity entity, const std::string& type, const Core::JsonValue& properties,
    std::string& error, const BehaviourRegistry& registry) {
    if (!IsAlive(entity)) {
        error = "AddBehaviour on a destroyed entity";
        return nullptr;
    }
    std::shared_ptr<Behaviour> behaviour = registry.Create(type, properties, error);
    if (!behaviour) return nullptr;
    Behaviour* raw = behaviour.get();
    Attach(entity, std::move(behaviour), type);
    return raw;
}

void GameWorld::Attach(Entity entity, std::shared_ptr<Behaviour> behaviour, const std::string& typeName) {
    Behaviours& behaviours = registry_.Has<Behaviours>(entity) ? *registry_.Get<Behaviours>(entity) : registry_.Add<Behaviours>(entity);
    behaviour->world_ = this;
    behaviour->entity_ = entity;
    if (!typeName.empty()) behaviour->typeName_ = typeName;
    behaviours.list.push_back(behaviour);
    if (batchDepth_ > 0) {
        pendingCreate_.push_back(behaviour);
    } else {
        Create(*behaviour);
    }
}

void GameWorld::Create(Behaviour& behaviour) {
    if (behaviour.created_ || behaviour.destroyed_) return;
    behaviour.created_ = true;
    behaviour.OnCreate();
}

void GameWorld::EndCreateBatch() {
    if (batchDepth_ == 0 || --batchDepth_ > 0) return;
    std::vector<std::weak_ptr<Behaviour>> pending;
    pending.swap(pendingCreate_);
    for (const auto& weak : pending) {
        if (auto behaviour = weak.lock()) {
            if (registry_.Valid(behaviour->entity_)) Create(*behaviour);
        }
    }
}

std::vector<std::pair<Entity, std::shared_ptr<Behaviour>>> GameWorld::SnapshotBehaviours() {
    std::vector<std::pair<Entity, std::shared_ptr<Behaviour>>> snapshot;
    for (Entity entity : SortedEntities<Behaviours>()) {
        for (const auto& behaviour : registry_.Get<Behaviours>(entity)->list) snapshot.emplace_back(entity, behaviour);
    }
    return snapshot;
}

void GameWorld::StartPending() {
    for (auto& [entity, behaviour] : SnapshotBehaviours()) {
        if (behaviour->created_ && !behaviour->started_ && !behaviour->destroyed_ && behaviour->enabled
            && registry_.Valid(entity)) {
            behaviour->started_ = true;
            behaviour->OnStart();
        }
    }
}

namespace {
bool Ticks(const Behaviour& behaviour) { return behaviour.enabled && behaviour.Started(); }
} // namespace

// ------------------------------------------------------------------ transforms

TRS GameWorld::GetLocal(Entity entity) const {
    const World::LocalTransform* local = registry_.Get<World::LocalTransform>(entity);
    return local ? local->trs : TRS{};
}

void GameWorld::SetLocal(Entity entity, const TRS& local) {
    World::LocalTransform* transform = registry_.Get<World::LocalTransform>(entity);
    if (!transform) return;
    transform->trs = local;
    RefreshWorld(entity);
}

TRS GameWorld::GetWorld(Entity entity) const {
    const World::WorldTransform* world = registry_.Get<World::WorldTransform>(entity);
    return world ? world->trs : TRS{};
}

void GameWorld::SetWorldPosition(Entity entity, Vec3 position) {
    const TRS world = GetWorld(entity);
    WriteWorldPose(entity, position, world.rotation);
}

void GameWorld::SetWorldRotation(Entity entity, Quat rotation) {
    const TRS world = GetWorld(entity);
    WriteWorldPose(entity, world.translation, Normalize(rotation));
}

void GameWorld::WriteWorldPose(Entity entity, Vec3 position, Quat rotation) {
    World::LocalTransform* local = registry_.Get<World::LocalTransform>(entity);
    if (!local) return;
    TRS world = GetWorld(entity);
    world.translation = position;
    world.rotation = rotation;
    const World::WorldTransform* parentWorld = registry_.Get<World::WorldTransform>(World::GetParent(registry_, entity));
    local->trs = parentWorld ? ToLocal(parentWorld->trs, world) : world;
    RefreshWorld(entity);
}

void GameWorld::UpdateTransforms() { World::UpdateTransforms(registry_); }

// ------------------------------------------------------------------ frame

void GameWorld::Tick(float dt) {
    ASTRAL_PROFILE_SCOPE("GameWorld.Tick");
    if (!std::isfinite(dt) || dt < 0.0f) dt = 0.0f;
    dt = std::min(dt, settings_.maxFrameDelta);
    const float scaled = dt * std::max(0.0f, std::isfinite(settings_.timeScale) ? settings_.timeScale : 1.0f);
    deltaTime_ = scaled;
    time_ += scaled;
    ++stats_.frame;

    UpdateTransforms();
    StartPending();

    const float fixed = settings_.fixedDelta > 1.0e-5f ? settings_.fixedDelta : 1.0f / 60.0f;
    accumulator_ += scaled;
    int steps = 0;
    while (accumulator_ >= fixed && steps < std::max(1, settings_.maxFixedSteps)) {
        FixedStep(fixed);
        accumulator_ -= fixed;
        ++steps;
    }
    if (accumulator_ >= fixed) accumulator_ = std::fmod(accumulator_, fixed); // drop what could not be simulated
    stats_.fixedStepsLastFrame = steps;

    timers_.Tick(scaled);
    for (auto& [entity, behaviour] : SnapshotBehaviours()) {
        if (Ticks(*behaviour) && !behaviour->destroyed_) behaviour->OnUpdate(scaled);
    }
    UpdateTransforms();
    UpdateAnimation(scaled);
    for (auto& [entity, behaviour] : SnapshotBehaviours()) {
        if (Ticks(*behaviour) && !behaviour->destroyed_) behaviour->OnLateUpdate(scaled);
    }
    UpdateTransforms();
    UpdatePresentation();
    particles_.Update(scaled);

    FlushDestroyed();
    events_.Dispatch();

    stats_.entities = registry_.AliveCount();
    stats_.bodies = physics_.BodyCount();
    stats_.behaviours = 0;
    for (Entity entity : registry_.EntitiesWith<Behaviours>()) stats_.behaviours += registry_.Get<Behaviours>(entity)->list.size();
}

void GameWorld::FixedStep(float dt) {
    for (auto& [entity, behaviour] : SnapshotBehaviours()) {
        if (Ticks(*behaviour) && !behaviour->destroyed_) behaviour->OnFixedUpdate(dt);
    }
    UpdateTransforms();
    SyncToPhysics(dt);
    physics_.Step(dt);
    SyncFromPhysics();
    DispatchContacts();
}

// ------------------------------------------------------------------ physics sync

void GameWorld::EnsureBody(Entity entity, Collider& collider) {
    if (collider.failed || (!collider.body.IsNull() && physics_.GetBody(collider.body))) return;
    collider.body = {};
    const TRS world = GetWorld(entity);
    const Vec3 scale = Abs(world.scale);
    Physics::Shape shape;
    switch (collider.shape) {
    case ColliderShape::Sphere: shape = Physics::Shape::Sphere(collider.radius * MaxAbs(scale)); break;
    case ColliderShape::Capsule:
        shape = Physics::Shape::CapsuleFromHeight(collider.radius * std::max(scale.x, scale.z), collider.height * scale.y);
        break;
    case ColliderShape::Box: shape = Physics::Shape::Box(Multiply(collider.halfExtents, scale)); break;
    case ColliderShape::Mesh: {
        // Unit-scale instances share the cooked geometry; scale is baked into
        // the vertices, so scaled instances cook their own copy.
        const bool unitScale = DistanceSquared(scale, {1.0f, 1.0f, 1.0f}) < 1.0e-10f;
        std::shared_ptr<const Physics::TriangleMesh> triangles = unitScale ? collider.triangles : nullptr;
        if (!triangles) {
            std::shared_ptr<const Graphics::MeshData> source = collider.sourceMesh;
            if (!source) {
                if (const MeshRenderer* renderer = registry_.Get<MeshRenderer>(entity)) source = renderer->mesh;
            }
            std::string error;
            if (source) triangles = CookCollisionMesh(*source, scale, error);
        }
        if (!triangles) {
            collider.failed = true;
            return;
        }
        if (unitScale) collider.triangles = triangles;
        shape = Physics::Shape::Mesh(triangles);
        break;
    }
    }
    const RigidBody* rigidBody = registry_.Get<RigidBody>(entity);
    Physics::BodyDesc desc;
    desc.type = rigidBody ? rigidBody->type : Physics::BodyType::Static;
    // Triangle meshes cannot be simulated (as with Unity's non-convex MeshCollider).
    if (shape.type == Physics::ShapeType::Mesh && desc.type == Physics::BodyType::Dynamic) desc.type = Physics::BodyType::Kinematic;
    desc.shape = shape;
    desc.position = world.translation;
    desc.rotation = world.rotation;
    desc.friction = collider.friction;
    desc.restitution = collider.restitution;
    desc.isTrigger = collider.isTrigger;
    desc.layer = collider.layer;
    desc.mask = collider.mask;
    desc.userData = Pack(entity) | kEntityTag;
    if (rigidBody) {
        desc.mass = rigidBody->mass;
        desc.linearDamping = rigidBody->linearDamping;
        desc.angularDamping = rigidBody->angularDamping;
        desc.gravityScale = rigidBody->gravityScale;
        desc.lockRotation = rigidBody->lockRotation;
        desc.linearVelocity = rigidBody->velocity;
    }
    collider.body = physics_.CreateBody(desc);
    collider.failed = collider.body.IsNull();
    collider.syncedPosition = world.translation;
    collider.syncedRotation = world.rotation;
}

void GameWorld::EnsureCharacter(Entity entity, CharacterMover& mover) {
    if (mover.controller) return;
    Physics::CharacterSettings settings;
    settings.radius = mover.radius;
    settings.height = mover.height;
    settings.stepHeight = mover.stepHeight;
    settings.maxSlopeDegrees = mover.maxSlopeDegrees;
    settings.jumpSpeed = mover.jumpSpeed;
    settings.gravity = mover.gravity;
    settings.groundAcceleration = mover.groundAcceleration;
    settings.airAcceleration = mover.airAcceleration;
    settings.collisionMask = mover.collisionMask;
    const TRS world = GetWorld(entity);
    mover.controller = std::make_shared<Physics::CharacterController>(physics_, settings, world.translation);
    Physics::BodyDesc desc;
    desc.type = Physics::BodyType::Kinematic;
    desc.shape = Physics::Shape::CapsuleFromHeight(mover.controller->Settings().radius, mover.controller->Settings().height);
    desc.position = mover.controller->Center();
    desc.userData = Pack(entity) | kEntityTag;
    mover.body = physics_.CreateBody(desc);
    mover.controller->SetIgnoredBody(mover.body);
    WriteWorldPose(entity, mover.controller->FootPosition(), world.rotation);
}

void GameWorld::SyncToPhysics(float dt) {
    for (Entity entity : SortedEntities<Collider>()) {
        Collider& collider = *registry_.Get<Collider>(entity);
        EnsureBody(entity, collider);
        if (collider.body.IsNull()) continue;
        const TRS world = GetWorld(entity);
        const Physics::Body* body = physics_.GetBody(collider.body);
        if (!body) continue;
        if (body->type == Physics::BodyType::Kinematic) {
            physics_.MoveKinematic(collider.body, {world.translation, world.rotation}, dt);
        } else if (DistanceSquared(world.translation, collider.syncedPosition) > 1.0e-10f
            || !SameRotation(world.rotation, collider.syncedRotation)) {
            // Moved by gameplay code: teleport the body (velocity is kept).
            physics_.SetPose(collider.body, {world.translation, world.rotation});
        }
        collider.syncedPosition = world.translation;
        collider.syncedRotation = world.rotation;
    }
    for (Entity entity : SortedEntities<CharacterMover>()) {
        CharacterMover& mover = *registry_.Get<CharacterMover>(entity);
        EnsureCharacter(entity, mover);
        const TRS world = GetWorld(entity);
        if (DistanceSquared(world.translation, mover.controller->FootPosition()) > 1.0e-10f) {
            mover.controller->Teleport(world.translation);
        }
        PushBodies(mover, dt);
        mover.controller->Move(mover.desiredVelocity, mover.jump, dt);
        mover.jump = false;
        WriteWorldPose(entity, mover.controller->FootPosition(), world.rotation);
        if (!mover.body.IsNull()) physics_.MoveKinematic(mover.body, {mover.controller->Center(), Quat{}}, dt);
    }
}

void GameWorld::PushBodies(CharacterMover& mover, float dt) {
    const Vec3 desired = Horizontal(mover.desiredVelocity);
    const float speed = Length(desired);
    if (!(mover.pushStrength > 0.0f) || speed < 1.0e-4f || !mover.controller) return;
    const Vec3 direction = desired / speed;
    const Physics::CharacterSettings& settings = mover.controller->Settings();
    // Probe just ahead of the capsule (past its skin) along the walking direction.
    const Physics::Shape probe = Physics::Shape::CapsuleFromHeight(settings.radius + settings.skin, settings.height);
    const Vec3 center = mover.controller->Center() + direction * (settings.skin + speed * dt);
    std::vector<Physics::BodyId> touching;
    physics_.Overlap(probe, {center, Quat{}}, touching, settings.collisionMask, false);
    for (Physics::BodyId id : touching) {
        const Physics::Body* body = physics_.GetBody(id);
        if (!body || id == mover.body || body->type != Physics::BodyType::Dynamic || !(body->inverseMass > 0.0f)) continue;
        const float mass = 1.0f / body->inverseMass;
        if (mass > mover.maxPushMass) continue;
        const float along = Dot(body->linearVelocity, direction);
        if (along >= speed) continue;
        physics_.ApplyImpulse(id, direction * ((speed - along) * mass * std::min(1.0f, mover.pushStrength)), body->pose.position);
    }
}

void GameWorld::SyncFromPhysics() {
    for (Entity entity : SortedEntities<Collider>()) {
        Collider& collider = *registry_.Get<Collider>(entity);
        const Physics::Body* body = physics_.GetBody(collider.body);
        if (!body || body->type != Physics::BodyType::Dynamic) continue;
        WriteWorldPose(entity, body->pose.position, body->pose.rotation);
        collider.syncedPosition = body->pose.position;
        collider.syncedRotation = body->pose.rotation;
    }
    for (Entity entity : SortedEntities<Collider>()) {
        // Kinematic bodies reached their targets during the step.
        Collider& collider = *registry_.Get<Collider>(entity);
        if (const Physics::Body* body = physics_.GetBody(collider.body)) {
            if (body->type == Physics::BodyType::Kinematic) {
                collider.syncedPosition = body->pose.position;
                collider.syncedRotation = body->pose.rotation;
            }
        }
    }
}

Entity GameWorld::EntityFromBody(Physics::BodyId id) const {
    const Physics::Body* body = physics_.GetBody(id);
    if (!body || !(body->userData & kEntityTag)) return {};
    const Entity entity = Unpack(body->userData & ~kEntityTag);
    return registry_.Valid(entity) ? entity : Entity{};
}

void GameWorld::DispatchContacts() {
    const std::vector<Physics::ContactEvent> contactEvents = physics_.Events();
    for (const Physics::ContactEvent& event : contactEvents) {
        const Entity a = EntityFromBody(event.a), b = EntityFromBody(event.b);
        if (a.IsNull() && b.IsNull()) continue;
        const bool begin = event.type == Physics::ContactEventType::Begin;
        events_.Publish(ContactNotification{a, b, begin, event.trigger, event.point, event.normal});
        auto notify = [&](Entity self, Entity other, Vec3 normal) {
            Behaviours* behaviours = registry_.Get<Behaviours>(self);
            if (!behaviours) return;
            const CollisionInfo info{other, event.point, normal, event.trigger};
            const std::vector<std::shared_ptr<Behaviour>> list = behaviours->list;
            for (const auto& behaviour : list) {
                if (!behaviour->enabled || !behaviour->created_ || behaviour->destroyed_) continue;
                if (event.trigger) {
                    if (begin) behaviour->OnTriggerEnter(info);
                    else behaviour->OnTriggerExit(info);
                } else {
                    if (begin) behaviour->OnCollisionEnter(info);
                    else behaviour->OnCollisionExit(info);
                }
            }
        };
        if (!a.IsNull()) notify(a, b, event.normal);
        if (!b.IsNull()) notify(b, a, -event.normal);
    }
}

bool GameWorld::Raycast(const Ray& ray, float maxDistance, EntityRaycastHit& hit, std::uint32_t mask, Entity ignore) const {
    Physics::BodyId ignoreBody{};
    if (const Collider* collider = registry_.Get<Collider>(ignore)) ignoreBody = collider->body;
    else if (const CharacterMover* mover = registry_.Get<CharacterMover>(ignore)) ignoreBody = mover->body;
    Physics::RaycastHit physicsHit;
    if (!physics_.Raycast(ray, maxDistance, physicsHit, mask, ignoreBody)) return false;
    hit = {EntityFromBody(physicsHit.body), physicsHit.point, physicsHit.normal, physicsHit.distance};
    return true;
}

// ------------------------------------------------------------------ animation, audio, particles

void GameWorld::UpdateAnimation(float dt) {
    for (Entity entity : SortedEntities<AnimatorComponent>()) {
        AnimatorComponent& component = *registry_.Get<AnimatorComponent>(entity);
        if (!component.animator && component.skeleton && component.machine) {
            component.animator = std::make_shared<Animation::Animator>(*component.skeleton, *component.machine);
        }
        if (!component.animator) continue;
        component.animator->Update(dt * component.speed);
        for (const Animation::AnimEvent& event : component.animator->Events()) {
            events_.Publish(AnimationNotification{entity, event.name, event.state});
        }
        const Vec3 rootMotion = component.animator->ConsumeRootMotion();
        if (component.applyRootMotion && LengthSquared(rootMotion) > 0.0f) {
            const TRS world = GetWorld(entity);
            WriteWorldPose(entity, world.translation + Rotate(world.rotation, rootMotion), world.rotation);
        }
        if (component.bindMesh && component.skeleton) {
            Animation::ComputeSkinMatrices(*component.skeleton, component.animator->GetPose(), component.skinMatrices);
            if (!component.skinned) component.skinned = std::make_shared<Graphics::MeshData>();
            Animation::SkinMesh(*component.bindMesh, component.skinMatrices, *component.skinned);
            if (MeshRenderer* renderer = registry_.Get<MeshRenderer>(entity)) renderer->mesh = component.skinned;
        }
    }
}

void GameWorld::UpdatePresentation() {
    for (Entity entity : SortedEntities<ParticleSystem>()) {
        ParticleSystem& system = *registry_.Get<ParticleSystem>(entity);
        if (!system.started && system.playOnStart) PlayParticles(entity);
        if (system.emitter.IsNull()) continue;
        if (VFX::ParticleEmitter* emitter = particles_.Get(system.emitter)) {
            if (system.followEntity) emitter->position = GetWorld(entity).translation;
        } else {
            system.emitter = {}; // finished and collected
        }
    }
    if (!audio_) return;
    Entity listener{};
    for (Entity entity : SortedEntities<AudioListener>()) {
        if (registry_.Get<AudioListener>(entity)->active && IsAlive(entity)) {
            listener = entity;
            break;
        }
    }
    if (listener.IsNull()) listener = ActiveCamera();
    if (!listener.IsNull()) {
        const TRS pose = GetWorld(listener);
        audio_->SetListener(pose.translation, Rotate(pose.rotation, {0, 0, 1}), Rotate(pose.rotation, {0, 1, 0}));
    }
    for (Entity entity : SortedEntities<AudioSource>()) {
        AudioSource& source = *registry_.Get<AudioSource>(entity);
        if (!source.started && source.playOnStart) PlayAudio(entity);
        if (source.voice.IsNull()) continue;
        if (!audio_->IsPlaying(source.voice)) {
            source.voice = {};
        } else if (source.spatial) {
            audio_->SetVoicePosition(source.voice, GetWorld(entity).translation);
        }
    }
}

bool GameWorld::PlayAudio(Entity entity) {
    AudioSource* source = registry_.Get<AudioSource>(entity);
    if (!source) return false;
    source->started = true;
    if (!audio_ || !source->clip) return false;
    if (!source->voice.IsNull()) audio_->Stop(source->voice);
    Audio::PlayParams params;
    params.volume = source->volume;
    params.pitch = source->pitch;
    params.loop = source->loop;
    params.bus = source->bus;
    params.spatial = source->spatial;
    params.position = GetWorld(entity).translation;
    params.minDistance = source->minDistance;
    params.maxDistance = source->maxDistance;
    params.priority = source->priority;
    source->voice = audio_->Play(source->clip.get(), params);
    return !source->voice.IsNull();
}

void GameWorld::StopAudio(Entity entity) {
    if (AudioSource* source = registry_.Get<AudioSource>(entity)) Release(entity, *source);
}

bool GameWorld::PlayParticles(Entity entity) {
    ParticleSystem* system = registry_.Get<ParticleSystem>(entity);
    if (!system) return false;
    system->started = true;
    if (!system->emitter.IsNull()) particles_.Stop(system->emitter);
    system->emitter = particles_.Spawn(system->ToSettings(), GetWorld(entity).translation);
    return !system->emitter.IsNull();
}

void GameWorld::StopParticles(Entity entity) {
    if (ParticleSystem* system = registry_.Get<ParticleSystem>(entity)) Release(entity, *system);
}

Audio::VoiceId GameWorld::PlaySoundAt(const Audio::AudioClip* clip, Vec3 position, float volume) {
    if (!audio_ || !clip) return {};
    Audio::PlayParams params;
    params.volume = volume;
    params.spatial = true;
    params.position = position;
    return audio_->Play(clip, params);
}

// ------------------------------------------------------------------ release

void GameWorld::Release(Entity, Collider& collider) {
    if (!collider.body.IsNull()) physics_.DestroyBody(collider.body);
    collider.body = {};
}

void GameWorld::Release(Entity, CharacterMover& mover) {
    if (!mover.body.IsNull()) physics_.DestroyBody(mover.body);
    mover.body = {};
    mover.controller.reset();
}

void GameWorld::Release(Entity, AudioSource& source) {
    if (audio_ && !source.voice.IsNull()) audio_->Stop(source.voice);
    source.voice = {};
}

void GameWorld::Release(Entity, ParticleSystem& system) {
    // Stop emitting; live particles fade out on their own.
    if (!system.emitter.IsNull()) particles_.Stop(system.emitter);
    system.emitter = {};
}

// ------------------------------------------------------------------ rendering

Entity GameWorld::ActiveCamera() const {
    Entity best{};
    int bestPriority = 0;
    for (Entity entity : registry_.EntitiesWith<Camera>()) {
        const Camera* camera = registry_.Get<Camera>(entity);
        if (!camera || !camera->active || !IsAlive(entity)) continue;
        if (best.IsNull() || camera->priority > bestPriority || (camera->priority == bestPriority && entity.index < best.index)) {
            best = entity;
            bestPriority = camera->priority;
        }
    }
    return best;
}

bool GameWorld::BuildRenderScene(Graphics::RenderScene& scene, Graphics::RenderView& view, int width, int height) const {
    const Entity cameraEntity = ActiveCamera();
    if (cameraEntity.IsNull()) return false;
    const Camera& camera = *registry_.Get<Camera>(cameraEntity);
    const TRS eye = GetWorld(cameraEntity);
    const Vec3 forward = Rotate(eye.rotation, {0, 0, 1});
    view = Graphics::RenderView::Perspective(eye.translation, eye.translation + forward, camera.fieldOfView, width, height,
        camera.nearPlane, camera.farPlane);

    std::vector<Entity> renderers = registry_.EntitiesWith<MeshRenderer>();
    std::sort(renderers.begin(), renderers.end());
    for (Entity entity : renderers) {
        const MeshRenderer& renderer = *registry_.Get<MeshRenderer>(entity);
        const World::WorldTransform* world = registry_.Get<World::WorldTransform>(entity);
        if (!renderer.visible || (!renderer.mesh && !renderer.lods) || !world) continue;
        Graphics::DrawItem draw;
        draw.mesh = renderer.mesh.get();
        if (renderer.lods && renderer.lods->Count() > 0) {
            int level = renderer.forcedLod;
            if (level < 0) {
                // Bounding sphere of the mesh in world space against the view.
                const Math::AABB bounds = renderer.lods->levels.front().mesh->bounds;
                const Vec3 center = TransformPoint(world->matrix, bounds.Center());
                const float scale = std::max(Length(Vec3{world->matrix.m[0][0], world->matrix.m[1][0], world->matrix.m[2][0]}),
                    std::max(Length(Vec3{world->matrix.m[0][1], world->matrix.m[1][1], world->matrix.m[2][1]}),
                        Length(Vec3{world->matrix.m[0][2], world->matrix.m[1][2], world->matrix.m[2][2]})));
                const float size = Graphics::ScreenSize(Length(bounds.Extents()) * scale, Distance(center, eye.translation),
                    Radians(camera.fieldOfView)) * environment_.lodBias;
                level = renderer.lods->Select(size, renderer.currentLod);
            }
            level = std::min(level, static_cast<int>(renderer.lods->Count()) - 1);
            renderer.currentLod = level;
            draw.mesh = renderer.lods->levels[static_cast<std::size_t>(level)].mesh.get();
        }
        if (!draw.mesh) continue;
        draw.material = renderer.material ? renderer.material.get() : &DefaultMaterial();
        draw.world = world->matrix;
        draw.objectId = renderer.objectId != 0 ? renderer.objectId : entity.index + 1;
        draw.tint = renderer.tint;
        draw.opacity = renderer.opacity;
        scene.draws.push_back(draw);
    }
    std::vector<Entity> lights = registry_.EntitiesWith<Light>();
    std::sort(lights.begin(), lights.end());
    float sunIntensity = -1.0f;
    for (Entity entity : lights) {
        const Light& light = *registry_.Get<Light>(entity);
        const TRS pose = GetWorld(entity);
        if (light.type == LightType::Directional) {
            if (light.intensity > sunIntensity) {
                sunIntensity = light.intensity;
                scene.sun.direction = Normalize(Rotate(pose.rotation, {0, 0, 1}), {0, -1, 0});
                scene.sun.color = light.color;
                scene.sun.intensity = light.intensity;
                scene.sun.castShadows = light.castShadows;
            }
        } else {
            scene.pointLights.push_back({pose.translation, light.color, light.intensity, light.radius});
        }
    }
    particles_.Collect(scene.billboards);
    scene.ambientSky = environment_.ambientSky;
    scene.ambientGround = environment_.ambientGround;
    scene.fog.enabled = environment_.fog;
    scene.fog.color = environment_.fogColor;
    scene.fog.density = environment_.fogDensity;
    scene.post.exposure = environment_.exposure;
    scene.post.outlines = environment_.outlines;
    scene.post.bloom = environment_.bloom;
    scene.shadows.enabled = environment_.shadows;
    scene.shadows.focus = eye.translation;
    scene.shadows.radius = environment_.shadowRadius;
    scene.time = time_;
    return true;
}

} // namespace Astral::Framework
