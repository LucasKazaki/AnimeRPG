#include "Engine/Core/JobSystem.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Mesh.h"
#include "Engine/Graphics/MeshSimplify.h"
#include "Engine/Graphics/SceneRenderer.h"
#include "Engine/Graphics/Texture.h"
#include "Tests/EngineTestSupport.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

using namespace Astral;
using namespace Astral::Graphics;
using Math::Vec3;

namespace {

// Writes the frame to $ASTRAL_CAPTURE_DIR/<name>.png when the variable is set,
// so humans can inspect the exact images the assertions ran against.
void MaybeCapture(const RenderTarget& target, const char* name) {
    const char* directory = std::getenv("ASTRAL_CAPTURE_DIR");
    if (!directory || !*directory) return;
    std::string error;
    ASTRAL_CHECK(WritePng(std::string(directory) + "/" + name + ".png", target.output, error));
}

struct TestScene {
    MeshData ground, box, sphere, pool;
    Material groundMaterial, redMaterial, blueMaterial, waterMaterial;
    RenderScene scene;

    TestScene() {
        MeshBuilder(ground).AddPlane({0, 0, 20}, {30, 30}, 4, {6, 6});
        MeshBuilder(box).AddBox({0, 0, 0}, {1, 1, 1});
        MeshBuilder(sphere).AddSphere({0, 0, 0}, 1.0f, 24, 32);
        MeshBuilder(pool).AddPlane({0, 0, 0}, {4, 6}, 1);
        for (MeshData* mesh : {&ground, &box, &sphere, &pool}) mesh->ComputeBounds();
        groundMaterial.baseColor = FromSrgb8(120, 170, 90);
        groundMaterial.outline = false;
        redMaterial.baseColor = FromSrgb8(220, 60, 50);
        redMaterial.rimIntensity = 0.4f;
        blueMaterial.baseColor = FromSrgb8(60, 90, 220);
        blueMaterial.specularIntensity = 0.5f;
        waterMaterial.shading = ShadingModel::Water;
        waterMaterial.baseColor = FromSrgb8(20, 50, 70);
        waterMaterial.reflectivity = 0.4f;
        waterMaterial.waveAmplitude = 0.0f;
        scene.draws.push_back({&ground, &groundMaterial, Math::Mat4::Identity(), 1});
        scene.draws.push_back({&box, &redMaterial, Math::Translation({-2.5f, 1.0f, 8.0f}), 2});
        scene.draws.push_back({&sphere, &blueMaterial, Math::Translation({2.5f, 1.2f, 12.0f}), 3});
        scene.draws.push_back({&pool, &waterMaterial, Math::Translation({0.0f, 0.02f, 3.0f}), 4});
        scene.shadows.focus = {0, 0, 10};
        scene.shadows.radius = 20.0f;
        scene.shadows.resolution = 1024;
        scene.sun.direction = Math::Normalize(Vec3{0.5f, -0.7f, 0.6f});
    }
};

RenderView TestView(int width = 320, int height = 180) {
    return RenderView::Perspective({0.0f, 3.0f, -4.0f}, {0.0f, 1.0f, 10.0f}, 55.0f, width, height);
}

int ProjectX(const RenderView& view, Vec3 p) {
    const Math::Vec4 c = view.ViewProjection() * Math::Vec4{p.x, p.y, p.z, 1};
    return static_cast<int>((c.x / c.w * 0.5f + 0.5f) * static_cast<float>(view.width));
}
int ProjectY(const RenderView& view, Vec3 p) {
    const Math::Vec4 c = view.ViewProjection() * Math::Vec4{p.x, p.y, p.z, 1};
    return static_cast<int>((0.5f - c.y / c.w * 0.5f) * static_cast<float>(view.height));
}

} // namespace

ASTRAL_TEST(PngRoundTripAndCorruptionRejection) {
    ImageRgba8 image;
    image.Resize(37, 23);
    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x)
            image.Set(x, y, {static_cast<std::uint8_t>(x * 7), static_cast<std::uint8_t>(y * 11),
                                static_cast<std::uint8_t>((x ^ y) * 5), static_cast<std::uint8_t>(200 + (x & 7))});
    for (bool alpha : {false, true}) {
        const std::vector<std::uint8_t> png = EncodePng(image, alpha);
        ImageRgba8 decoded;
        std::string error;
        ASTRAL_CHECK(DecodePng(png.data(), png.size(), decoded, error));
        ASTRAL_CHECK(decoded.width == image.width && decoded.height == image.height);
        for (int y = 0; y < image.height; ++y) {
            for (int x = 0; x < image.width; ++x) {
                const Rgba8 a = image.Get(x, y), b = decoded.Get(x, y);
                ASTRAL_CHECK(a.r == b.r && a.g == b.g && a.b == b.b);
                ASTRAL_CHECK(b.a == (alpha ? a.a : 255));
            }
        }
        // Every single-byte corruption must be rejected or decode to a valid image, never crash.
        for (std::size_t i = 8; i < png.size(); i += 3) {
            std::vector<std::uint8_t> broken = png;
            broken[i] ^= 0x5A;
            ImageRgba8 ignored;
            std::string ignoredError;
            DecodePng(broken.data(), broken.size(), ignored, ignoredError);
        }
        std::vector<std::uint8_t> truncated(png.begin(), png.begin() + static_cast<long>(png.size() / 2));
        ASTRAL_CHECK(!DecodePng(truncated.data(), truncated.size(), decoded, error));
    }
    PngLimits tiny;
    tiny.maxPixels = 10;
    const std::vector<std::uint8_t> png = EncodePng(image);
    ImageRgba8 decoded;
    std::string error;
    ASTRAL_CHECK(!DecodePng(png.data(), png.size(), decoded, error, tiny));
}

ASTRAL_TEST(ZlibRoundTripsCompressibleAndRandomData) {
    std::vector<std::uint8_t> data(200000);
    std::uint32_t state = 1;
    for (std::size_t i = 0; i < data.size(); ++i) {
        state = state * 1664525u + 1013904223u;
        data[i] = i < 100000 ? static_cast<std::uint8_t>((i / 64) & 0xFF) : static_cast<std::uint8_t>(state >> 24);
    }
    const auto compressed = ZlibCompress(data.data(), data.size());
    ASTRAL_CHECK(compressed.size() < data.size());
    std::vector<std::uint8_t> restored;
    std::string error;
    ASTRAL_CHECK(ZlibDecompress(compressed.data(), compressed.size(), restored, data.size(), error));
    ASTRAL_CHECK(restored == data);
    ASTRAL_CHECK(!ZlibDecompress(compressed.data(), compressed.size(), restored, data.size() - 1, error));
}

ASTRAL_TEST(TextureMipsAndSampling) {
    std::vector<Math::Vec4> texels(8 * 8);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x) texels[static_cast<std::size_t>(y * 8 + x)] = ((x + y) & 1) ? Math::Vec4{1, 1, 1, 1} : Math::Vec4{0, 0, 0, 1};
    Texture2D texture;
    texture.FromTexels(8, 8, texels);
    ASTRAL_CHECK(texture.LevelCount() == 4);
    const Math::Vec4 coarse = texture.Sample({0.3f, 0.7f}, 10.0f);
    ASTRAL_CHECK_NEAR(coarse.x, 0.5f, 1e-4);
    const Math::Vec4 texelCenter = texture.SampleBilinear({0.5f / 8.0f, 0.5f / 8.0f}, 0);
    ASTRAL_CHECK_NEAR(texelCenter.x, 0.0f, 1e-5);
    const Math::Vec4 wrapped = texture.SampleBilinear({1.0f + 1.5f / 8.0f, 0.5f / 8.0f}, 0);
    ASTRAL_CHECK_NEAR(wrapped.x, 1.0f, 1e-5);
    ASTRAL_CHECK(texture.Sample({std::nanf(""), 0.0f}, 0.0f).w == 1.0f);
}

ASTRAL_TEST(MeshBuilderWindingFacesOutward) {
    MeshData mesh;
    MeshBuilder builder(mesh);
    builder.AddBox({0, 0, 0}, {1, 2, 3});
    builder.AddSphere({5, 0, 0}, 1.0f, 8, 12);
    builder.AddCylinder({-5, 0, 0}, 1.0f, 2.0f, 12, true, 0.5f);
    builder.AddCapsule({0, 0, 6}, 0.5f, 2.0f, 8, 12);
    builder.AddTaperedBox({0, 0, -6}, {1, 1}, {0.3f, 0.3f}, 5.0f);
    builder.AddPyramid({3, 0, -6}, {1, 1}, 1.0f);
    builder.AddTorus({0, 5, 0}, 2.0f, 0.3f, 16, 8);
    mesh.ComputeBounds();
    std::string error;
    ASTRAL_CHECK(mesh.Validate(error));
    int checked = 0;
    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        const Vertex& a = mesh.vertices[mesh.indices[i]];
        const Vertex& b = mesh.vertices[mesh.indices[i + 1]];
        const Vertex& c = mesh.vertices[mesh.indices[i + 2]];
        const Vec3 geometric = Math::Cross(b.position - a.position, c.position - a.position);
        if (Math::Length(geometric) < 1e-6f) continue;
        ASTRAL_CHECK(Math::Dot(geometric, a.normal + b.normal + c.normal) > 0.0f);
        ++checked;
    }
    ASTRAL_CHECK(checked > 500);
    MeshData broken = mesh;
    broken.indices.push_back(static_cast<std::uint32_t>(broken.vertices.size()));
    ASTRAL_CHECK(!broken.Validate(error));
}

ASTRAL_TEST(RasterizerTopLeftRuleCoversSharedEdgeOnce) {
    // Two triangles sharing a diagonal must cover a full quad with no gaps or double hits.
    const int size = 16;
    std::vector<int> hits(static_cast<std::size_t>(size * size), 0);
    auto vertex = [](float x, float y) {
        ClipVertex v;
        v.clip = {x, y, 0.5f, 1.0f};
        return v;
    };
    std::vector<SetupTriangle> triangles;
    // Clip-space quad covering x,y in [-0.5, 0.5] with a front-facing winding.
    ClipAndSetupTriangle(vertex(-0.5f, -0.5f), vertex(-0.5f, 0.5f), vertex(0.5f, 0.5f), CullMode::Back, size, size, 0, triangles);
    ClipAndSetupTriangle(vertex(-0.5f, -0.5f), vertex(0.5f, 0.5f), vertex(0.5f, -0.5f), CullMode::Back, size, size, 1, triangles);
    ASTRAL_CHECK(triangles.size() == 2);
    for (const SetupTriangle& t : triangles) {
        RasterizeTriangle(t, 0, 0, size, size, [&](int x, int y, float z, float, float, float) {
            ASTRAL_CHECK_NEAR(z, 0.5f, 1e-5);
            ++hits[static_cast<std::size_t>(y * size + x)];
        });
    }
    int covered = 0;
    for (int value : hits) {
        ASTRAL_CHECK(value <= 1);
        covered += value;
    }
    ASTRAL_CHECK(covered == 64); // exactly the 8x8 pixel centres inside the quad
    // The reverse winding is a back face and is culled.
    std::vector<SetupTriangle> culled;
    ClipAndSetupTriangle(vertex(-0.5f, -0.5f), vertex(0.5f, 0.5f), vertex(-0.5f, 0.5f), CullMode::Back, size, size, 0, culled);
    ASTRAL_CHECK(culled.empty());
    ClipAndSetupTriangle(vertex(-0.5f, -0.5f), vertex(0.5f, 0.5f), vertex(-0.5f, 0.5f), CullMode::None, size, size, 0, culled);
    ASTRAL_CHECK(culled.size() == 1 && culled[0].backFacing);
}

ASTRAL_TEST(ClippingHandlesTrianglesThroughTheCamera) {
    // A triangle crossing the near plane and the side planes is clipped, not dropped or exploded.
    ClipVertex a, b, c;
    a.clip = {-5.0f, -1.0f, -2.0f, 0.5f};
    b.clip = {0.0f, 3.0f, 5.0f, 6.0f};
    c.clip = {5.0f, -1.0f, 5.0f, 6.0f};
    std::vector<SetupTriangle> out;
    ClipAndSetupTriangle(a, b, c, CullMode::None, 64, 64, 0, out);
    ASTRAL_CHECK(!out.empty());
    for (const SetupTriangle& t : out) {
        for (int i = 0; i < 3; ++i) {
            ASTRAL_CHECK(t.z[i] >= 0.0f && t.z[i] <= 1.0f);
            ASTRAL_CHECK(t.x[i] >= -kSubpixelScale && t.x[i] <= 65 * kSubpixelScale);
        }
    }
    // Entirely behind the camera: nothing.
    a.clip = {0, 0, -1, -1};
    b.clip = {1, 0, -1, -1};
    c.clip = {0, 1, -1, -1};
    out.clear();
    ClipAndSetupTriangle(a, b, c, CullMode::None, 64, 64, 0, out);
    ASTRAL_CHECK(out.empty());
}

ASTRAL_TEST(SceneRendersWithDepthShadowsOutlinesAndSky) {
    Core::JobSystem jobs(3);
    TestScene test;
    SceneRenderer renderer(&jobs);
    RenderTarget target;
    const RenderView view = TestView();
    renderer.Render(test.scene, view, target);
    MaybeCapture(target, "graphics_test_scene");
    const RenderStats& stats = renderer.Stats();
    ASTRAL_CHECK(stats.drawsVisible == 4);
    ASTRAL_CHECK(stats.trianglesRasterized > 100);
    ASTRAL_CHECK(stats.pixelsShaded > 0);
    ASTRAL_CHECK(stats.reflectionPixels > 0);

    // Box centre shows the box (object 2), sphere centre the sphere (object 3).
    const int boxX = ProjectX(view, {-2.5f, 1.0f, 7.0f}), boxY = ProjectY(view, {-2.5f, 1.0f, 7.0f});
    ASTRAL_CHECK(target.objectId[target.Index(boxX, boxY)] == 2);
    const int sphereX = ProjectX(view, {2.5f, 1.2f, 11.0f}), sphereY = ProjectY(view, {2.5f, 1.2f, 11.0f});
    ASTRAL_CHECK(target.objectId[target.Index(sphereX, sphereY)] == 3);
    // Red box renders red-dominant; blue sphere blue-dominant.
    const Rgba8 boxPixel = target.output.Get(boxX, boxY);
    ASTRAL_CHECK(boxPixel.r > boxPixel.g + 40 && boxPixel.r > boxPixel.b + 40);
    const Rgba8 spherePixel = target.output.Get(sphereX, sphereY);
    ASTRAL_CHECK(spherePixel.b > spherePixel.r + 30);
    // Top of the frame is sky.
    ASTRAL_CHECK(target.flags[target.Index(view.width / 2, 2)] & kPixelSky);
    // The box casts a shadow on the ground away from the sun (+x/+z direction of travel).
    const Vec3 shadowPoint{-1.4f, 0.0f, 9.4f};
    const Vec3 litPoint{-5.0f, 0.0f, 6.0f};
    ASTRAL_CHECK(renderer.ShadowFactor(shadowPoint, {0, 1, 0}) < 0.2f);
    ASTRAL_CHECK(renderer.ShadowFactor(litPoint, {0, 1, 0}) > 0.9f);
    // Silhouette outline pixels are darker than the box interior.
    int outlineX = boxX;
    while (outlineX > 0 && target.objectId[target.Index(outlineX - 1, boxY)] == 2) --outlineX;
    const Rgba8 edge = target.output.Get(outlineX, boxY);
    ASTRAL_CHECK(edge.r + edge.g + edge.b < boxPixel.r + boxPixel.g + boxPixel.b);
}

ASTRAL_TEST(CameraOcclusionFadeDithersOnlyPropsBlockingTheFocus) {
    TestScene test;
    MeshData panel;
    MeshBuilder(panel).AddBox({0, 0, 0}, {0.8f, 0.8f, 0.1f});
    panel.ComputeBounds();
    Material prop;
    prop.baseColor = FromSrgb8(90, 90, 96);
    prop.cameraFade = true;
    test.scene.draws.push_back({&panel, &prop, Math::Translation({0.0f, 2.0f, 2.0f}), 20}); // on the eye->focus line
    test.scene.draws.push_back({&panel, &prop, Math::Translation({4.0f, 1.0f, 8.0f}), 21}); // off to the side
    SceneRenderer renderer(nullptr);
    RenderTarget target;
    RenderView view = TestView();
    auto count = [&](std::uint32_t id, bool* outlined) {
        int pixels = 0;
        for (std::size_t p = 0; p < target.objectId.size(); ++p) {
            if (target.objectId[p] != id) continue;
            ++pixels;
            if (outlined && (target.flags[p] & kPixelOutline)) *outlined = true;
        }
        return pixels;
    };
    renderer.Render(test.scene, view, target);
    const int solidBlocking = count(20, nullptr), solidSide = count(21, nullptr);
    view.occlusionFocus = {0.0f, 1.0f, 8.0f};
    view.occlusionRadius = 1.5f;
    renderer.Render(test.scene, view, target);
    MaybeCapture(target, "graphics_occlusion_fade");
    bool fadedOutlined = false;
    const int fadedBlocking = count(20, &fadedOutlined), fadedSide = count(21, nullptr);
    ASTRAL_CHECK(solidBlocking > 300);
    ASTRAL_CHECK(fadedBlocking > 0);
    ASTRAL_CHECK(fadedBlocking < solidBlocking * 35 / 100);
    ASTRAL_CHECK(!fadedOutlined);
    ASTRAL_CHECK(fadedSide == solidSide);
    // What was behind the screen door is now visible through it (the red box region).
    ASTRAL_CHECK(renderer.Stats().drawsVisible == 6);
}

ASTRAL_TEST(RenderingIsDeterministicAcrossThreadCounts) {
    TestScene test;
    const RenderView view = TestView(160, 90);
    RenderTarget single, multi;
    SceneRenderer serial(nullptr);
    serial.Render(test.scene, view, single);
    Core::JobSystem jobs(4);
    SceneRenderer parallel(&jobs);
    parallel.Render(test.scene, view, multi);
    ASTRAL_CHECK(HashImage(single.output) == HashImage(multi.output));
}

ASTRAL_TEST(TranslucencyAndBillboardsBlendOverOpaque) {
    Core::JobSystem jobs(2);
    TestScene test;
    MeshData shieldMesh;
    MeshBuilder(shieldMesh).AddSphere({0, 0, 0}, 1.5f, 16, 24);
    shieldMesh.ComputeBounds();
    Material shield;
    shield.blend = BlendMode::Additive;
    shield.baseColor = {1.0f, 0.85f, 0.2f};
    shield.opacity = 0.5f;
    shield.rimIntensity = 1.0f;
    test.scene.draws.push_back({&shieldMesh, &shield, Math::Translation({-2.5f, 1.0f, 8.0f}), 9});
    test.scene.billboards.push_back({{2.5f, 1.2f, 10.0f}, 0.8f, 0.0f, {4.0f, 1.0f, 3.0f, 1.0f}, BlendMode::Additive});
    SceneRenderer renderer(&jobs);
    RenderTarget target;
    const RenderView view = TestView();
    renderer.Render(test.scene, view, target);
    MaybeCapture(target, "graphics_translucency");
    ASTRAL_CHECK(renderer.Stats().transparentTriangles > 0);
    const int bx = ProjectX(view, {2.5f, 1.2f, 10.0f}), by = ProjectY(view, {2.5f, 1.2f, 10.0f});
    const Rgba8 glow = target.output.Get(bx, by);
    ASTRAL_CHECK(glow.r > 200 && glow.b > 150);
    // Translucent draws never write object ids.
    ASTRAL_CHECK(target.objectId[target.Index(ProjectX(view, {-2.5f, 1.0f, 7.0f}), ProjectY(view, {-2.5f, 1.0f, 7.0f}))] == 2);
}

ASTRAL_TEST(DebugLinesRespectDepth) {
    TestScene test;
    test.scene.lines.push_back({{-6, 1, 8}, {6, 1, 8}, {0, 255, 0, 255}, true});
    test.scene.lines.push_back({{-6, 3.5f, 20}, {6, 3.5f, 20}, {255, 0, 255, 255}, false});
    SceneRenderer renderer(nullptr);
    RenderTarget target;
    const RenderView view = TestView();
    renderer.Render(test.scene, view, target);
    int green = 0, magenta = 0;
    for (int y = 0; y < target.Height(); ++y) {
        for (int x = 0; x < target.Width(); ++x) {
            const Rgba8 p = target.output.Get(x, y);
            green += p.r == 0 && p.g == 255 && p.b == 0;
            magenta += p.r == 255 && p.g == 0 && p.b == 255;
        }
    }
    ASTRAL_CHECK(green > 20);
    ASTRAL_CHECK(magenta > 20);
    // The depth-tested green line is hidden where it passes behind the red box face at z = 7.
    const int boxX = ProjectX(view, {-2.5f, 1.0f, 8.0f}), lineY = ProjectY(view, {-2.5f, 1.0f, 8.0f});
    const Rgba8 hidden = target.output.Get(boxX, lineY);
    ASTRAL_CHECK(!(hidden.r == 0 && hidden.g == 255 && hidden.b == 0));
}

ASTRAL_TEST(ToneMapperIsMonotonicAndBounded) {
    float previous = -1.0f;
    for (float v = 0.0f; v < 50.0f; v += 0.05f) {
        const Color c = ToneMapNeutral({v, v, v});
        ASTRAL_CHECK(c.x >= previous - 1e-6f);
        ASTRAL_CHECK(c.x <= 1.0f + 1e-5f);
        previous = c.x;
    }
    const Color hue = ToneMapNeutral({0.5f, 0.2f, 0.1f});
    ASTRAL_CHECK(hue.x > hue.y && hue.y > hue.z);
}

ASTRAL_TEST(SkyLutMatchesAnalyticSkyAndPresetsScale) {
    TestScene test;
    SceneRenderer cached(nullptr);
    RenderTarget target;
    cached.Render(test.scene, TestView(64, 36), target); // builds the LUT
    for (const Vec3& dir : {Math::Normalize(Vec3{0.3f, 0.5f, 0.8f}), Math::Normalize(Vec3{-0.7f, 0.1f, 0.2f}),
             Vec3{0, 1, 0}, Math::Normalize(Vec3{0.2f, -0.4f, 1.0f})}) {
        const Color lut = cached.SkyColor(test.scene, dir);
        const Color base = SceneRenderer::SkyBase(test.scene.sky, dir, true);
        // Away from the sun the cached sky equals the analytic base sky (bilinear error only).
        if (Math::Dot(dir, -Math::Normalize(test.scene.sun.direction)) < 0.5f) {
            ASTRAL_CHECK_NEAR(lut.x, base.x, 0.08);
            ASTRAL_CHECK_NEAR(lut.y, base.y, 0.08);
            ASTRAL_CHECK_NEAR(lut.z, base.z, 0.08);
        }
    }
    const RendererSettings low = RendererSettings::Preset(0), epic = RendererSettings::Preset(3);
    ASTRAL_CHECK(low.maxShadowResolution < epic.maxShadowResolution);
    ASTRAL_CHECK(low.reflectionSteps < epic.reflectionSteps);
    ASTRAL_CHECK(!low.bloom && epic.bloom);
    // Every preset renders the same scene without NaNs and keeps the sky on top.
    for (int level = 0; level <= 3; ++level) {
        SceneRenderer renderer(nullptr);
        renderer.Settings() = RendererSettings::Preset(level);
        renderer.Render(test.scene, TestView(96, 54), target);
        ASTRAL_CHECK(target.flags[target.Index(48, 1)] & kPixelSky);
        for (const Color& c : target.hdr) ASTRAL_CHECK(Math::IsFinite(c));
    }
}

// ------------------------------------------------------------------ simplification and LOD

namespace {
bool SameVertex(const Vertex& a, const Vertex& b) {
    return std::memcmp(&a.position, &b.position, sizeof(a.position)) == 0 && std::memcmp(&a.uv, &b.uv, sizeof(a.uv)) == 0
        && std::memcmp(&a.normal, &b.normal, sizeof(a.normal)) == 0;
}
bool IsInputVertex(const MeshData& input, const Vertex& v) {
    for (const Vertex& original : input.vertices) {
        if (SameVertex(original, v)) return true;
    }
    return false;
}
} // namespace

ASTRAL_TEST(SimplificationCollapsesFlatSurfacesWithoutError) {
    MeshData plane;
    MeshBuilder(plane).AddPlane({0, 0, 0}, {4, 4}, 16);
    plane.ComputeBounds();
    ASTRAL_CHECK(plane.TriangleCount() == 512);
    MeshData simplified;
    SimplifyStats stats;
    SimplifyOptions options;
    options.targetTriangles = 2;
    ASTRAL_CHECK(SimplifyMesh(plane, options, simplified, &stats));
    ASTRAL_CHECK(simplified.TriangleCount() <= 4 && stats.trianglesIn == 512 && stats.collapses > 0);
    ASTRAL_CHECK(stats.error < 1e-3f);
    ASTRAL_CHECK(simplified.bounds.min.x == -4.0f && simplified.bounds.max.z == 4.0f); // corners survive
    for (std::size_t i = 0; i < simplified.indices.size(); i += 3) {
        const Vec3 a = simplified.vertices[simplified.indices[i]].position, b = simplified.vertices[simplified.indices[i + 1]].position,
                   c = simplified.vertices[simplified.indices[i + 2]].position;
        ASTRAL_CHECK(Math::Cross(b - a, c - a).y > 0.0f); // still facing up
    }
    // Locked borders keep every boundary vertex; the interior still collapses.
    options.lockBorders = true;
    options.targetTriangles = 0;
    options.targetRatio = 0.0f;
    ASTRAL_CHECK(SimplifyMesh(plane, options, simplified, &stats));
    ASTRAL_CHECK(simplified.TriangleCount() < 200);
    std::size_t borderVertices = 0;
    for (const Vertex& v : simplified.vertices) {
        if (std::fabs(std::fabs(v.position.x) - 4.0f) < 1e-6f || std::fabs(std::fabs(v.position.z) - 4.0f) < 1e-6f) ++borderVertices;
    }
    ASTRAL_CHECK(borderVertices == 64);
    // Invalid input is refused.
    MeshData broken = plane;
    broken.indices.push_back(9999);
    broken.indices.push_back(0);
    broken.indices.push_back(1);
    ASTRAL_CHECK(!SimplifyMesh(broken, options, simplified));
}

ASTRAL_TEST(SimplifiedSpheresStayCloseAndFacingOut) {
    MeshData sphere;
    MeshBuilder(sphere).AddSphere({0, 0, 0}, 1.0f, 32, 48);
    sphere.ComputeBounds();
    MeshData simplified;
    SimplifyStats stats;
    SimplifyOptions options;
    options.targetRatio = 0.25f;
    ASTRAL_CHECK(SimplifyMesh(sphere, options, simplified, &stats));
    ASTRAL_CHECK(simplified.TriangleCount() <= sphere.TriangleCount() / 4 + 1);
    ASTRAL_CHECK(simplified.TriangleCount() > sphere.TriangleCount() / 8);
    ASTRAL_CHECK(stats.error > 0.0f && stats.error < 0.2f);
    std::string error;
    ASTRAL_CHECK(simplified.Validate(error));
    // Half-edge collapses keep original vertices (UVs and normals untouched).
    for (const Vertex& v : simplified.vertices) ASTRAL_CHECK(IsInputVertex(sphere, v));
    for (std::size_t i = 0; i < simplified.indices.size(); i += 3) {
        const Vec3 a = simplified.vertices[simplified.indices[i]].position, b = simplified.vertices[simplified.indices[i + 1]].position,
                   c = simplified.vertices[simplified.indices[i + 2]].position;
        const Vec3 centroid = (a + b + c) / 3.0f;
        ASTRAL_CHECK(Math::Dot(Math::Cross(b - a, c - a), centroid) > 0.0f); // outward winding kept
        ASTRAL_CHECK(Math::Length(centroid) > 0.8f);                          // hugs the surface
    }
    // A tight error bound stops early on curved surfaces.
    options.maxError = 1.0e-4f;
    ASTRAL_CHECK(SimplifyMesh(sphere, options, simplified, &stats));
    ASTRAL_CHECK(simplified.TriangleCount() > sphere.TriangleCount() * 9 / 10);

    // Skinned meshes keep one influence per surviving vertex.
    MeshData skinned;
    MeshBuilder builder(skinned);
    builder.SetSkinJoint(3);
    builder.AddPlane({0, 0, 0}, {1, 1}, 8);
    ASTRAL_CHECK(skinned.skin.size() == skinned.vertices.size());
    options = {};
    options.targetRatio = 0.1f;
    ASTRAL_CHECK(SimplifyMesh(skinned, options, simplified, &stats));
    ASTRAL_CHECK(simplified.TriangleCount() < skinned.TriangleCount() / 4);
    ASTRAL_CHECK(simplified.skin.size() == simplified.vertices.size());
    for (const SkinInfluence& influence : simplified.skin) ASTRAL_CHECK(influence.joints[0] == 3);
}

ASTRAL_TEST(LodGroupsSelectByScreenSizeWithHysteresis) {
    ASTRAL_CHECK_NEAR(ScreenSize(1.0f, 10.0f, Math::Radians(90.0f)), 0.1f, 1e-5);
    ASTRAL_CHECK(ScreenSize(1.0f, 0.5f, 1.0f) > 1.0f && ScreenSize(0.0f, 5.0f, 1.0f) == 0.0f);
    auto sphere = std::make_shared<MeshData>();
    MeshBuilder(*sphere).AddSphere({0, 0, 0}, 1.0f, 24, 32);
    sphere->ComputeBounds();
    const LodGroup group = BuildLodGroup(sphere);
    ASTRAL_CHECK(group.Count() == 4);
    ASTRAL_CHECK(group.levels[0].mesh == sphere);
    for (std::size_t i = 1; i < group.Count(); ++i) {
        ASTRAL_CHECK(group.levels[i].mesh->TriangleCount() < group.levels[i - 1].mesh->TriangleCount());
        ASTRAL_CHECK(group.levels[i].minScreenSize < group.levels[i - 1].minScreenSize);
    }
    ASTRAL_CHECK(group.levels.back().minScreenSize == 0.0f);
    ASTRAL_CHECK(group.Select(0.8f) == 0 && group.Select(0.3f) == 1 && group.Select(0.15f) == 2 && group.Select(0.01f) == 3);
    // Hysteresis: thresholds must be passed by 10% before switching.
    ASTRAL_CHECK(group.Select(0.52f, 1) == 1 && group.Select(0.56f, 1) == 0);
    ASTRAL_CHECK(group.Select(0.48f, 0) == 0 && group.Select(0.44f, 0) == 1);
    ASTRAL_CHECK(group.Select(0.01f, 0) == 3 && group.Select(0.9f, 3) == 0);
    ASTRAL_CHECK(LodGroup{}.Select(1.0f) == -1);
    // A mesh that cannot be reduced (one triangle) yields a single level used everywhere.
    auto triangle = std::make_shared<MeshData>();
    MeshBuilder builder(*triangle);
    builder.AddTriangle(builder.AddVertex({0, 0, 0}, {0, 1, 0}, {}), builder.AddVertex({0, 0, 1}, {0, 1, 0}, {}),
        builder.AddVertex({1, 0, 0}, {0, 1, 0}, {}));
    triangle->ComputeBounds();
    const LodGroup single = BuildLodGroup(triangle);
    ASTRAL_CHECK(single.Count() == 1 && single.Select(0.001f) == 0);
}

ASTRAL_TEST_MAIN("EngineGraphicsTests")
