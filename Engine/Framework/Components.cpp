#include "Engine/Framework/Components.h"

#include <string>
#include <vector>

namespace Astral::Framework {

VFX::EmitterSettings ParticleSystem::ToSettings() const {
    VFX::EmitterSettings s;
    s.rate = rate;
    s.burst = burst;
    s.duration = duration;
    s.lifetimeMin = lifetimeMin;
    s.lifetimeMax = lifetimeMax < lifetimeMin ? lifetimeMin : lifetimeMax;
    s.direction = direction;
    s.spreadDegrees = spreadDegrees;
    s.speedMin = speedMin;
    s.speedMax = speedMax < speedMin ? speedMin : speedMax;
    s.spawnRadius = spawnRadius;
    s.gravity = gravity;
    s.drag = drag;
    s.sizeStart = sizeStart;
    s.sizeEnd = sizeEnd;
    s.colors = {{0.0f, colorStart}, {1.0f, colorEnd}};
    s.blend = additive ? Graphics::BlendMode::Additive : Graphics::BlendMode::AlphaBlend;
    s.maxParticles = maxParticles;
    s.seed = static_cast<std::uint32_t>(seed);
    return s;
}

std::shared_ptr<const Physics::TriangleMesh> CookCollisionMesh(const Graphics::MeshData& mesh, Math::Vec3 scale,
    std::string& error) {
    std::vector<Math::Vec3> positions;
    positions.reserve(mesh.vertices.size());
    for (const Graphics::Vertex& vertex : mesh.vertices) positions.push_back(Math::Multiply(vertex.position, scale));
    auto cooked = Physics::TriangleMesh::Create(positions, mesh.indices, error);
    if (!cooked) error = "collision mesh '" + mesh.name + "': " + error;
    return cooked;
}

namespace {

Core::TypeRegistry BuildFrameworkTypes() {
    Core::TypeRegistry r;
    r.Register<TransformDesc>("Transform")
        .Field("position", &TransformDesc::position)
        .Field("rotation", &TransformDesc::rotation, "[x, y, z, w] or {\"euler\": [pitch, yaw, roll]} degrees")
        .Field("scale", &TransformDesc::scale);
    r.Register<MeshRenderer>("MeshRenderer")
        .Field("mesh", &MeshRenderer::meshRef, "primitive:cube|sphere|plane|capsule|cylinder or model.glb#mesh.primitive")
        .Field("material", &MeshRenderer::materialRef, "scene material name or model.glb#materialN")
        .Field("tint", &MeshRenderer::tint)
        .Field("opacity", &MeshRenderer::opacity).Range(0.0, 1.0)
        .Field("visible", &MeshRenderer::visible)
        .Field("objectId", &MeshRenderer::objectId);
    r.Register<Light>("Light")
        .EnumField("type", &Light::type, {"Directional", "Point"})
        .Field("color", &Light::color)
        .Field("intensity", &Light::intensity).Range(0.0, 1.0e4)
        .Field("radius", &Light::radius).Range(0.0, 1.0e4)
        .Field("castShadows", &Light::castShadows);
    r.Register<Camera>("Camera")
        .Field("fieldOfView", &Camera::fieldOfView).Range(1.0, 179.0)
        .Field("nearPlane", &Camera::nearPlane).Range(1.0e-3, 1.0e4)
        .Field("farPlane", &Camera::farPlane).Range(1.0e-2, 1.0e7)
        .Field("priority", &Camera::priority)
        .Field("active", &Camera::active);
    r.Register<Collider>("Collider")
        .EnumField("shape", &Collider::shape, {"Sphere", "Capsule", "Box", "Mesh"})
        .Field("radius", &Collider::radius).Range(1.0e-4, 1.0e5)
        .Field("height", &Collider::height).Range(1.0e-4, 1.0e5)
        .Field("halfExtents", &Collider::halfExtents)
        .Field("mesh", &Collider::meshRef)
        .Field("friction", &Collider::friction).Range(0.0, 10.0)
        .Field("restitution", &Collider::restitution).Range(0.0, 1.0)
        .Field("isTrigger", &Collider::isTrigger)
        .Field("layer", &Collider::layer)
        .Field("mask", &Collider::mask);
    r.Register<RigidBody>("RigidBody")
        .EnumField("type", &RigidBody::type, {"Static", "Kinematic", "Dynamic"})
        .Field("mass", &RigidBody::mass).Range(1.0e-4, 1.0e7)
        .Field("linearDamping", &RigidBody::linearDamping).Range(0.0, 1.0e3)
        .Field("angularDamping", &RigidBody::angularDamping).Range(0.0, 1.0e3)
        .Field("gravityScale", &RigidBody::gravityScale)
        .Field("lockRotation", &RigidBody::lockRotation)
        .Field("velocity", &RigidBody::velocity);
    r.Register<CharacterMover>("CharacterMover")
        .Field("radius", &CharacterMover::radius).Range(0.05, 100.0)
        .Field("height", &CharacterMover::height).Range(0.1, 100.0)
        .Field("stepHeight", &CharacterMover::stepHeight).Range(0.0, 100.0)
        .Field("maxSlopeDegrees", &CharacterMover::maxSlopeDegrees).Range(0.0, 89.0)
        .Field("jumpSpeed", &CharacterMover::jumpSpeed).Range(0.0, 1.0e3)
        .Field("gravity", &CharacterMover::gravity)
        .Field("groundAcceleration", &CharacterMover::groundAcceleration).Range(0.0, 1.0e4)
        .Field("airAcceleration", &CharacterMover::airAcceleration).Range(0.0, 1.0e4)
        .Field("collisionMask", &CharacterMover::collisionMask)
        .Field("pushStrength", &CharacterMover::pushStrength).Range(0.0, 1.0)
        .Field("maxPushMass", &CharacterMover::maxPushMass).Range(0.0, 1.0e6);
    r.Register<AudioSource>("AudioSource")
        .Field("clip", &AudioSource::clipRef)
        .Field("volume", &AudioSource::volume).Range(0.0, 16.0)
        .Field("pitch", &AudioSource::pitch).Range(0.01, 16.0)
        .Field("loop", &AudioSource::loop)
        .Field("spatial", &AudioSource::spatial)
        .Field("playOnStart", &AudioSource::playOnStart)
        .Field("minDistance", &AudioSource::minDistance).Range(0.0, 1.0e5)
        .Field("maxDistance", &AudioSource::maxDistance).Range(0.0, 1.0e5)
        .EnumField("bus", &AudioSource::bus, {"Master", "Sfx", "Music", "Voice", "Ui"})
        .Field("priority", &AudioSource::priority);
    r.Register<AudioListener>("AudioListener").Field("active", &AudioListener::active);
    r.Register<ParticleSystem>("ParticleSystem")
        .Field("rate", &ParticleSystem::rate).Range(0.0, 1.0e5)
        .Field("burst", &ParticleSystem::burst).Range(0.0, 1.0e5)
        .Field("duration", &ParticleSystem::duration)
        .Field("lifetimeMin", &ParticleSystem::lifetimeMin).Range(0.0, 1.0e4)
        .Field("lifetimeMax", &ParticleSystem::lifetimeMax).Range(0.0, 1.0e4)
        .Field("direction", &ParticleSystem::direction)
        .Field("spreadDegrees", &ParticleSystem::spreadDegrees).Range(0.0, 180.0)
        .Field("speedMin", &ParticleSystem::speedMin)
        .Field("speedMax", &ParticleSystem::speedMax)
        .Field("spawnRadius", &ParticleSystem::spawnRadius).Range(0.0, 1.0e4)
        .Field("gravity", &ParticleSystem::gravity)
        .Field("drag", &ParticleSystem::drag).Range(0.0, 1.0e3)
        .Field("sizeStart", &ParticleSystem::sizeStart).Range(0.0, 1.0e3)
        .Field("sizeEnd", &ParticleSystem::sizeEnd).Range(0.0, 1.0e3)
        .Field("colorStart", &ParticleSystem::colorStart)
        .Field("colorEnd", &ParticleSystem::colorEnd)
        .Field("additive", &ParticleSystem::additive)
        .Field("maxParticles", &ParticleSystem::maxParticles).Range(1.0, 1.0e6)
        .Field("seed", &ParticleSystem::seed)
        .Field("playOnStart", &ParticleSystem::playOnStart)
        .Field("followEntity", &ParticleSystem::followEntity);
    r.Register<AnimatorComponent>("Animator")
        .Field("speed", &AnimatorComponent::speed).Range(0.0, 100.0)
        .Field("applyRootMotion", &AnimatorComponent::applyRootMotion);
    r.Register<Graphics::Material>("Material")
        .Field("name", &Graphics::Material::name)
        .EnumField("shading", &Graphics::Material::shading, {"Toon", "Lit", "Unlit", "Water"})
        .EnumField("blend", &Graphics::Material::blend, {"Opaque", "AlphaBlend", "Additive"})
        .Field("baseColor", &Graphics::Material::baseColor)
        .Field("opacity", &Graphics::Material::opacity).Range(0.0, 1.0)
        .Field("texture", &Graphics::Material::baseTexturePath, "base colour texture asset path")
        .Field("uvScale", &Graphics::Material::uvScale)
        .Field("emissive", &Graphics::Material::emissive)
        .Field("shadeColor", &Graphics::Material::shadeColor)
        .Field("shadeThreshold", &Graphics::Material::shadeThreshold)
        .Field("shadeSoftness", &Graphics::Material::shadeSoftness)
        .Field("specularThreshold", &Graphics::Material::specularThreshold)
        .Field("specularIntensity", &Graphics::Material::specularIntensity)
        .Field("rimColor", &Graphics::Material::rimColor)
        .Field("rimIntensity", &Graphics::Material::rimIntensity)
        .Field("rimWidth", &Graphics::Material::rimWidth)
        .Field("roughness", &Graphics::Material::roughness).Range(0.0, 1.0)
        .Field("metallic", &Graphics::Material::metallic).Range(0.0, 1.0)
        .Field("reflectivity", &Graphics::Material::reflectivity).Range(0.0, 1.0)
        .Field("waveAmplitude", &Graphics::Material::waveAmplitude)
        .Field("waveScale", &Graphics::Material::waveScale)
        .Field("castShadows", &Graphics::Material::castShadows)
        .Field("receiveShadows", &Graphics::Material::receiveShadows)
        .Field("outline", &Graphics::Material::outline)
        .Field("cameraFade", &Graphics::Material::cameraFade)
        .Field("doubleSided", &Graphics::Material::doubleSided)
        .Field("vertexColor", &Graphics::Material::vertexColor);
    r.Register<Environment>("Environment")
        .Field("ambientSky", &Environment::ambientSky)
        .Field("ambientGround", &Environment::ambientGround)
        .Field("fog", &Environment::fog)
        .Field("fogColor", &Environment::fogColor)
        .Field("fogDensity", &Environment::fogDensity).Range(0.0, 1.0)
        .Field("exposure", &Environment::exposure).Range(0.0, 100.0)
        .Field("shadows", &Environment::shadows)
        .Field("shadowRadius", &Environment::shadowRadius).Range(1.0, 1.0e4)
        .Field("outlines", &Environment::outlines)
        .Field("bloom", &Environment::bloom);
    return r;
}

} // namespace

const Core::TypeRegistry& FrameworkTypes() {
    static const Core::TypeRegistry registry = BuildFrameworkTypes();
    return registry;
}

} // namespace Astral::Framework
