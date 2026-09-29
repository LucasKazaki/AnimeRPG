#include "Game/Showcase/MallScene.h"

#include <cmath>

namespace Astral::Showcase {

using namespace Math;
using Graphics::Color;
using Graphics::FromSrgb8;
using Graphics::Material;
using Graphics::MeshBuilder;
using Graphics::MeshData;

namespace {
Material Toon(Color base, const Graphics::Texture2D* texture = nullptr, float uvScale = 1.0f) {
    Material m;
    m.baseColor = base;
    m.baseTexture = texture;
    m.uvScale = {uvScale, uvScale};
    m.shadeThreshold = 0.05f;
    m.shadeSoftness = 0.08f;
    return m;
}
} // namespace

MeshData& MallScene::NewMesh() {
    meshes_.push_back(std::make_unique<MeshData>());
    return *meshes_.back();
}

void MallScene::AddCollider(Vec3 center, Vec3 half, Quat rotation, std::uint32_t layer) {
    Physics::BodyDesc desc;
    desc.type = Physics::BodyType::Static;
    desc.layer = layer;
    desc.shape = Physics::Shape::Box(half);
    desc.position = center;
    desc.rotation = rotation;
    physics_.CreateBody(desc);
}

void MallScene::BuildTextures() {
    grassTexture_ = Graphics::Procedural::Grass(256, FromSrgb8(122, 176, 84), FromSrgb8(74, 128, 58), 7);
    marbleTexture_ = Graphics::Procedural::Marble(256, FromSrgb8(238, 236, 228), FromSrgb8(170, 168, 176), 3);
    pavingTexture_ = Graphics::Procedural::Paving(256, 4, 8, FromSrgb8(206, 198, 184), FromSrgb8(140, 132, 122), 5);
    gravelTexture_ = Graphics::Procedural::Gravel(128, FromSrgb8(214, 196, 160), 9);
    stoneTexture_ = Graphics::Procedural::Marble(128, FromSrgb8(120, 118, 124), FromSrgb8(84, 82, 90), 11);
}

void MallScene::BuildMaterials() {
    grass_ = Toon({1, 1, 1}, &grassTexture_, 0.25f);
    grass_.outline = false;
    grass_.shadeColor = {0.55f, 0.62f, 0.8f};
    gravel_ = Toon({1, 1, 1}, &gravelTexture_, 0.35f);
    gravel_.outline = false;
    paving_ = Toon({1, 1, 1}, &pavingTexture_, 0.25f);
    paving_.outline = false;
    marble_ = Toon({1, 1, 1}, &marbleTexture_, 0.35f);
    marble_.shadeColor = {0.66f, 0.66f, 0.84f};
    marble_.rimIntensity = 0.08f;
    marble_.rimColor = {1.0f, 0.9f, 0.8f};
    marbleShade_ = marble_;
    marbleShade_.baseColor = {0.78f, 0.77f, 0.8f};
    monumentLower_ = marble_;
    monumentUpper_ = marble_;
    monumentUpper_.baseColor = {0.93f, 0.9f, 0.84f}; // the real shaft changes colour one third up
    water_.shading = Graphics::ShadingModel::Water;
    water_.baseColor = FromSrgb8(22, 52, 70);
    water_.reflectivity = 0.42f;   // the Reflecting Pool is a calm mirror
    water_.waveAmplitude = 0.018f;
    water_.waveScale = 0.7f;
    water_.specularIntensity = 0.0f;
    water_.outline = false;
    water_.castShadows = false;
    granite_ = Toon({1, 1, 1}, &stoneTexture_, 0.5f);
    bark_ = Toon(FromSrgb8(96, 72, 60));
    leaves_ = Toon(FromSrgb8(86, 150, 78));
    leaves_.shadeColor = {0.45f, 0.58f, 0.72f};
    leaves_.rimIntensity = 0.12f;
    leaves_.rimColor = {0.9f, 1.0f, 0.6f};
    metal_ = Toon(FromSrgb8(40, 44, 52));
    metal_.specularIntensity = 0.4f;
    lampGlow_.shading = Graphics::ShadingModel::Unlit;
    lampGlow_.baseColor = {1.0f, 0.85f, 0.6f};
    lampGlow_.emissive = {1.6f, 1.2f, 0.6f};
    lampGlow_.outline = false;
    crystal_ = Toon(FromSrgb8(120, 220, 255));
    crystal_.emissive = {0.25f, 0.9f, 1.3f};
    crystal_.rimIntensity = 0.8f;
    crystal_.rimColor = {0.8f, 1.0f, 1.0f};
    crystal_.specularIntensity = 1.0f;
    crystal_.specularThreshold = 0.92f;
    crystalDebris_ = crystal_;
    crystalDebris_.emissive = {0.1f, 0.5f, 0.8f};
    riftRing_m_ = Toon(FromSrgb8(40, 30, 70));
    riftRing_m_.emissive = {1.2f, 0.4f, 2.2f};
    riftRing_m_.rimIntensity = 0.6f;
    riftRing_m_.rimColor = {1.0f, 0.6f, 1.0f};
    riftCore_m_.shading = Graphics::ShadingModel::Unlit;
    riftCore_m_.blend = Graphics::BlendMode::Additive;
    riftCore_m_.baseColor = {0.6f, 0.25f, 1.2f};
    riftCore_m_.opacity = 0.65f;
    riftCore_m_.rimIntensity = 1.5f;
    riftCore_m_.rimColor = {0.4f, 1.2f, 2.0f};
    riftCore_m_.doubleSided = true;
    riftCore_m_.outline = false;
    capitol_ = marble_;
    capitol_.baseColor = {0.96f, 0.95f, 0.93f};
    runeGlow_.shading = Graphics::ShadingModel::Unlit;
    runeGlow_.baseColor = {0.5f, 0.3f, 1.0f};
    runeGlow_.emissive = {0.9f, 0.4f, 2.4f};
    runeGlow_.outline = false;
    flagRed_ = Toon(FromSrgb8(190, 40, 50));
    flagWhite_ = Toon(FromSrgb8(240, 240, 240));
    flagBlue_ = Toon(FromSrgb8(40, 60, 140));
    bench_ = Toon(FromSrgb8(92, 70, 52));
    flagPole_ = Toon(FromSrgb8(150, 150, 158));
    flagPole_.outline = false; // hairline poles read better without ink
    flagPole_.specularIntensity = 0.5f;
    // Props that may sit between the camera and the player dither out of the way.
    for (Graphics::Material* m : {&bark_, &leaves_, &metal_, &lampGlow_, &flagPole_, &flagRed_, &flagWhite_, &flagBlue_, &bench_, &crystal_}) {
        m->cameraFade = true;
    }
}

void MallScene::BuildGround() {
    MeshData& lawn = NewMesh();
    MeshBuilder(lawn).AddPlane({0.0f, 0.0f, 60.0f}, {90.0f, 110.0f}, 8, {45.0f, 55.0f});
    lawn.ComputeBounds();
    draws_.push_back({&lawn, &grass_, Mat4::Identity(), ObjectIds::Ground});

    // Gravel walks flanking the Mall and paved cross walks.
    MeshData& gravel = NewMesh();
    MeshBuilder g(gravel);
    for (float x : {-15.0f, 15.0f}) g.AddPlane({x, 0.015f, 62.0f}, {1.8f, 84.0f}, 1, {1.0f, 45.0f});
    for (float z : {-2.0f, 26.0f, 56.0f}) g.AddPlane({0.0f, 0.016f, z}, {16.8f, 1.4f}, 1, {9.0f, 1.0f});
    gravel.ComputeBounds();
    draws_.push_back({&gravel, &gravel_, Mat4::Identity(), ObjectIds::Paths});

    // Lamp posts and benches along the walks.
    MeshData& lampPoles = NewMesh();
    MeshData& lampGlobes = NewMesh();
    MeshBuilder poles(lampPoles), globes(lampGlobes);
    for (float x : {-13.0f, 13.0f}) {
        for (float z = 4.0f; z <= 140.0f; z += 12.0f) {
            poles.AddCylinder({x, 0.0f, z}, 0.07f, 3.2f, 8, true);
            poles.AddBox({x, 3.25f, z}, {0.14f, 0.05f, 0.14f});
            globes.AddSphere({x, 3.5f, z}, 0.2f, 6, 8);
            AddCollider({x, 1.6f, z}, {0.08f, 1.6f, 0.08f}, {}, CollisionLayers::Prop);
        }
    }
    lampPoles.ComputeBounds();
    lampGlobes.ComputeBounds();
    draws_.push_back({&lampPoles, &metal_, Mat4::Identity(), ObjectIds::Props});
    draws_.push_back({&lampGlobes, &lampGlow_, Mat4::Identity(), ObjectIds::Props + 1});

    MeshData& benches = NewMesh();
    MeshBuilder b(benches);
    for (float x : {-11.8f, 11.8f}) {
        for (float z : {10.0f, 34.0f, 46.0f, 62.0f}) {
            b.AddBox({x, 0.45f, z}, {0.28f, 0.05f, 0.9f});
            b.AddBox({x + (x < 0 ? -0.24f : 0.24f), 0.75f, z}, {0.04f, 0.25f, 0.9f});
            for (float dz : {-0.75f, 0.75f}) b.AddBox({x, 0.2f, z + dz}, {0.22f, 0.2f, 0.05f});
            AddCollider({x, 0.45f, z}, {0.3f, 0.45f, 0.9f}, {}, CollisionLayers::Prop);
        }
    }
    benches.ComputeBounds();
    draws_.push_back({&benches, &bench_, Mat4::Identity(), ObjectIds::Props + 2});
}

void MallScene::BuildLincoln(const Scene::LandmarkProxy& proxy) {
    const Vec3 p = proxy.position;
    const float hx = proxy.dimensions.x * 0.5f, hz = proxy.dimensions.z * 0.5f; // 6 x 3.5
    MeshData& mesh = NewMesh();
    MeshBuilder m(mesh);
    // Three-tier stylobate.
    const float tiers[3][3] = {{0.2f, hx + 0.6f, hz + 0.6f}, {0.6f, hx + 0.3f, hz + 0.3f}, {1.0f, hx, hz}};
    for (const auto& t : tiers) {
        m.AddBox({p.x, t[0], p.z}, {t[1], 0.2f, t[2]});
        AddCollider({p.x, t[0], p.z}, {t[1], 0.2f, t[2]});
    }
    // Grand stairs facing the Reflecting Pool (+z).
    for (int k = 1; k <= 6; ++k) {
        const float height = 0.2f * static_cast<float>(k);
        const float z0 = p.z + hz + 0.4f * static_cast<float>(6 - k);
        m.AddBox({p.x, height * 0.5f, z0 + 0.2f}, {3.4f, height * 0.5f, 0.2f});
        AddCollider({p.x, height * 0.5f, z0 + 0.2f}, {3.4f, height * 0.5f, 0.2f});
    }
    const float floor = 1.2f, columnHeight = 3.0f;
    // Peristyle colonnade.
    std::vector<Vec2> columns;
    for (int i = 0; i < 9; ++i) {
        const float x = -hx + 0.5f + (2.0f * hx - 1.0f) * static_cast<float>(i) / 8.0f;
        columns.push_back({x, -hz + 0.5f});
        columns.push_back({x, hz - 0.5f});
    }
    for (float z : {-1.25f, 0.0f, 1.25f}) {
        columns.push_back({-hx + 0.5f, z});
        columns.push_back({hx - 0.5f, z});
    }
    for (const Vec2& c : columns) {
        const Vec3 base{p.x + c.x, floor, p.z + c.y};
        m.AddBox(base + Vec3{0, 0.06f, 0}, {0.34f, 0.06f, 0.34f});
        m.AddCylinder(base + Vec3{0, 0.12f, 0}, 0.27f, columnHeight - 0.26f, 14, false, 0.24f);
        m.AddBox(base + Vec3{0, columnHeight - 0.07f, 0}, {0.34f, 0.07f, 0.34f});
        AddCollider(base + Vec3{0, columnHeight * 0.5f, 0}, {0.27f, columnHeight * 0.5f, 0.27f});
    }
    // Entablature, frieze and attic.
    const float top = floor + columnHeight;
    m.AddBox({p.x, top + 0.2f, p.z}, {hx + 0.1f, 0.2f, hz + 0.1f});
    m.AddBox({p.x, top + 0.55f, p.z}, {hx + 0.05f, 0.15f, hz + 0.05f});
    m.AddBox({p.x, top + 0.8f, p.z}, {hx + 0.2f, 0.1f, hz + 0.2f});
    m.AddBox({p.x, top + 1.25f, p.z}, {hx - 0.7f, 0.35f, hz - 0.6f});
    m.AddBox({p.x, top + 1.65f, p.z}, {hx - 0.6f, 0.05f, hz - 0.5f});
    mesh.ComputeBounds();
    draws_.push_back({&mesh, &marble_, Mat4::Identity(), ObjectIds::Lincoln});

    // Cella walls (open to the east/+z) and the seated statue.
    MeshData& cella = NewMesh();
    MeshBuilder c(cella);
    c.AddBox({p.x, floor + 1.5f, p.z - 2.0f}, {4.2f, 1.5f, 0.15f});
    c.AddBox({p.x - 4.2f, floor + 1.5f, p.z}, {0.15f, 1.5f, 2.0f});
    c.AddBox({p.x + 4.2f, floor + 1.5f, p.z}, {0.15f, 1.5f, 2.0f});
    AddCollider({p.x, floor + 1.5f, p.z - 2.0f}, {4.2f, 1.5f, 0.15f});
    AddCollider({p.x - 4.2f, floor + 1.5f, p.z}, {0.15f, 1.5f, 2.0f});
    AddCollider({p.x + 4.2f, floor + 1.5f, p.z}, {0.15f, 1.5f, 2.0f});
    const Vec3 statue{p.x, floor, p.z - 1.1f};
    c.AddBox(statue + Vec3{0, 0.35f, 0}, {0.9f, 0.35f, 0.6f});          // pedestal
    c.AddBox(statue + Vec3{0, 1.0f, -0.1f}, {0.6f, 0.3f, 0.45f});       // chair seat
    c.AddBox(statue + Vec3{0, 1.6f, -0.45f}, {0.62f, 0.6f, 0.1f});      // chair back
    c.AddTaperedBox(statue + Vec3{0, 1.3f, -0.1f}, {0.38f, 0.25f}, {0.3f, 0.2f}, 0.8f); // torso
    c.AddSphere(statue + Vec3{0, 2.25f, -0.05f}, 0.17f, 8, 10);
    c.AddBox(statue + Vec3{-0.2f, 1.05f, 0.35f}, {0.12f, 0.1f, 0.35f}); // knees
    c.AddBox(statue + Vec3{0.2f, 1.05f, 0.35f}, {0.12f, 0.1f, 0.35f});
    AddCollider(statue + Vec3{0, 1.0f, -0.1f}, {0.9f, 1.0f, 0.6f});
    cella.ComputeBounds();
    draws_.push_back({&cella, &marbleShade_, Mat4::Identity(), ObjectIds::Lincoln + 1});
}

void MallScene::BuildPool(const Scene::LandmarkProxy& proxy) {
    const Vec3 p = proxy.position;
    const float hx = proxy.dimensions.x * 0.5f, hz = proxy.dimensions.z * 0.5f; // 5 x 10
    const float rim = proxy.dimensions.y; // 0.35
    MeshData& coping = NewMesh();
    MeshBuilder c(coping);
    const Vec3 sides[4][2] = {{{p.x - hx + 0.25f, rim * 0.5f, p.z}, {0.25f, rim * 0.5f, hz}},
        {{p.x + hx - 0.25f, rim * 0.5f, p.z}, {0.25f, rim * 0.5f, hz}},
        {{p.x, rim * 0.5f, p.z - hz + 0.25f}, {hx - 0.5f, rim * 0.5f, 0.25f}},
        {{p.x, rim * 0.5f, p.z + hz - 0.25f}, {hx - 0.5f, rim * 0.5f, 0.25f}}};
    for (const auto& side : sides) {
        c.AddBox(side[0], side[1]);
        AddCollider(side[0], side[1]);
    }
    coping.ComputeBounds();
    draws_.push_back({&coping, &granite_, Mat4::Identity(), ObjectIds::PoolRim});
    MeshData& water = NewMesh();
    MeshBuilder(water).AddPlane({p.x, rim - 0.07f, p.z}, {hx - 0.5f, hz - 0.5f}, 4);
    water.ComputeBounds();
    draws_.push_back({&water, &water_, Mat4::Identity(), ObjectIds::Pool});
    // The supernatural Mall: the pool surface bears weight.
    AddCollider({p.x, (rim - 0.07f) * 0.5f, p.z}, {hx - 0.5f, (rim - 0.07f) * 0.5f, hz - 0.5f});

    // Elm rows flanking the pool.
    MeshData& trunks = NewMesh();
    MeshData& canopies = NewMesh();
    MeshBuilder t(trunks), l(canopies);
    for (float x : {p.x - hx - 2.2f, p.x + hx + 2.2f}) {
        for (float z = p.z - hz + 1.0f; z <= p.z + hz - 1.0f; z += 3.6f) {
            t.AddCylinder({x, 0.0f, z}, 0.2f, 2.6f, 8, false, 0.14f);
            l.SetTransform(Translation({x, 3.6f, z}) * Scaling({1.5f, 1.25f, 1.5f}));
            l.AddSphere({0, 0, 0}, 1.0f, 7, 9);
            l.SetTransform(Translation({x + 0.6f, 3.0f, z + 0.3f}) * Scaling({1.1f, 0.9f, 1.1f}));
            l.AddSphere({0, 0, 0}, 1.0f, 6, 8);
            l.SetTransform(Translation({x - 0.5f, 3.1f, z - 0.4f}) * Scaling({1.0f, 0.85f, 1.0f}));
            l.AddSphere({0, 0, 0}, 1.0f, 6, 8);
            AddCollider({x, 1.3f, z}, {0.22f, 1.3f, 0.22f}, {}, CollisionLayers::Prop);
        }
    }
    trunks.ComputeBounds();
    canopies.ComputeBounds();
    draws_.push_back({&trunks, &bark_, Mat4::Identity(), ObjectIds::Trees});
    draws_.push_back({&canopies, &leaves_, Mat4::Identity(), ObjectIds::Trees + 1});

    // Mana Reactor rift hovering over the pool: reflected by the water (SSR).
    riftPosition_ = {p.x, 3.0f, p.z + 2.0f};
    MeshBuilder ring(riftRing_);
    ring.SetTransform(Translation(riftPosition_) * RotationX(kHalfPi));
    ring.AddTorus({0, 0, 0}, 1.7f, 0.13f, 40, 10);
    for (int i = 0; i < 8; ++i) {
        const float a = kTwoPi * static_cast<float>(i) / 8.0f;
        ring.AddBox({std::cos(a) * 1.95f, 0.0f, std::sin(a) * 1.95f}, {0.08f, 0.08f, 0.22f});
    }
    riftRing_.ComputeBounds();
    MeshBuilder core(riftCore_);
    core.SetTransform(Translation(riftPosition_) * Scaling({1.55f, 1.55f, 0.18f}));
    core.AddSphere({0, 0, 0}, 1.0f, 14, 24);
    riftCore_.ComputeBounds();
    VFX::EmitterSettings riftMotes = VFX::Presets::ManaMotes(1.4f);
    riftMotes.colors = {{0.0f, {1.2f, 0.4f, 2.6f, 0}}, {0.25f, {1.4f, 0.6f, 3.0f, 1}}, {1.0f, {0.4f, 1.2f, 2.4f, 0}}};
    riftMotes.rate = 24.0f;
    riftMotes.swirl = 1.2f;
    particles_.Spawn(riftMotes, riftPosition_ - Vec3{0, 1.2f, 0});
    particles_.Spawn(VFX::Presets::ManaMotes(hx), {p.x, 0.3f, p.z});
}

void MallScene::BuildMonument(const Scene::LandmarkProxy& proxy) {
    const Vec3 p = proxy.position;
    const float half = proxy.dimensions.x * 0.5f;       // 2.5
    const float height = proxy.dimensions.y;            // 22
    const float shoulder = height * 0.82f;
    const float topHalf = half * 0.68f;
    MeshData& plaza = NewMesh();
    MeshBuilder(plaza).AddCylinder({p.x, 0.0f, p.z}, 9.5f, 0.03f, 48, true);
    plaza.ComputeBounds();
    draws_.push_back({&plaza, &paving_, Mat4::Identity(), ObjectIds::Paths + 1});
    // Lower and upper shaft with the historical colour change one third up.
    const float change = height * 0.27f;
    const float changeHalf = Lerp(half, topHalf, change / shoulder);
    MeshData& lower = NewMesh();
    MeshBuilder(lower).AddTaperedBox({p.x, 0.0f, p.z}, {half, half}, {changeHalf, changeHalf}, change);
    lower.ComputeBounds();
    draws_.push_back({&lower, &monumentLower_, Mat4::Identity(), ObjectIds::Monument});
    MeshData& upper = NewMesh();
    MeshBuilder u(upper);
    u.AddTaperedBox({p.x, change, p.z}, {changeHalf, changeHalf}, {topHalf, topHalf}, shoulder - change);
    u.AddPyramid({p.x, shoulder, p.z}, {topHalf, topHalf}, height - shoulder);
    upper.ComputeBounds();
    draws_.push_back({&upper, &monumentUpper_, Mat4::Identity(), ObjectIds::Monument});
    AddCollider({p.x, height * 0.5f, p.z}, {half, height * 0.5f, half});
    // Ring of flags.
    MeshData& poles = NewMesh();
    MeshData& red = NewMesh();
    MeshData& white = NewMesh();
    MeshData& blue = NewMesh();
    MeshBuilder pb(poles), rb(red), wb(white), bb(blue);
    for (int i = 0; i < 50; ++i) {
        const float a = kTwoPi * static_cast<float>(i) / 50.0f;
        const Vec3 base{p.x + std::cos(a) * 8.6f, 0.0f, p.z + std::sin(a) * 8.6f};
        pb.AddCylinder(base, 0.04f, 5.2f, 6, true);
        const Mat4 flagFrame = Translation(base + Vec3{0, 4.6f, 0}) * RotationY(a + kHalfPi);
        rb.SetTransform(flagFrame);
        rb.AddBox({0.55f, 0.35f, 0.0f}, {0.5f, 0.12f, 0.01f});
        wb.SetTransform(flagFrame);
        wb.AddBox({0.55f, 0.15f, 0.0f}, {0.5f, 0.08f, 0.01f});
        bb.SetTransform(flagFrame);
        bb.AddBox({0.25f, 0.4f, 0.0f}, {0.2f, 0.08f, 0.015f});
        AddCollider(base + Vec3{0, 2.6f, 0}, {0.06f, 2.6f, 0.06f}, {}, CollisionLayers::Prop);
    }
    for (MeshData* m : {&poles, &red, &white, &blue}) m->ComputeBounds();
    draws_.push_back({&poles, &flagPole_, Mat4::Identity(), ObjectIds::Flags});
    draws_.push_back({&red, &flagRed_, Mat4::Identity(), ObjectIds::Flags + 1});
    draws_.push_back({&white, &flagWhite_, Mat4::Identity(), ObjectIds::Flags + 2});
    draws_.push_back({&blue, &flagBlue_, Mat4::Identity(), ObjectIds::Flags + 3});
    particles_.Spawn(VFX::Presets::ManaMotes(3.5f), {p.x, 0.5f, p.z});
}

void MallScene::BuildBackdrop() {
    // United States Capitol silhouette at the east end, softened by distance fog.
    MeshData& capitol = NewMesh();
    MeshBuilder c(capitol);
    const Vec3 base{5.0f, 0.0f, 158.0f};
    c.AddBox(base + Vec3{0, 3.0f, 0}, {22.0f, 3.0f, 6.0f});
    c.AddBox(base + Vec3{0, 4.0f, -4.0f}, {8.0f, 4.0f, 3.0f});
    for (int i = 0; i < 10; ++i) c.AddCylinder(base + Vec3{-5.4f + 1.2f * static_cast<float>(i), 6.0f, -6.5f}, 0.25f, 3.0f, 8);
    c.AddBox(base + Vec3{0, 9.3f, -5.0f}, {7.0f, 0.3f, 2.0f});
    c.AddPyramid(base + Vec3{0, 9.6f, -5.0f}, {7.0f, 2.0f}, 1.8f);
    c.AddCylinder(base + Vec3{0, 6.0f, 0}, 5.0f, 5.0f, 28, true);
    c.AddCylinder(base + Vec3{0, 11.0f, 0}, 4.6f, 1.2f, 28, true);
    c.SetTransform(Translation(base + Vec3{0, 12.2f, 0}) * Scaling({4.6f, 5.2f, 4.6f}));
    c.AddSphere({0, 0, 0}, 1.0f, 16, 28);
    c.SetTransform(Mat4::Identity());
    c.AddCylinder(base + Vec3{0, 17.2f, 0}, 0.9f, 2.0f, 12, true);
    c.AddSphere(base + Vec3{0, 19.6f, 0}, 0.6f, 6, 8);
    capitol.ComputeBounds();
    draws_.push_back({&capitol, &capitol_, Mat4::Identity(), ObjectIds::Capitol});
    // Distant tree lines along the Mall.
    MeshData& far = NewMesh();
    MeshBuilder f(far);
    for (float x : {-30.0f, 38.0f}) {
        for (float z = -10.0f; z <= 150.0f; z += 7.0f) {
            f.SetTransform(Translation({x + std::sin(z) * 2.0f, 3.2f, z}) * Scaling({3.2f, 2.8f, 3.2f}));
            f.AddSphere({0, 0, 0}, 1.0f, 5, 7);
        }
    }
    far.ComputeBounds();
    draws_.push_back({&far, &leaves_, Mat4::Identity(), ObjectIds::Trees + 2});
}

void MallScene::BuildTrainingGround() {
    // Paved training circle with a glowing mana rune ring around the dummy.
    MeshData& plaza = NewMesh();
    MeshBuilder(plaza).AddCylinder({1.5f, 0.0f, 0.0f}, 6.2f, 0.025f, 40, true);
    plaza.ComputeBounds();
    draws_.push_back({&plaza, &paving_, Mat4::Identity(), ObjectIds::Paths + 2});
    MeshData& rune = NewMesh();
    MeshBuilder r(rune);
    r.AddTorus({1.5f, 0.04f, 0.0f}, 5.8f, 0.05f, 64, 4);
    r.AddTorus({1.5f, 0.04f, 0.0f}, 5.3f, 0.025f, 64, 4);
    for (int i = 0; i < 12; ++i) {
        const float a = kTwoPi * static_cast<float>(i) / 12.0f;
        r.SetTransform(Translation({1.5f + std::cos(a) * 5.55f, 0.045f, std::sin(a) * 5.55f}) * RotationY(-a));
        r.AddBox({0, 0, 0}, {0.03f, 0.01f, 0.18f});
    }
    rune.ComputeBounds();
    draws_.push_back({&rune, &runeGlow_, Mat4::Identity(), ObjectIds::Paths + 3});

    // Destructible mana crystals.
    MeshBuilder cb(crystalMesh_);
    cb.AddCylinder({0, -0.8f, 0}, 0.32f, 1.25f, 6, true, 0.26f);
    cb.AddCylinder({0, 0.45f, 0}, 0.26f, 0.45f, 6, false, 0.0f);
    crystalMesh_.ComputeBounds();
    MeshBuilder(debrisMesh_).AddBox({0, 0, 0}, {1, 1, 1});
    debrisMesh_.ComputeBounds();
    Physics::DestructibleSettings settings;
    settings.health = 60.0f;
    settings.piecesX = 2;
    settings.piecesY = 4;
    settings.piecesZ = 2;
    settings.density = 250.0f;
    settings.breakImpulse = 5.0f;
    settings.debrisLifetime = 6.0f;
    std::uint32_t seed = 21;
    for (const Vec3& at : {Vec3{-5.0f, 0.8f, -1.5f}, Vec3{7.2f, 0.8f, 2.8f}, Vec3{-3.6f, 0.8f, 5.2f}}) {
        settings.seed = seed++;
        Crystal crystal;
        crystal.position = at;
        crystal.destructible = std::make_unique<Physics::Destructible>(physics_, Physics::Pose{at, {}}, Vec3{0.3f, 0.8f, 0.3f}, settings);
        crystals_.push_back(std::move(crystal));
        particles_.Spawn(VFX::Presets::ManaMotes(0.6f), at - Vec3{0, 0.8f, 0});
    }
}

MallScene::MallScene(const Scene::WorldBlockout& blockout, Physics::PhysicsWorld& physics)
    : blockout_(blockout), physics_(physics) {
    BuildTextures();
    BuildMaterials();
    // Ground collider (a thick slab so nothing tunnels through it).
    AddCollider({0.0f, -1.0f, 60.0f}, {90.0f, 1.0f, 110.0f});
    BuildGround();
    for (const Scene::LandmarkProxy& proxy : blockout.Landmarks()) {
        switch (proxy.kind) {
        case Scene::LandmarkKind::LincolnMemorial: BuildLincoln(proxy); break;
        case Scene::LandmarkKind::ReflectingPool: BuildPool(proxy); break;
        case Scene::LandmarkKind::WashingtonMonument: BuildMonument(proxy); break;
        }
    }
    BuildBackdrop();
    BuildTrainingGround();
}

void MallScene::AppendDraws(Graphics::RenderScene& scene) const {
    for (const StaticDraw& d : draws_) scene.draws.push_back({d.mesh, d.material, d.world, d.objectId});
    // Rift: slow spin and a breathing core.
    const Mat4 spin = Translation(riftPosition_) * RotationZ(time_ * 0.6f) * Translation(-riftPosition_);
    scene.draws.push_back({&riftRing_, &riftRing_m_, spin, ObjectIds::Rift});
    Graphics::DrawItem core{&riftCore_, &riftCore_m_, Mat4::Identity(), ObjectIds::Rift + 1};
    core.opacity = 0.75f + 0.25f * std::sin(time_ * 2.3f);
    scene.draws.push_back(core);
    for (std::size_t i = 0; i < crystals_.size(); ++i) {
        const Crystal& crystal = crystals_[i];
        const Physics::Destructible& d = *crystal.destructible;
        if (!d.Broken()) {
            Graphics::DrawItem item{&crystalMesh_, &crystal_, Translation(crystal.position) * RotationY(time_ * 0.4f + static_cast<float>(i)),
                ObjectIds::Crystal + static_cast<std::uint32_t>(i)};
            item.emissiveBoost = Vec3{0.2f, 0.5f, 0.8f} * (0.5f + 0.5f * std::sin(time_ * 3.0f + static_cast<float>(i)));
            scene.draws.push_back(item);
            continue;
        }
        const auto& pieces = d.Pieces();
        for (std::size_t k = 0; k < d.Debris().size() && k < pieces.size(); ++k) {
            const Physics::Body* body = physics_.GetBody(d.Debris()[k]);
            if (!body) continue;
            const Mat4 world = Translation(body->pose.position) * ToMat4(body->pose.rotation) * Scaling(pieces[k].halfExtents * 0.98f);
            scene.draws.push_back({&debrisMesh_, &crystalDebris_, world, ObjectIds::Crystal + 10 + static_cast<std::uint32_t>(k)});
        }
    }
    particles_.Collect(scene.billboards);
}

void MallScene::Update(float dt, float time) {
    time_ = time;
    particles_.Update(dt);
    for (Crystal& crystal : crystals_) {
        crystal.destructible->Update(dt);
        if (crystal.destructible->Broken()) {
            crystal.respawnTimer += dt;
            if (crystal.respawnTimer > 10.0f) {
                crystal.destructible->Reset();
                crystal.respawnTimer = 0.0f;
                particles_.Spawn(VFX::Presets::HitBurst(), crystal.position);
            }
        }
    }
}

int MallScene::ShatterCrystals(Vec3 point, float radius, Vec3 direction, float damage) {
    int broken = 0;
    for (Crystal& crystal : crystals_) {
        if (crystal.destructible->Broken()) continue;
        if (DistanceSquared(Horizontal(crystal.position), Horizontal(point)) > radius * radius) continue;
        if (crystal.destructible->ApplyDamage(damage, crystal.position - Normalize(direction, {0, 0, 1}) * 0.3f, direction)) {
            ++broken;
            crystal.respawnTimer = 0.0f;
            VFX::EmitterSettings shards = VFX::Presets::SlashSparks(Vec3{0, 1, 0});
            shards.colors = {{0.0f, {1.0f, 3.0f, 4.0f, 1}}, {1.0f, {0.2f, 0.8f, 1.6f, 0}}};
            shards.spreadDegrees = 80.0f;
            particles_.Spawn(shards, crystal.position);
            particles_.Spawn(VFX::Presets::DebrisDust(), crystal.position - Vec3{0, 0.6f, 0});
        }
    }
    return broken;
}

} // namespace Astral::Showcase
