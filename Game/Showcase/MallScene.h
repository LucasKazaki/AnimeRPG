#pragma once

// The supernatural National Mall as an engine scene: detailed procedural
// landmarks placed on the gameplay blockout's footprints (so interaction
// ranges and objectives are unchanged), physical colliders, destructible mana
// crystals, ambient mana VFX and the Mana Reactor rift. Presentation only:
// gameplay state stays in Engine/Scene.

#include "Engine/Graphics/ProceduralTextures.h"
#include "Engine/Graphics/RenderScene.h"
#include "Engine/Physics/Destruction.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Scene/WorldBlockout.h"
#include "Engine/VFX/Particles.h"

#include <memory>
#include <vector>

namespace Astral::Showcase {

namespace ObjectIds {
constexpr std::uint32_t Ground = 1, Paths = 2, Lincoln = 10, Pool = 20, PoolRim = 21, Trees = 22, Monument = 30,
    Flags = 31, Capitol = 40, Props = 50, Crystal = 60, Rift = 70, Player = 100, Dummy = 200;
}

namespace CollisionLayers {
constexpr std::uint32_t World = 1u << 0; // blocks characters and the camera spring arm
constexpr std::uint32_t Prop = 1u << 1;  // thin props: block characters; the camera fades them instead
}

struct StaticDraw {
    const Graphics::MeshData* mesh;
    const Graphics::Material* material;
    Math::Mat4 world;
    std::uint32_t objectId;
};

struct Crystal {
    std::unique_ptr<Physics::Destructible> destructible;
    Math::Vec3 position;
    float respawnTimer{};
};

class MallScene {
public:
    MallScene(const Scene::WorldBlockout& blockout, Physics::PhysicsWorld& physics);

    // Adds static and dynamic geometry (crystals and their debris) to the frame.
    void AppendDraws(Graphics::RenderScene& scene) const;
    void Update(float dt, float time);
    // Breaks crystals within `radius` of `point` (cosmetic destruction). Returns count.
    int ShatterCrystals(Math::Vec3 point, float radius, Math::Vec3 direction, float damage);
    VFX::ParticleWorld& Particles() { return particles_; }
    const Scene::WorldBlockout& Blockout() const { return blockout_; }
    Math::Vec3 RiftPosition() const { return riftPosition_; }
    const std::vector<Crystal>& Crystals() const { return crystals_; }

private:
    void BuildTextures();
    void BuildMaterials();
    void BuildGround();
    void BuildLincoln(const Scene::LandmarkProxy& proxy);
    void BuildPool(const Scene::LandmarkProxy& proxy);
    void BuildMonument(const Scene::LandmarkProxy& proxy);
    void BuildBackdrop();
    void BuildTrainingGround();
    void AddCollider(Math::Vec3 center, Math::Vec3 half, Math::Quat rotation = {},
        std::uint32_t layer = CollisionLayers::World);
    Graphics::MeshData& NewMesh();

    const Scene::WorldBlockout& blockout_;
    Physics::PhysicsWorld& physics_;
    VFX::ParticleWorld particles_;
    std::vector<std::unique_ptr<Graphics::MeshData>> meshes_;
    std::vector<StaticDraw> draws_;
    std::vector<Crystal> crystals_;
    Graphics::MeshData crystalMesh_;
    Graphics::MeshData debrisMesh_;
    Graphics::MeshData riftRing_;
    Graphics::MeshData riftCore_;
    Math::Vec3 riftPosition_{};
    float time_{};

    Graphics::Texture2D grassTexture_, marbleTexture_, pavingTexture_, gravelTexture_, stoneTexture_;
    Graphics::Material grass_, gravel_, paving_, marble_, marbleShade_, monumentLower_, monumentUpper_, water_,
        granite_, bark_, leaves_, metal_, lampGlow_, crystal_, crystalDebris_, riftRing_m_, riftCore_m_, capitol_,
        runeGlow_, flagRed_, flagWhite_, flagBlue_, bench_, flagPole_;
};

} // namespace Astral::Showcase
