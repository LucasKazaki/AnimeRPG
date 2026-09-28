#pragma once

// Frame description handed to the renderer: a flat list of draw items plus
// lights, atmosphere and post settings. Gameplay/world code extracts this each
// frame (the "render proxy" split used by UE), so the renderer never touches
// game state and any backend (software today, GPU later) consumes the same data.

#include "Engine/Graphics/Material.h"
#include "Engine/Graphics/Mesh.h"
#include "Engine/Math/Geometry.h"

#include <cstdint>
#include <vector>

namespace Astral::Graphics {

struct RenderView {
    Math::Vec3 eye{};
    Math::Mat4 view = Math::Mat4::Identity();
    Math::Mat4 projection = Math::Mat4::Identity();
    float nearPlane{0.1f};
    float farPlane{500.0f};
    int width{1280};
    int height{720};

    static RenderView Perspective(Math::Vec3 eye, Math::Vec3 target, float verticalFovDegrees,
        int width, int height, float nearPlane = 0.25f, float farPlane = 600.0f) {
        RenderView v;
        v.eye = eye;
        v.width = width;
        v.height = height;
        v.nearPlane = nearPlane;
        v.farPlane = farPlane;
        v.view = Math::LookAtLH(eye, target, {0.0f, 1.0f, 0.0f});
        v.projection = Math::PerspectiveLH(Math::Radians(verticalFovDegrees),
            static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1), nearPlane, farPlane);
        return v;
    }
    Math::Mat4 ViewProjection() const { return projection * view; }
};

struct DrawItem {
    const MeshData* mesh{};
    const Material* material{};
    Math::Mat4 world = Math::Mat4::Identity();
    std::uint32_t objectId{};   // stable id for outlines/picking; 0 = anonymous
    Color tint{1.0f, 1.0f, 1.0f};
    float opacity{1.0f};        // multiplies material opacity (fades, afterimages)
    Color emissiveBoost{0.0f, 0.0f, 0.0f}; // per-instance highlight (selection, hit flash)
};

struct DirectionalLight {
    Math::Vec3 direction{0.35f, -0.8f, 0.45f}; // direction the light travels
    Color color{1.0f, 0.96f, 0.9f};
    float intensity{2.6f};
    bool castShadows{true};
};

struct PointLight {
    Math::Vec3 position{};
    Color color{1.0f, 1.0f, 1.0f};
    float intensity{1.0f};
    float radius{5.0f};
};

struct Billboard {
    Math::Vec3 position{};
    float size{0.2f};
    float rotation{};
    Math::Vec4 color{1, 1, 1, 1}; // linear rgb + alpha
    BlendMode blend{BlendMode::Additive};
    const Texture2D* texture{}; // null = soft round disc
    float stretch{1.0f};        // >1 elongates along velocity (sparks, speed lines)
    Math::Vec3 velocity{};
};

struct DebugLine {
    Math::Vec3 start{};
    Math::Vec3 end{};
    Rgba8 color{255, 255, 255, 255};
    bool depthTest{true};
};

struct SkySettings {
    Color zenith{0.20f, 0.36f, 0.78f};
    Color horizon{0.78f, 0.84f, 0.95f};
    Color ground{0.32f, 0.30f, 0.30f};
    Color sunColor{6.0f, 5.4f, 4.6f};
    float sunSizeDegrees{1.8f};
    float cloudRotation{}; // radians about +Y; animate for drifting clouds at no cost
};

struct FogSettings {
    bool enabled{true};
    Color color{0.72f, 0.80f, 0.93f};
    float density{0.006f};
    float heightFalloff{0.08f};
    float maxOpacity{0.85f};
};

struct ShadowSettings {
    bool enabled{true};
    int resolution{2048};
    Math::Vec3 focus{};     // world point the shadow map is centred on
    float radius{60.0f};    // half extent of the covered area
    float depthBias{0.0015f};
    float normalBias{0.04f};
    float softness{1.0f};   // PCF kernel radius in texels
};

struct PostSettings {
    float exposure{1.0f};
    bool bloom{true};
    float bloomThreshold{1.1f};
    float bloomIntensity{0.35f};
    bool outlines{true};
    float outlineDepthThreshold{0.035f}; // relative linear-depth jump
    float outlineNormalThreshold{0.6f};  // 1 - cos(angle)
    float outlineDarkness{0.28f};        // line colour = pixel colour * darkness
    Color outlineTint{0.18f, 0.12f, 0.28f};
    float saturation{1.05f};
    float contrast{1.05f};
    Color tint{1.0f, 1.0f, 1.0f};
    float vignette{0.18f};
    float chromaticAberration{0.0f}; // pixels at the frame edge
    bool fxaa{true};
    bool dither{true};
};

struct RenderScene {
    std::vector<DrawItem> draws;
    std::vector<Billboard> billboards;
    std::vector<DebugLine> lines;
    DirectionalLight sun;
    std::vector<PointLight> pointLights;
    Color ambientSky{0.30f, 0.34f, 0.45f};
    Color ambientGround{0.18f, 0.16f, 0.15f};
    SkySettings sky;
    FogSettings fog;
    ShadowSettings shadows;
    PostSettings post;
    float time{}; // seconds, drives water animation
};

} // namespace Astral::Graphics
