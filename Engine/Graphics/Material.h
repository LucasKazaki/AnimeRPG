#pragma once

// Surface description consumed by the scene renderer. Shading models mirror the
// role of UE's material shading models: Toon is the anime cel model (banded
// light, tinted shade colour, rim light, stylised highlight, ink outline),
// Lit is a GGX/Lambert physically based model, Unlit ignores lighting, and
// Water adds animated normals plus screen-space reflections.

#include "Engine/Graphics/Color.h"
#include "Engine/Graphics/Texture.h"

namespace Astral::Graphics {

enum class ShadingModel : std::uint8_t { Toon, Lit, Unlit, Water };
enum class BlendMode : std::uint8_t { Opaque, AlphaBlend, Additive };

struct Material {
    std::string name;
    ShadingModel shading{ShadingModel::Toon};
    BlendMode blend{BlendMode::Opaque};

    Color baseColor{0.8f, 0.8f, 0.8f};
    float opacity{1.0f};
    const Texture2D* baseTexture{};
    Math::Vec2 uvScale{1.0f, 1.0f};
    Color emissive{0.0f, 0.0f, 0.0f}; // linear HDR, added after lighting

    // Toon (cel) parameters.
    Color shadeColor{0.62f, 0.58f, 0.78f}; // multiplies albedo on the unlit side
    float shadeThreshold{0.15f};           // N.L where light turns to shade
    float shadeSoftness{0.04f};            // width of the terminator
    float specularThreshold{0.97f};        // N.H above which a hard highlight appears
    float specularIntensity{0.0f};
    Color rimColor{1.0f, 1.0f, 1.0f};
    float rimIntensity{0.0f};
    float rimWidth{0.35f};

    // Lit (PBR) parameters.
    float roughness{0.6f};
    float metallic{0.0f};

    // Water parameters.
    float reflectivity{0.0f};  // SSR contribution at normal incidence (Fresnel F0)
    float waveAmplitude{0.0f}; // normal perturbation strength
    float waveScale{1.0f};

    bool castShadows{true};
    bool receiveShadows{true};
    bool outline{true};
    bool doubleSided{false};
    bool vertexColor{true}; // multiply albedo by the vertex colour
};

} // namespace Astral::Graphics
