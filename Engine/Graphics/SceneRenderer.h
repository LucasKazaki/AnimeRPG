#pragma once

// Software scene renderer (the reference backend of the Astral render path).
//
// Frame graph:
//   1. Shadow pass   - directional shadow map, texel-snapped, depth only.
//   2. Geometry      - per-draw culling, vertex transform, clip, setup (parallel per draw).
//   3. Visibility    - tile-parallel depth + triangle-id raster (no shading).
//   4. Shading       - one shade per visible pixel: Toon / Lit / Unlit / Water, sky, fog.
//   5. Reflections   - screen-space reflections for water pixels, sky fallback.
//   6. Translucency  - sorted transparent meshes and soft billboards (tile-parallel).
//   7. Post          - ink outlines, bloom, tone map, grading, FXAA, vignette, dither.
//   8. Debug lines   - depth-tested wire overlay (keeps the wireframe debug view).
//
// A GPU backend is expected to consume the same RenderScene/RenderView and be
// validated against this renderer's images.

#include "Engine/Core/JobSystem.h"
#include "Engine/Graphics/PostProcess.h"
#include "Engine/Graphics/Rasterizer.h"
#include "Engine/Graphics/RenderScene.h"
#include "Engine/Graphics/RenderTarget.h"

#include <vector>

namespace Astral::Graphics {

struct RenderStats {
    std::size_t drawsSubmitted{};
    std::size_t drawsVisible{};
    std::size_t trianglesSubmitted{};
    std::size_t trianglesRasterized{};
    std::size_t shadowTriangles{};
    std::size_t transparentTriangles{};
    std::size_t pixelsShaded{};
    std::size_t reflectionPixels{};
    double shadowMs{}, geometryMs{}, rasterMs{}, shadeMs{}, reflectionMs{}, translucencyMs{}, postMs{}, totalMs{};
    std::size_t memoryBytes{};
};

// Scalability knobs (the role of UE's scalability groups). They cap or disable
// scene-authored features; they never change gameplay or scene content.
struct RendererSettings {
    int tileSize{64};
    int reflectionSteps{48};
    float reflectionMaxDistance{160.0f};
    bool clouds{true};
    bool skyLut{true};       // cache the cloud sky in an equirect LUT (rebuilt on change)
    int skyLutWidth{1024};
    int maxShadowResolution{4096};
    int shadowFilter{2};     // 0 = hard, 1 = 2x2 bilinear, 2 = 4x4 tent PCF
    bool reflections{true};
    bool bloom{true};
    bool fxaa{true};
    bool outlines{true};

    // 0 = Low, 1 = Medium, 2 = High, 3 = Epic.
    static RendererSettings Preset(int level) {
        RendererSettings s;
        switch (level <= 0 ? 0 : (level >= 3 ? 3 : level)) {
        case 0:
            s.reflectionSteps = 12; s.maxShadowResolution = 1024; s.shadowFilter = 0;
            s.bloom = false; s.fxaa = false; s.clouds = false; s.skyLutWidth = 512;
            break;
        case 1:
            s.reflectionSteps = 24; s.maxShadowResolution = 1536; s.shadowFilter = 1; s.skyLutWidth = 768;
            break;
        case 2:
            s.reflectionSteps = 40; s.maxShadowResolution = 2048; s.shadowFilter = 2;
            break;
        default:
            s.reflectionSteps = 64; s.maxShadowResolution = 4096; s.shadowFilter = 2; s.skyLutWidth = 2048;
            break;
        }
        return s;
    }
};

class SceneRenderer {
public:
    explicit SceneRenderer(Core::JobSystem* jobs = nullptr) : jobs_(jobs) {}

    void Render(const RenderScene& scene, const RenderView& view, RenderTarget& target);
    const RenderStats& Stats() const { return stats_; }
    RendererSettings& Settings() { return settings_; }

    // Exposed for tests and tools.
    Color SkyColor(const RenderScene& scene, Math::Vec3 direction) const;
    static Color SkyBase(const SkySettings& sky, Math::Vec3 direction, bool clouds);
    float ShadowFactor(Math::Vec3 worldPosition, Math::Vec3 normal) const;

private:
    struct ShadowMap {
        int size{};
        std::vector<float> depth;
        Math::Mat4 viewProjection = Math::Mat4::Identity();
        float texelWorld{};
        float depthRange{1.0f};
        float depthBias{};
        float normalBias{};
        bool valid{};
    };
    struct TranslucentItem {
        const DrawItem* draw{};
        const Billboard* billboard{};
        float sortDepth{};
    };

    void ParallelFor(std::size_t count, std::size_t batch, const std::function<void(std::size_t, std::size_t)>& fn);
    void RenderShadowMap(const RenderScene& scene);
    void BuildTriangles(const RenderScene& scene, const std::vector<std::uint32_t>& drawIndices,
        const Math::Mat4& viewProjection, int width, int height, bool shadowPass, std::vector<SetupTriangle>& out);
    void RasterizeVisibility(RenderTarget& target);
    void Shade(const RenderScene& scene, const RenderView& view, RenderTarget& target);
    void Reflections(const RenderScene& scene, const RenderView& view, RenderTarget& target);
    void Translucency(const RenderScene& scene, const RenderView& view, RenderTarget& target);
    void DrawDebugLines(const RenderScene& scene, const RenderView& view, RenderTarget& target);
    void EnsureSkyLut(const RenderScene& scene);
    void PrepareOcclusionFade(const RenderScene& scene, const RenderView& view);
    // 1 = fully visible; smaller values are dithered out in the opaque pass.
    float OcclusionCoverage(Math::Vec3 world) const;
    Color SampleSkyLut(Math::Vec3 direction, float rotation) const;

    Core::JobSystem* jobs_{};
    RendererSettings settings_;
    RenderStats stats_;
    ShadowMap shadow_;
    PostProcessor post_;
    std::vector<SetupTriangle> triangles_;
    std::vector<SetupTriangle> translucentTriangles_;
    std::vector<TranslucentItem> translucentItems_;
    std::vector<std::vector<SetupTriangle>> perDraw_;
    std::vector<std::uint32_t> drawList_;
    std::vector<std::uint8_t> fadeDraws_;
    Math::Mat4 inverseViewProjection_ = Math::Mat4::Identity();
    Math::Vec3 fadeEye_{}, fadeFocus_{};
    float fadeRadius_{};
    bool fadeActive_{};
    TileBins bins_;
    std::vector<Color> reflectionColor_;
    std::vector<float> reflectionWeight_;
    std::vector<Color> skyLut_;
    int skyLutWidth_{};
    int skyLutHeight_{};
    std::uint64_t skyLutKey_{};
    // View basis cached per frame.
    Math::Vec3 right_{}, up_{}, forward_{};
    float tanHalfX_{1.0f}, tanHalfY_{1.0f};
};

} // namespace Astral::Graphics
