#include "Engine/Framework/GameWorld.h"
#include "Engine/Graphics/D3D11/BuiltinShaders.h"
#include "Engine/Graphics/D3D11/D3D11Renderer.h"
#include "Engine/Graphics/D3D11/ShaderCompiler.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/PostProcess.h"
#include "Engine/Graphics/SceneRenderer.h"
#include "Tests/EngineTestSupport.h"

#include <windows.h>
#include <psapi.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

using namespace Astral;
using namespace Astral::Graphics;
namespace GPU = Astral::Graphics::D3D11;

namespace {
constexpr int kWidth = 1280, kHeight = 720;
constexpr std::size_t kMiB = 1024u * 1024u;

void Require(bool success, const std::string& error) {
    if (!success) std::fprintf(stderr, "%s\n", error.c_str());
    ASTRAL_CHECK(success);
}
void Render(GPU::D3D11Renderer& renderer, const RenderScene& scene, const RenderView& view, GPU::Frame& frame) {
    std::string error;
    Require(renderer.RenderOpaqueUnlit(scene, view, frame, error) == GPU::RenderStatus::Rendered, error);
}
void Initialize(GPU::D3D11Renderer& renderer, GPU::Limits limits = {}) {
    std::string error;
    Require(renderer.Initialize(error, limits), error);
    const auto& info = renderer.Info();
    ASTRAL_CHECK(info.hardware && info.featureLevel == 0xb000 && !info.adapter.empty());
    std::printf("Hardware adapter=%s vendor=%04x device=%04x LUID=%llu FL=%x debugLayer=%d\n%s\n",
        info.adapter.c_str(), info.vendorId, info.deviceId, static_cast<unsigned long long>(info.adapterLuid),
        info.featureLevel, info.debugLayer, info.diagnostics.c_str());
}
void Neutral(RenderScene& scene) {
    scene.fog.enabled = false; scene.shadows.enabled = false;
    scene.sky.zenith = scene.sky.horizon = scene.sky.ground = scene.sky.sunColor = {};
    scene.post.bloom = scene.post.outlines = scene.post.fxaa = scene.post.dither = false;
    scene.post.saturation = scene.post.contrast = 1; scene.post.vignette = scene.post.chromaticAberration = 0;
}
void Reference(const RenderScene& scene, const RenderView& view, RenderTarget& frame) {
    SceneRenderer renderer;
    auto& settings = renderer.Settings();
    settings.clouds = settings.skyLut = settings.reflections = settings.bloom = settings.outlines = settings.fxaa = false;
    renderer.Render(scene, view, frame);
}
std::shared_ptr<MeshData> Quad() {
    auto mesh = std::make_shared<MeshData>();
    MeshBuilder builder(*mesh);
    builder.AddQuad({-1, -1, 0}, {-1, 1, 0}, {1, 1, 0}, {1, -1, 0}, {0, 0, -1});
    mesh->ComputeBounds();
    return mesh;
}
Material Unlit(Color color = {1, 1, 1}) {
    Material material;
    material.shading = ShadingModel::Unlit; material.baseColor = color;
    material.castShadows = material.receiveShadows = material.outline = false;
    return material;
}
struct Fixture {
    Framework::GameWorld world;
    std::shared_ptr<MeshData> box = std::make_shared<MeshData>(), quad = Quad();
    std::shared_ptr<Material> boxMaterial = std::make_shared<Material>(Unlit({0.7f, 0.3f, 0.2f}));
    std::shared_ptr<Material> quadMaterial = std::make_shared<Material>(Unlit({0.6f, 0.9f, 0.7f}));
    Texture2D texture;
    World::Entity boxEntity{}, quadEntity{};

    explicit Fixture(bool textured = true) {
        MeshBuilder builder(*box); builder.SetColor(Math::Vec4{0.7f, 0.8f, 0.9f, 1});
        builder.AddBox({}, {0.85f, 0.75f, 0.65f}); box->ComputeBounds();
        std::vector<Math::Vec4> texels(8 * 8);
        for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x)
            texels[static_cast<std::size_t>(y * 8 + x)] = {0.1f + 0.8f * x / 7, 0.1f + 0.8f * y / 7, 0.35f, 1};
        texture.FromTexels(8, 8, std::move(texels));
        if (textured) quadMaterial->baseTexture = &texture;
        quadMaterial->uvScale = {1.3f, 0.8f};
        boxMaterial->emissive = {0.03f, 0.01f, 0.02f};
        Math::TRS camera; camera.translation = {0, 0, -4};
        const auto eye = world.CreateEntity("GPU validation camera", camera);
        world.Add<Framework::Camera>(eye);
        Math::TRS pose; pose.translation = {-0.8f, 0.1f, 3.0f}; pose.rotation = Math::QuatFromAxisAngle({0, 1, 0}, 0.31f);
        boxEntity = world.CreateEntity("GPU validation indexed box", pose);
        auto& boxRenderer = world.Add<Framework::MeshRenderer>(boxEntity);
        boxRenderer.mesh = box; boxRenderer.material = boxMaterial; boxRenderer.objectId = 17;
        boxRenderer.tint = {0.9f, 0.8f, 1};
        pose.translation = {0.9f, -0.1f, 4.0f}; pose.rotation = Math::QuatFromAxisAngle({0, 1, 0}, -0.12f);
        quadEntity = world.CreateEntity("GPU validation textured quad", pose);
        auto& quadRenderer = world.Add<Framework::MeshRenderer>(quadEntity);
        quadRenderer.mesh = quad; quadRenderer.material = quadMaterial; quadRenderer.objectId = 23;
        world.Env().fog = world.Env().shadows = world.Env().bloom = world.Env().outlines = false;
    }
    void Build(RenderScene& scene, RenderView& view, int width = kWidth, int height = kHeight) {
        scene = {};
        ASTRAL_CHECK(world.BuildRenderScene(scene, view, width, height));
        ASTRAL_CHECK(scene.draws.size() == 2);
        ASTRAL_CHECK(scene.draws[0].mesh == box.get() && scene.draws[1].mesh == quad.get());
        Neutral(scene);
    }
};

bool Boundary(const std::vector<std::uint32_t>& ids, int width, int height, int x, int y) {
    const auto id = ids[static_cast<std::size_t>(y) * width + x];
    for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) {
        const int nx = std::clamp(x + dx, 0, width - 1), ny = std::clamp(y + dy, 0, height - 1);
        if (ids[static_cast<std::size_t>(ny) * width + nx] != id) return true;
    }
    return false;
}
void Compare(const RenderTarget& cpu, const GPU::Frame& gpu, bool textured) {
    ASTRAL_CHECK(cpu.Width() == gpu.width && cpu.Height() == gpu.height);
    ASTRAL_CHECK(cpu.Width() > 0 && cpu.Height() > 0);
    ASTRAL_CHECK(cpu.hdr.size() == cpu.PixelCount() && gpu.hdr.size() == cpu.PixelCount());
    ASTRAL_CHECK(cpu.objectId.size() == cpu.PixelCount() && gpu.objectId.size() == cpu.PixelCount());
    // Reject corrupt input on either side, including boundary/background pixels,
    // before subtraction, maximum/mean accumulation or boundary exclusions.
    for (std::size_t p = 0; p < cpu.PixelCount(); ++p) {
        for (const Color color : {cpu.hdr[p], gpu.hdr[p]}) {
            ASTRAL_CHECK(std::isfinite(color.x) && std::isfinite(color.y) && std::isfinite(color.z));
        }
    }
    std::size_t differences = 0, checked = 0, foreground = 0;
    double maximum = 0, sum = 0;
    for (int y = 0; y < gpu.height; ++y) for (int x = 0; x < gpu.width; ++x) {
        const auto p = static_cast<std::size_t>(y) * gpu.width + x;
        const bool boundary = Boundary(cpu.objectId, gpu.width, gpu.height, x, y)
            || Boundary(gpu.objectId, gpu.width, gpu.height, x, y);
        if (cpu.objectId[p] != gpu.objectId[p]) { ++differences; ASTRAL_CHECK(boundary); }
        if (gpu.objectId[p]) ++foreground;
        if (!boundary) {
            ASTRAL_CHECK(cpu.objectId[p] == gpu.objectId[p]);
            const Color delta = cpu.hdr[p] - gpu.hdr[p];
            for (float value : {delta.x, delta.y, delta.z}) {
                maximum = std::max(maximum, static_cast<double>(std::fabs(value)));
                // Do not dilute the texture mean with the black background.
                if (gpu.objectId[p]) { sum += std::fabs(value); ++checked; }
            }
        }
    }
    ASTRAL_CHECK(foreground > 100 && checked > 100);
    const double fraction = static_cast<double>(differences) / gpu.hdr.size();
    const double mean = sum / checked;
    std::printf("CPU/GPU coverage differences=%zu (%.6f), RGB max=%.9f mean=%.9f textured=%d\n",
        differences, fraction, maximum, mean, textured);
    ASTRAL_CHECK(fraction <= 0.005);
    ASTRAL_CHECK(maximum <= (textured ? 1.0 / 128.0 : 2.0e-4));
    if (textured) ASTRAL_CHECK(mean <= 1.0 / 512.0);
}
int CompareProbe(const std::string& side, const std::string& value, bool textured) {
    // This private CPU-only child calls the actual comparison oracle. No
    // renderer, device, shader compiler or desktop window is constructed.
    RenderTarget cpu; cpu.Resize(16, 16);
    GPU::Frame gpu; gpu.width = gpu.height = 16;
    gpu.hdr.assign(16 * 16, Color{0.25f, 0.5f, 0.75f});
    gpu.objectId.assign(16 * 16, 17);
    cpu.hdr = gpu.hdr; cpu.objectId = gpu.objectId;
    if (value != "finite") {
        ASTRAL_CHECK(side == "cpu" || side == "gpu");
        Color& pixel = side == "cpu" ? cpu.hdr[8 * 16 + 8] : gpu.hdr[8 * 16 + 8];
        if (value == "nan") pixel.x = std::numeric_limits<float>::quiet_NaN();
        else if (value == "positive-infinity") pixel.y = std::numeric_limits<float>::infinity();
        else if (value == "negative-infinity") pixel.z = -std::numeric_limits<float>::infinity();
        else ASTRAL_CHECK(false);
    } else ASTRAL_CHECK(side == "none");
    Compare(cpu, gpu, textured);
    return 0; // Only the finite positive controls should reach this return.
}
DWORD RunCompareNonfiniteProbe(const wchar_t* side = L"cpu", const wchar_t* value = L"nan", bool textured = false) {
    std::vector<wchar_t> executable(32768);
    const DWORD count = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    ASTRAL_CHECK(count > 0 && count < executable.size());
    std::wstring command = L"\"" + std::wstring(executable.data(), count) + L"\" --compare-probe "
        + side + L" " + value + (textured ? L" 1" : L" 0");
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW; startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    ASTRAL_CHECK(CreateProcessW(executable.data(), command.data(), nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process));
    const DWORD wait = WaitForSingleObject(process.hProcess, 5000);
    if (wait != WAIT_OBJECT_0) {
        // Only this explicitly created child is terminated on its deadline.
        TerminateProcess(process.hProcess, 99);
        WaitForSingleObject(process.hProcess, 2000);
    }
    DWORD exitCode = STILL_ACTIVE;
    const BOOL gotExitCode = GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    ASTRAL_CHECK(wait == WAIT_OBJECT_0 && gotExitCode);
    std::printf("CPU-only comparison child side=%ls value=%ls textured=%d exit=%lu\n", side, value, textured, exitCode);
    std::fflush(stdout);
    return exitCode;
}
ImageRgba8 Display(const std::vector<Color>& pixels, int width, int height) {
    ImageRgba8 image; image.Resize(width, height);
    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
        const Color color = ToneMapNeutral(Math::Max(pixels[static_cast<std::size_t>(y) * width + x], Color{}));
        image.Set(x, y, {EncodeSrgb8(color.x), EncodeSrgb8(color.y), EncodeSrgb8(color.z), 255});
    }
    return image;
}
void Capture(const RenderTarget& cpu, const GPU::Frame& gpu) {
    const char* location = std::getenv("ASTRAL_CAPTURE_DIR");
    if (!location || !*location) return;
    const std::filesystem::path directory = std::filesystem::u8path(location);
    std::filesystem::create_directories(directory);
    std::string error;
    Require(WritePng((directory / "d3d11-cpu.png").u8string(), Display(cpu.hdr, gpu.width, gpu.height), error), error);
    Require(WritePng((directory / "d3d11-gpu.png").u8string(), Display(gpu.hdr, gpu.width, gpu.height), error), error);
    auto difference = gpu.hdr, ids = gpu.hdr;
    for (std::size_t p = 0; p < difference.size(); ++p) {
        const Color delta = gpu.hdr[p] - cpu.hdr[p];
        difference[p] = {std::fabs(delta.x) * 16, std::fabs(delta.y) * 16, std::fabs(delta.z) * 16};
        ids[p] = gpu.objectId[p] == 17 ? Color{1, 0.2f, 0.1f} : gpu.objectId[p] ? Color{0.1f, 1, 0.3f} : Color{};
    }
    Require(WritePng((directory / "d3d11-difference-x16.png").u8string(), Display(difference, gpu.width, gpu.height), error), error);
    Require(WritePng((directory / "d3d11-object-ids.png").u8string(), Display(ids, gpu.width, gpu.height), error), error);
}
double Percentile(std::vector<double> samples, double percentile) {
    std::sort(samples.begin(), samples.end());
    return samples[static_cast<std::size_t>(std::ceil(percentile * samples.size())) - 1];
}
void PrintTimes(const char* name, const std::vector<double>& values) {
    std::printf("%s n=%zu min=%.6f p50=%.6f p95=%.6f p99=%.6f max=%.6f ms\n", name, values.size(),
        *std::min_element(values.begin(), values.end()), Percentile(values, .5), Percentile(values, .95),
        Percentile(values, .99), *std::max_element(values.begin(), values.end()));
}
void MemoryGuard() {
    MEMORYSTATUSEX memory{}; memory.dwLength = sizeof(memory);
    ASTRAL_CHECK(GlobalMemoryStatusEx(&memory));
    std::printf("Available host bytes=%llu\n", static_cast<unsigned long long>(memory.ullAvailPhys));
    ASTRAL_CHECK(memory.ullAvailPhys >= 6ull * 1024ull * kMiB);
}
PROCESS_MEMORY_COUNTERS_EX ProcessMemory() {
    PROCESS_MEMORY_COUNTERS_EX counters{}; counters.cb = sizeof(counters);
    ASTRAL_CHECK(GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters)));
    return counters;
}
} // namespace

ASTRAL_TEST(ComparisonOracleRejectsNonfiniteCpuReference) {
    ASTRAL_CHECK(RunCompareNonfiniteProbe() == 3);
}

ASTRAL_TEST(ComparisonOracleRejectsNonfiniteHdrOnEitherSide) {
    for (const wchar_t* side : {L"cpu", L"gpu"})
        for (const wchar_t* value : {L"nan", L"positive-infinity", L"negative-infinity"})
            for (bool textured : {false, true})
                ASTRAL_CHECK(RunCompareNonfiniteProbe(side, value, textured) == 3);
}

ASTRAL_TEST(ComparisonOracleAcceptsFiniteHdrInBothModes) {
    for (bool textured : {false, true})
        ASTRAL_CHECK(RunCompareNonfiniteProbe(L"none", L"finite", textured) == 0);
}

ASTRAL_TEST(ShaderCompilerReturnsRealDiagnosticsAndRecovers) {
    const auto broken = GPU::CompileShader("float4 Broken( : SV_Target {", "invalid-neutral-fixture", "Broken", "ps_5_0");
    ASTRAL_CHECK(!broken.Succeeded());
    ASTRAL_CHECK(broken.diagnostics.find("invalid-neutral-fixture") != std::string::npos);
    ASTRAL_CHECK(broken.diagnostics.find("error X") != std::string::npos);
    std::printf("Expected shader error: %s\n", broken.diagnostics.c_str());
    const auto fixed = GPU::CompileShader("float4 Main() : SV_Target { return float4(1,0,0,1); }", "valid-neutral-fixture", "Main", "ps_5_0");
    ASTRAL_CHECK(fixed.Succeeded() && fixed.bytecode.size() > 16);
    // Compile the exact production source without Initialize/device creation.
    // Cover both helper flag paths independently of the executable build mode.
    for (bool debug : {true, false}) {
        const auto vertex = GPU::CompileShader(GPU::Detail::kOpaqueUnlitShader,
            "AstralOpaqueUnlit", "VSMain", "vs_5_0", debug);
        Require(vertex.Succeeded(), vertex.diagnostics);
        ASTRAL_CHECK(!vertex.bytecode.empty());
        const auto pixel = GPU::CompileShader(GPU::Detail::kOpaqueUnlitShader,
            "AstralOpaqueUnlit", "PSMain", "ps_5_0", debug);
        Require(pixel.Succeeded(), pixel.diagnostics);
        ASTRAL_CHECK(!pixel.bytecode.empty());
        std::printf("Production opaque Unlit shaders debug=%d VSbytes=%zu PSbytes=%zu\n",
            debug, vertex.bytecode.size(), pixel.bytecode.size());
    }
}

ASTRAL_TEST(WorldRenderSceneMatchesHardwareUnlitAndLinearTextures) {
    MemoryGuard();
    GPU::D3D11Renderer renderer; Initialize(renderer);
    for (bool textured : {false, true}) {
        Fixture fixture(textured); RenderScene scene; RenderView view; fixture.Build(scene, view);
        RenderTarget cpu; GPU::Frame gpu;
        Reference(scene, view, cpu); Render(renderer, scene, view, gpu); Compare(cpu, gpu, textured);
        ASTRAL_CHECK(gpu.stats.draws == 2 && gpu.stats.triangles == 14 && gpu.stats.vertices == 28);
        ASTRAL_CHECK(gpu.stats.textureCount == (textured ? 1u : 0u));
        if (textured) {
            Capture(cpu, gpu);
            fixture.texture.wrap = TextureWrap::Clamp;
            scene.draws[0].material = fixture.boxMaterial.get(); fixture.boxMaterial->vertexColor = false;
            scene.draws[0].emissiveBoost = {0.02f, 0.01f, 0};
            Reference(scene, view, cpu); Render(renderer, scene, view, gpu); Compare(cpu, gpu, true);
        }
    }
}

ASTRAL_TEST(DepthClippingWindingAndDoubleSidedUseHardware) {
    GPU::D3D11Renderer renderer; Initialize(renderer);
    auto mesh = Quad(); Material nearMaterial = Unlit({0.9f, 0.1f, 0.2f}), farMaterial = Unlit({0.1f, 0.8f, 0.3f});
    RenderView view = RenderView::Perspective({}, {0, 0, 1}, 65, 320, 180, .25f, 20);
    RenderScene scene; Neutral(scene);
    scene.draws.push_back({mesh.get(), &farMaterial, Math::Translation({0, 0, 4}), 72});
    scene.draws.push_back({mesh.get(), &nearMaterial, Math::Translation({0, 0, 2}), 71});
    GPU::Frame gpu; RenderTarget cpu;
    Reference(scene, view, cpu); Render(renderer, scene, view, gpu); Compare(cpu, gpu, false);
    ASTRAL_CHECK(gpu.objectId[static_cast<std::size_t>(90) * 320 + 160] == 71);
    const auto first = gpu.objectId;
    std::reverse(scene.draws.begin(), scene.draws.end()); Render(renderer, scene, view, gpu);
    ASTRAL_CHECK(gpu.objectId == first); // depth, not submission order
    for (std::size_t i = 0; i < mesh->indices.size(); i += 3) std::swap(mesh->indices[i], mesh->indices[i + 1]);
    Render(renderer, scene, view, gpu);
    ASTRAL_CHECK(std::all_of(gpu.objectId.begin(), gpu.objectId.end(), [](auto id) { return id == 0; }));
    nearMaterial.doubleSided = farMaterial.doubleSided = true;
    Render(renderer, scene, view, gpu); ASTRAL_CHECK(gpu.objectId == first);
    // One vertex crosses the near plane and two cross screen boundaries.
    mesh->vertices[0].position = {-3, -2, -1.9f}; mesh->ComputeBounds();
    Reference(scene, view, cpu); Render(renderer, scene, view, gpu); Compare(cpu, gpu, false);
}

ASTRAL_TEST(FrameUploadsDoNotKeepStalePointerIdentityAndMipsAreUsed) {
    GPU::D3D11Renderer renderer; Initialize(renderer);
    Fixture fixture; RenderScene scene; RenderView view; fixture.Build(scene, view, 320, 180);
    GPU::Frame first, changed; Render(renderer, scene, view, first);
    // Mutate the same MeshData/Texture2D objects; cross-frame pointer caches lose this change.
    for (auto& vertex : fixture.quad->vertices) vertex.position.x += 0.3f;
    fixture.quad->ComputeBounds();
    fixture.texture.FromTexels(8, 8, std::vector<Math::Vec4>(64, {0.15f, 0.35f, 0.65f, 1}));
    Render(renderer, scene, view, changed); ASTRAL_CHECK(first.objectId != changed.objectId);
    RenderTarget cpu; Reference(scene, view, cpu); Compare(cpu, changed, true);
    // An even checker has black/white level 0 and uniform .5 coarse levels.
    std::vector<Math::Vec4> checker(64 * 64);
    for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x)
        checker[static_cast<std::size_t>(y * 64 + x)] = ((x + y) & 1) ? Math::Vec4{1, 1, 1, 1} : Math::Vec4{0, 0, 0, 1};
    fixture.texture.FromTexels(64, 64, std::move(checker)); fixture.quadMaterial->uvScale = {200, 200};
    Render(renderer, scene, view, changed); Reference(scene, view, cpu); Compare(cpu, changed, true);
    std::size_t checked = 0;
    const Color expected = fixture.quadMaterial->baseColor * .5f;
    for (int y = 1; y + 1 < view.height; ++y) for (int x = 1; x + 1 < view.width; ++x) {
        const auto p = static_cast<std::size_t>(y) * view.width + x;
        if (changed.objectId[p] != 23 || Boundary(changed.objectId, view.width, view.height, x, y)) continue;
        ASTRAL_CHECK_NEAR(changed.hdr[p].x, expected.x, 2e-4);
        ASTRAL_CHECK_NEAR(changed.hdr[p].y, expected.y, 2e-4);
        ++checked;
    }
    ASTRAL_CHECK(checked > 20);
}

ASTRAL_TEST(InvalidInputsBudgetsAndZeroViewportPreserveLastFrame) {
    GPU::D3D11Renderer renderer; Initialize(renderer);
    Fixture fixture; RenderScene scene; RenderView view; fixture.Build(scene, view, 160, 90);
    GPU::Frame frame; Render(renderer, scene, view, frame);
    const auto ids = frame.objectId; const Color first = frame.hdr[0];
    const auto rejects = [&] {
        std::string error;
        ASTRAL_CHECK(renderer.RenderOpaqueUnlit(scene, view, frame, error) == GPU::RenderStatus::Failed);
        ASTRAL_CHECK(!error.empty()); ASTRAL_CHECK(frame.objectId == ids && frame.width == 160);
        ASTRAL_CHECK(frame.hdr[0].x == first.x);
        std::printf("Expected rejection: %s\n", error.c_str());
    };
    const auto saved = view;
    view.width = -1; rejects(); view = saved;
    view.width = std::numeric_limits<int>::max(); rejects(); view = saved;
    view.projection.m[0][0] = std::numeric_limits<float>::quiet_NaN(); rejects(); view = saved;
    const auto index = fixture.box->indices[0]; fixture.box->indices[0] = UINT32_MAX; rejects(); fixture.box->indices[0] = index;
    const auto vertexColor = fixture.box->vertices[0].color;
    fixture.box->vertices[0].color.x = std::numeric_limits<float>::quiet_NaN(); rejects(); fixture.box->vertices[0].color = vertexColor;
    fixture.boxMaterial->shading = ShadingModel::Toon; rejects(); fixture.boxMaterial->shading = ShadingModel::Unlit;
    fixture.boxMaterial->blend = BlendMode::AlphaBlend; rejects(); fixture.boxMaterial->blend = BlendMode::Opaque;
    fixture.boxMaterial->cameraFade = true; rejects(); fixture.boxMaterial->cameraFade = false;
    scene.draws[0].opacity = .5f; rejects(); scene.draws[0].opacity = 1;
    scene.lines.push_back({}); rejects(); scene.lines.clear();
    scene.billboards.push_back({}); rejects(); scene.billboards.clear();
    fixture.texture.FromTexels(1, 1, {{std::numeric_limits<float>::infinity(), 0, 0, 1}}); rejects();
    fixture.texture.FromTexels(1, 1, {{1, 1, 1, 1}});
    std::string error; view.width = 0;
    ASTRAL_CHECK(renderer.RenderOpaqueUnlit(scene, view, frame, error) == GPU::RenderStatus::Skipped);
    ASTRAL_CHECK(frame.objectId == ids); view = saved;
    Render(renderer, scene, view, frame);
    for (int budget = 0; budget < 5; ++budget) {
        GPU::Limits limits;
        if (budget == 0) limits.maxDraws = 1;
        if (budget == 1) limits.maxGeometryBytes = 1;
        if (budget == 2) limits.maxTextureBytes = 1;
        if (budget == 3) limits.maxPixels = 1;
        if (budget == 4) limits.maxResourceBytes = 128;
        Initialize(renderer, limits); rejects();
    }
    GPU::Limits wide;
    wide.maxDraws = wide.maxGeometryBytes = wide.maxTextureBytes = wide.maxPixels = wide.maxResourceBytes = std::numeric_limits<std::size_t>::max();
    Initialize(renderer, wide); Render(renderer, scene, view, frame); // no SIZE_MAX arithmetic wrapping
    GPU::RenderStatus otherThreadStatus{};
    std::string otherThreadError;
    std::thread otherThread([&] { otherThreadStatus = renderer.RenderOpaqueUnlit(scene, view, frame, otherThreadError); });
    otherThread.join();
    ASTRAL_CHECK(otherThreadStatus == GPU::RenderStatus::Failed && !otherThreadError.empty());
    ASTRAL_CHECK(frame.objectId == ids && renderer.Ready());
    renderer.Shutdown(); renderer.Shutdown(); ASTRAL_CHECK(!renderer.Ready());
    ASTRAL_CHECK(renderer.RenderOpaqueUnlit(scene, view, frame, error) == GPU::RenderStatus::Failed);
    Initialize(renderer); Render(renderer, scene, view, frame);
}

ASTRAL_TEST(RepeatedResizeReloadAndReinitializeKeepResourceCountsBounded) {
    MemoryGuard();
    GPU::D3D11Renderer renderer; Initialize(renderer);
    Fixture fixture; RenderScene scene; RenderView view; GPU::Frame frame;
    fixture.Build(scene, view, 160, 90); Render(renderer, scene, view, frame);
    const auto resources = frame.stats.resourceCount;
    const auto bytesAtSmallSize = frame.stats.resourceBytes;
    const auto before = ProcessMemory();
    for (int cycle = 0; cycle < 20; ++cycle) {
        fixture.Build(scene, view, cycle & 1 ? 320 : 160, cycle & 1 ? 180 : 90);
        fixture.texture.FromTexels(8, 8, std::vector<Math::Vec4>(64, {0.02f * cycle, .3f, .7f, 1}));
        Render(renderer, scene, view, frame);
        ASTRAL_CHECK(frame.stats.resourceCount == resources && frame.stats.resourceBytes <= 256 * kMiB);
        if (cycle == 9) { renderer.Shutdown(); Initialize(renderer); }
    }
    fixture.Build(scene, view, 160, 90); Render(renderer, scene, view, frame);
    ASTRAL_CHECK(frame.stats.resourceCount == resources);
    ASTRAL_CHECK(frame.stats.resourceBytes == bytesAtSmallSize);
    const auto after = ProcessMemory();
    std::printf("20 resize/reload cycles: resourceCount=%zu accountedBytes=%zu privateBefore=%zu privateAfter=%zu working=%zu\n",
        resources, frame.stats.resourceBytes, before.PrivateUsage, after.PrivateUsage, after.WorkingSetSize);
    ASTRAL_CHECK(after.PrivateUsage <= 768 * kMiB);
    renderer.Shutdown(); ASTRAL_CHECK(!renderer.Ready());
}

ASTRAL_TEST(Fixed720pHardwareWorkloadRecordsDeclaredBudgets) {
    MemoryGuard();
    GPU::D3D11Renderer renderer; Initialize(renderer);
    Fixture fixture; RenderScene scene; RenderView view; fixture.Build(scene, view);
    GPU::Frame frame;
    for (int i = 0; i < 30; ++i) Render(renderer, scene, view, frame);
    std::vector<double> gpu, upload, submit, readback, total;
    for (int i = 0; i < 120; ++i) {
        if (i % 30 == 0) MemoryGuard();
        Render(renderer, scene, view, frame);
        ASTRAL_CHECK(frame.stats.gpuTimingValid);
        gpu.push_back(frame.stats.gpuMs); upload.push_back(frame.stats.uploadMs);
        submit.push_back(frame.stats.submissionMs); readback.push_back(frame.stats.completionReadbackMs); total.push_back(frame.stats.totalMs);
    }
    PrintTimes("GPU opaque Unlit pass", gpu); PrintTimes("CPU prepare/upload", upload);
    PrintTimes("CPU submission", submit); PrintTimes("CPU completion/readback", readback); PrintTimes("Synchronous total", total);
    RenderTarget cpu;
    for (int i = 0; i < 2; ++i) Reference(scene, view, cpu);
    std::vector<double> reference;
    for (int i = 0; i < 10; ++i) {
        const auto begin = std::chrono::steady_clock::now(); Reference(scene, view, cpu);
        reference.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count());
    }
    PrintTimes("CPU reference full Render (includes CPU post; not pass parity)", reference);
    Compare(cpu, frame, true);
    const auto memory = ProcessMemory();
    std::printf("Fixed workload %dx%d draws=%zu vertices=%zu triangles=%zu textureCount=%zu resourceCount=%zu accountedBytes=%zu private=%zu working=%zu localVRAMAvailable=%d localVRAMUsage=%llu localVRAMBudget=%llu\n",
        frame.width, frame.height, frame.stats.draws, frame.stats.vertices, frame.stats.triangles, frame.stats.textureCount,
        frame.stats.resourceCount, frame.stats.resourceBytes, memory.PrivateUsage, memory.WorkingSetSize, frame.stats.videoMemoryAvailable,
        static_cast<unsigned long long>(frame.stats.localVideoUsage), static_cast<unsigned long long>(frame.stats.localVideoBudget));
    ASTRAL_CHECK(Percentile(gpu, .95) <= 16.67 && Percentile(total, .95) <= 100);
    ASTRAL_CHECK(memory.PrivateUsage <= 768 * kMiB && frame.stats.resourceBytes <= 256 * kMiB);
}

int main(int argc, char** argv) {
    if (argc == 2 && std::strcmp(argv[1], "--compare-nonfinite-probe") == 0) {
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        return CompareProbe("cpu", "nan", false);
    }
    if (argc == 5 && std::strcmp(argv[1], "--compare-probe") == 0) {
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        ASTRAL_CHECK(std::strcmp(argv[4], "0") == 0 || std::strcmp(argv[4], "1") == 0);
        return CompareProbe(argv[2], argv[3], std::strcmp(argv[4], "1") == 0);
    }
    return Test::RunAll(argc, argv, "EngineD3D11Tests");
}
