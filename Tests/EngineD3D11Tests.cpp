#include "Engine/Framework/GameWorld.h"
#include "Engine/Framework/GameHost.h"
#include "Engine/Platform/Win32/D3D11Viewport.h"
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
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <map>
#include <numeric>
#include <regex>
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
        const auto displayVertex = GPU::CompileShader(GPU::Detail::kNativeDisplayShader,
            "AstralNativeDisplay", "VSMain", "vs_5_0", debug);
        const auto displayPixel = GPU::CompileShader(GPU::Detail::kNativeDisplayShader,
            "AstralNativeDisplay", "PSMain", "ps_5_0", debug);
        Require(displayVertex.Succeeded(), displayVertex.diagnostics);
        Require(displayPixel.Succeeded(), displayPixel.diagnostics);
        std::printf("Production native display shaders debug=%d VSbytes=%zu PSbytes=%zu\n",
            debug, displayVertex.bytecode.size(), displayPixel.bytecode.size());
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

namespace {
using NativeClock = std::chrono::steady_clock;

void PumpNativeMessages() {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        ASTRAL_CHECK(message.message != WM_QUIT);
        TranslateMessage(&message); DispatchMessageW(&message);
    }
}
struct NativeWindow {
    HWND window{};
    NativeWindow(int width = kWidth, int height = kHeight) {
        constexpr wchar_t name[] = L"AstralNativeGpuTestParent";
        WNDCLASSW type{}; type.hInstance = GetModuleHandleW(nullptr);
        type.lpfnWndProc = DefWindowProcW; type.lpszClassName = name;
        ASTRAL_CHECK(RegisterClassW(&type) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS);
        RECT frame{0, 0, width, height};
        ASTRAL_CHECK(AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, FALSE, 0));
        window = CreateWindowExW(0, name, L"Astral D3D11 native fixture (test only)", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
            20, 20, frame.right - frame.left, frame.bottom - frame.top, nullptr, nullptr, type.hInstance, nullptr);
        ASTRAL_CHECK(window);
        ShowWindow(window, SW_SHOWNOACTIVATE);
        ASTRAL_CHECK(SetWindowPos(window, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE));
        PumpNativeMessages();
        RECT client{}; ASTRAL_CHECK(GetClientRect(window, &client));
        ASTRAL_CHECK(client.right == width && client.bottom == height);
    }
    void Resize(int width, int height, const char* phase, int cycle) {
        RECT frame{0, 0, width, height};
        ASTRAL_CHECK(AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, FALSE, 0));
        // Only the explicit zero-client probe bypasses default min-size
        // validation. Nonzero native and measured fixture placement is normal.
        const UINT exactZero = width == 0 && height == 0 ? SWP_NOSENDCHANGING : 0;
        ASTRAL_CHECK(SetWindowPos(window, nullptr, 0, 0, frame.right - frame.left, frame.bottom - frame.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | exactZero));
        PumpNativeMessages();
        RECT client{}; ASTRAL_CHECK(GetClientRect(window, &client));
        RECT outer{}; const BOOL outerKnown = GetWindowRect(window, &outer);
        std::printf("Native resize cycle=%d phase=%s requestedClient=%dx%d actualClient=%ldx%ld outerKnown=%d actualOuter=%ldx%ld style=0x%08lX minimized=%d\n",
            cycle, phase, width, height, client.right, client.bottom, outerKnown,
            outer.right - outer.left, outer.bottom - outer.top,
            static_cast<unsigned long>(GetWindowLongPtrW(window, GWL_STYLE)), IsIconic(window));
        ASTRAL_CHECK(client.right == width && client.bottom == height);
    }
    ~NativeWindow() { if (window) DestroyWindow(window); }
};

struct NativeFixture {
    std::filesystem::path parent, root;
    std::unique_ptr<Framework::GameHost> host;
    NativeFixture() {
        const char* fixture = std::getenv("ASTRAL_NATIVE_GPU_FIXTURE");
        Require(fixture && *fixture, "ASTRAL_NATIVE_GPU_FIXTURE must name the reviewed separate Content fixture");
        const auto source = std::filesystem::canonical(std::filesystem::u8path(fixture));
        parent = std::filesystem::canonical(std::filesystem::temp_directory_path());
        const auto stamp = NativeClock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt) {
            const auto candidate = parent / ("astral_native_gpu_" + std::to_string(GetCurrentProcessId()) + "_"
                + std::to_string(stamp) + "_" + std::to_string(attempt));
            std::error_code error;
            if (std::filesystem::create_directory(candidate, error)) { root = std::filesystem::canonical(candidate); break; }
            ASTRAL_CHECK(!error);
        }
        ASTRAL_CHECK(!root.empty() && root.parent_path() == parent);
        std::filesystem::copy(source, root, std::filesystem::copy_options::recursive);
        Framework::HostSettings settings;
        settings.contentRoot = root.u8string(); settings.workerThreads = 0; settings.audio = false;
        host = std::make_unique<Framework::GameHost>(settings);
        std::string error; Require(host->LoadProject("project.json", error), error);
        host->Update(0, {});
        std::printf("Actual authored native fixture root=%s scene=%s\n", root.u8string().c_str(), host->ScenePath().c_str());
    }
    ~NativeFixture() {
        host.reset();
        std::error_code error;
        const auto current = std::filesystem::weakly_canonical(root, error);
        if (!error && !root.empty() && current == root && root.parent_path() == parent
            && root.filename().u8string().find("astral_native_gpu_") == 0) std::filesystem::remove_all(root, error);
    }
    Core::JsonValue SceneJson() const {
        Core::JsonValue json; std::string error;
        Require(Core::ReadJsonFile((root / "scenes/main.scene.json").u8string(), json, error), error);
        return json;
    }
    void Reload(Core::JsonValue json) {
        std::string error;
        Require(Core::WriteJsonFile((root / "scenes/main.scene.json").u8string(), json, error), error);
        ASTRAL_CHECK(host->Assets().Reload("scenes/main.scene.json"));
        Require(host->LoadScene("scenes/main.scene.json", error), error);
        host->Update(0, {});
    }
};

void CheckNativeCounts(const GPU::FrameStats& stats) {
    ASTRAL_CHECK(stats.draws == 4 && stats.vertices == 76 && stats.triangles == 22 && stats.textureCount == 1);
    ASTRAL_CHECK(stats.resourceBytes <= 256 * kMiB && stats.inFlightFrames <= 2 && stats.queryCount <= 12);
}
void CheckAuthoredCounts(Framework::GameHost& host, int width = kWidth, int height = kHeight) {
    RenderScene scene; RenderView view;
    ASTRAL_CHECK(host.PrepareRenderScene(width, height, scene, view));
    std::size_t vertices = 0, triangles = 0; std::size_t textures = 0;
    ASTRAL_CHECK(scene.draws.size() == 4);
    for (const auto& draw : scene.draws) {
        ASTRAL_CHECK(draw.mesh && draw.material && draw.material->shading == ShadingModel::Unlit && draw.material->blend == BlendMode::Opaque);
        vertices += draw.mesh->vertices.size(); triangles += draw.mesh->TriangleCount();
        textures += draw.material->baseTexture != nullptr;
    }
    ASTRAL_CHECK(vertices == 76 && triangles == 22 && textures == 1);
    std::printf("Native authored workload: %dx%d submitted draws=4 vertices=%zu triangles=%zu textures=%zu\n", width, height, vertices, triangles, textures);
}

ImageRgba8 NativeDisplayReference(const std::vector<Color>& hdr, int width, int height, const ImageRgba8& overlay, float exposure) {
    ASTRAL_CHECK(hdr.size() == static_cast<std::size_t>(width) * height && overlay.width == width && overlay.height == height);
    ImageRgba8 image; image.Resize(width, height);
    for (std::size_t p = 0; p < hdr.size(); ++p) {
        const Color linear = ToneMapNeutral(Math::Max(hdr[p] * exposure, Color{}));
        const Color encoded{LinearToSrgb(linear.x), LinearToSrgb(linear.y), LinearToSrgb(linear.z)};
        const float values[] = {encoded.x, encoded.y, encoded.z};
        const float alpha = overlay.pixels[p * 4 + 3] / 255.0f;
        for (int c = 0; c < 3; ++c) image.pixels[p * 4 + c] = static_cast<std::uint8_t>(std::clamp(
            static_cast<int>(std::lround(values[c] * 255.0f * (1 - alpha) + overlay.pixels[p * 4 + c] * alpha)), 0, 255));
        image.pixels[p * 4 + 3] = 255;
    }
    return image;
}
void CheckNativeDisplayAndCapture(const RenderTarget& reference, const GPU::PresentationCapture& capture) {
    const auto expected = NativeDisplayReference(capture.geometry.hdr, capture.display.width, capture.display.height, capture.overlay, 1);
    const auto cpuDisplay = NativeDisplayReference(reference.hdr, capture.display.width, capture.display.height, capture.overlay, 1);
    ImageRgba8 difference, ids;
    difference.Resize(expected.width, expected.height); ids.Resize(expected.width, expected.height);
    double maximum = 0, sum = 0;
    for (std::size_t p = 0; p < capture.geometry.hdr.size(); ++p) {
        for (int c = 0; c < 3; ++c) {
            const double delta = std::abs(static_cast<int>(expected.pixels[p * 4 + c]) - static_cast<int>(capture.display.pixels[p * 4 + c]));
            maximum = std::max(maximum, delta); sum += delta;
            difference.pixels[p * 4 + c] = static_cast<std::uint8_t>(std::min(delta * 16, 255.0));
        }
        ASTRAL_CHECK(capture.display.pixels[p * 4 + 3] == 255);
        difference.pixels[p * 4 + 3] = 255;
        const auto id = capture.geometry.objectId[p];
        ids.Set(static_cast<int>(p % expected.width), static_cast<int>(p / expected.width),
            id ? Rgba8{static_cast<std::uint8_t>(40 + id * 53 % 210), static_cast<std::uint8_t>(40 + id * 97 % 210),
                static_cast<std::uint8_t>(40 + id * 139 % 210), 255} : Rgba8{0, 0, 0, 255});
    }
    const double mean = sum / (capture.geometry.hdr.size() * 3);
    std::printf("Actual swapchain display versus independent CPU neutral/UI reference: max=%.3f mean=%.6f byte values, frame=%llu\n",
        maximum, mean, static_cast<unsigned long long>(capture.geometry.stats.frameId));
    const char* directory = std::getenv("ASTRAL_CAPTURE_DIR");
    Require(directory && *directory, "Native hardware acceptance requires a scoped ASTRAL_CAPTURE_DIR");
    const auto path = std::filesystem::u8path(directory);
    std::filesystem::create_directories(path);
    std::string error;
    Require(WritePng((path / "native-cpu-geometry-display.png").u8string(), cpuDisplay, error), error);
    Require(WritePng((path / "native-swapchain.png").u8string(), capture.display, error), error);
    Require(WritePng((path / "native-display-reference.png").u8string(), expected, error), error);
    Require(WritePng((path / "native-display-difference-x16.png").u8string(), difference, error), error);
    Require(WritePng((path / "native-object-ids.png").u8string(), ids, error), error);
    Require(WritePng((path / "native-cpu-ui-only.png").u8string(), capture.overlay, error, true), error);
    ASTRAL_CHECK(maximum <= 2 && mean <= 0.5);
}
}

ASTRAL_TEST(NativeValidationRejectsUnsupportedBeforeDeviceCreation) {
    GPU::D3D11Renderer renderer;
    Fixture fixture; RenderScene scene; RenderView view; fixture.Build(scene, view);
    std::string error;
    ASTRAL_CHECK(renderer.ValidateOpaqueUnlit(scene, view, error) && !renderer.Ready() && !renderer.HasWindow());
    const Color hugeClear{std::numeric_limits<float>::max(), 0, 0};
    scene.post.exposure = 1;
    ASTRAL_CHECK(renderer.ValidateOpaqueUnlit(scene, view, error, hugeClear) && !renderer.Ready());
    scene.post.exposure = 2;
    ASTRAL_CHECK(!renderer.ValidateOpaqueUnlit(scene, view, error, hugeClear)
        && error.find("Clear color") != std::string::npos && error.find("overflow") != std::string::npos && !renderer.Ready());
    ASTRAL_CHECK(renderer.ValidateOpaqueUnlit(scene, view, error) && !renderer.Ready());
    scene.post.exposure = 0;
    ASTRAL_CHECK(renderer.ValidateOpaqueUnlit(scene, view, error, hugeClear) && !renderer.Ready());
    for (float invalid : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()}) {
        ASTRAL_CHECK(!renderer.ValidateOpaqueUnlit(scene, view, error, {0, invalid, 0})
            && error.find("clear color") != std::string::npos && !renderer.Ready() && !renderer.HasWindow());
    }
    scene.post.exposure = 1;
    RenderView oversized = view; oversized.width = 3840; oversized.height = 2160;
    ASTRAL_CHECK(!renderer.ValidateOpaqueUnlit(scene, oversized, error)
        && error.find("pixel/dimension budget") != std::string::npos && !renderer.Ready() && !renderer.HasWindow());
    fixture.boxMaterial->name = "Authored Toon"; fixture.boxMaterial->shading = ShadingModel::Toon;
    ASTRAL_CHECK(!renderer.ValidateOpaqueUnlit(scene, view, error) && error.find("Authored Toon") != std::string::npos);
    ASTRAL_CHECK(error.find("Unlit") != std::string::npos && !renderer.Ready());
    fixture.boxMaterial->shading = ShadingModel::Unlit;
    fixture.boxMaterial->emissive = {std::numeric_limits<float>::max(), 0, 0}; scene.post.exposure = 100;
    ASTRAL_CHECK(!renderer.ValidateOpaqueUnlit(scene, view, error) && error.find("overflow") != std::string::npos && !renderer.Ready());
    view.width = 0;
    ASTRAL_CHECK(!renderer.ValidateOpaqueUnlit(scene, view, error) && !renderer.Ready());
}

ASTRAL_TEST(NativeAuthoredHostPresentsActualSwapchainAndCpuUi) {
    MemoryGuard(); NativeFixture fixture; NativeWindow window;
    CheckAuthoredCounts(*fixture.host);
    Platform::D3D11Viewport viewport; std::string error;
    Require(viewport.Activate(window.window, *fixture.host, error), error);
    ASTRAL_CHECK(viewport.Renderer().Info().hardware);
    {
        RenderScene scene; RenderView view;
        ASTRAL_CHECK(fixture.host->PrepareRenderScene(kWidth, kHeight, scene, view));
        scene.post.exposure = 2;
        GPU::FrameStats unchanged; unchanged.frameId = 73;
        GPU::PresentationCapture untouched; untouched.geometry.width = 91;
        ImageRgba8 noOverlay;
        ASTRAL_CHECK(viewport.Renderer().PresentOpaqueUnlit(scene, view, noOverlay, unchanged, error, &untouched,
            {std::numeric_limits<float>::max(), 0, 0}) == GPU::RenderStatus::Failed);
        ASTRAL_CHECK(error.find("Clear color") != std::string::npos && error.find("overflow") != std::string::npos);
        ASTRAL_CHECK(unchanged.frameId == 73 && untouched.geometry.width == 91 && fixture.host->Stats().cpuSceneRenders == 0);
        std::vector<GPU::FrameStats> noSubmission;
        Require(viewport.CollectCompletedFrames(noSubmission, error, true), error);
        ASTRAL_CHECK(noSubmission.empty() && viewport.Renderer().Ready() && viewport.Renderer().HasWindow());
    }
    fixture.host->Console().Open(); fixture.host->Console().Print("Original authored meshes; CPU UI overlay");
    fixture.host->Update(0, {});
    Platform::NativeViewportStats stats; GPU::PresentationCapture capture;
    Require(viewport.Render(*fixture.host, stats, error, &capture) == GPU::RenderStatus::Rendered, error);
    CheckNativeCounts(stats.gpu);
    ASTRAL_CHECK(stats.cpuSceneRendersBefore == stats.cpuSceneRendersAfter && fixture.host->Stats().cpuSceneRenders == 0);
    ASTRAL_CHECK(capture.display.width == kWidth && capture.display.height == kHeight);
    std::printf("Actual native HWND capture: %s capture=%dx%d\n", viewport.Diagnostics().c_str(), capture.display.width, capture.display.height);
    RenderScene scene; RenderView view; ASTRAL_CHECK(fixture.host->PrepareRenderScene(kWidth, kHeight, scene, view));
    Neutral(scene); RenderTarget cpu; Reference(scene, view, cpu);
    for (std::size_t p = 0; p < cpu.hdr.size(); ++p) if (!cpu.objectId[p]) cpu.hdr[p] = {0.02f, 0.025f, 0.04f};
    CheckNativeDisplayAndCapture(cpu, capture);
    Compare(cpu, capture.geometry, true);
    std::vector<GPU::FrameStats> completed;
    Require(viewport.CollectCompletedFrames(completed, error, true), error);
    Require(viewport.Deactivate(error), error);
    ASTRAL_CHECK(!viewport.Active() && !viewport.Renderer().HasWindow());
}

ASTRAL_TEST(NativeUnsupportedReloadPreservesAuthoredMaterialAndReturnsToCpu) {
    NativeFixture fixture; NativeWindow window(320, 180);
    auto changed = fixture.SceneJson();
    changed.Find("materials")->Find("boxUnlit")->Set("shading", "Toon");
    fixture.Reload(changed);
    Platform::D3D11Viewport viewport; std::string error;
    ASTRAL_CHECK(!viewport.Activate(window.window, *fixture.host, error));
    ASTRAL_CHECK(!viewport.Active() && !viewport.Renderer().Ready() && error.find("boxUnlit") != std::string::npos);
    changed.Find("materials")->Find("boxUnlit")->Set("shading", "Unlit"); fixture.Reload(changed);
    Require(viewport.Activate(window.window, *fixture.host, error), error);
    Platform::NativeViewportStats stats;
    Require(viewport.Render(*fixture.host, stats, error) == GPU::RenderStatus::Rendered, error);
    changed.Find("materials")->Find("boxUnlit")->Set("shading", "Toon"); fixture.Reload(changed);
    ASTRAL_CHECK(viewport.Render(*fixture.host, stats, error) == GPU::RenderStatus::Failed && error.find("Unlit") != std::string::npos);
    Require(viewport.Deactivate(error), error); // same explicit return used by native Tools
    RenderScene scene; RenderView view; ASTRAL_CHECK(fixture.host->PrepareRenderScene(320, 180, scene, view));
    bool authoredToon = false;
    for (const auto& draw : scene.draws) if (draw.material->name == "boxUnlit") authoredToon = draw.material->shading == ShadingModel::Toon;
    ASTRAL_CHECK(authoredToon && fixture.host->Stats().cpuSceneRenders == 0);
    fixture.host->Render(320, 180);
    ASTRAL_CHECK(fixture.host->Stats().cpuSceneRenders == 1 && fixture.host->Stats().renderBackend == "CPU");
}

namespace {
void DebugLine(const std::string& line) { OutputDebugStringA((line + '\n').c_str()); }

bool CaptureLifetimeChildStreams() {
    const char* directory = std::getenv("ASTRAL_CAPTURE_DIR");
    if (!directory || !*directory) { DebugLine("ASTRAL_LIFETIME_CRT_ERROR missing scoped capture directory"); return false; }
    std::error_code failure;
    const auto path = std::filesystem::canonical(std::filesystem::u8path(directory), failure);
    if (failure || !std::filesystem::is_directory(path)) { DebugLine("ASTRAL_LIFETIME_CRT_ERROR invalid capture directory"); return false; }
    FILE* captured = nullptr;
    const auto output = path / "native-lifetime-child.stdout.log";
    const int outputError = _wfreopen_s(&captured, output.c_str(), L"wbx", stdout);
    if (outputError || !captured || std::setvbuf(stdout, nullptr, _IONBF, 0)) {
        DebugLine("ASTRAL_LIFETIME_CRT_ERROR stdout code=" + std::to_string(outputError)); return false;
    }
    const auto errors = path / "native-lifetime-child.stderr.log";
    const int errorCode = _wfreopen_s(&captured, errors.c_str(), L"wbx", stderr);
    if (errorCode || !captured || std::setvbuf(stderr, nullptr, _IONBF, 0)) {
        DebugLine("ASTRAL_LIFETIME_CRT_ERROR stderr code=" + std::to_string(errorCode)); return false;
    }
    DebugLine("ASTRAL_LIFETIME_CRT stdout=" + output.u8string() + " stderr=" + errors.u8string());
    return true;
}

int NativeLifetimeProbe() {
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    if (!CaptureLifetimeChildStreams()) return 125;
    NativeFixture fixture; NativeWindow window(320, 180);
    Platform::D3D11Viewport viewport; std::string error;
    Initialize(viewport.Renderer());
    ASTRAL_CHECK(viewport.Renderer().Info().debugLayer);
    // Warm lazy driver state once, then compare equivalent initialized,
    // quiescent states with no native window/frame resources attached.
    Require(viewport.Activate(window.window, *fixture.host, error), error);
    Platform::NativeViewportStats stats;
    Require(viewport.Render(*fixture.host, stats, error) == GPU::RenderStatus::Rendered, error);
    DebugLine("ASTRAL_NATIVE_ADAPTER " + viewport.Diagnostics());
    std::vector<GPU::FrameStats> completed;
    Require(viewport.CollectCompletedFrames(completed, error, true), error);
    Require(viewport.Deactivate(error), error);
    DebugLine("ASTRAL_LIVE_BASELINE_BEGIN");
    Require(viewport.Renderer().ReportLiveObjects(error), error);
    DebugLine("ASTRAL_LIVE_BASELINE_END");
    for (int cycle = 0; cycle < 20; ++cycle) {
        MemoryGuard();
        if (cycle % 5 == 0) {
            ImageRgba8 texture; texture.Resize(8, 8);
            for (int y = 0; y < 8; ++y) for (int x = 0; x < 8; ++x)
                texture.Set(x, y, {static_cast<std::uint8_t>(40 + cycle * 4), static_cast<std::uint8_t>(80 + x * 15),
                    static_cast<std::uint8_t>(100 + y * 12), 255});
            Require(WritePng((fixture.root / "textures/GroundChecker.png").u8string(), texture, error), error);
            ASTRAL_CHECK(fixture.host->Assets().Reload("textures/GroundChecker.png"));
        }
        auto json = fixture.SceneJson();
        Core::JsonValue position = Core::JsonValue::MakeArray();
        position.Append(cycle * 0.025); position.Append(2.0); position.Append(0.0);
        json.Find("entities")->Items()[2].Find("transform")->Set("position", std::move(position));
        fixture.Reload(std::move(json));
        window.Resize(cycle & 1 ? 640 : 320, cycle & 1 ? 360 : 180, "initial", cycle + 1);
        Require(viewport.Activate(window.window, *fixture.host, error), error);
        Require(viewport.Render(*fixture.host, stats, error) == GPU::RenderStatus::Rendered, error);
        CheckNativeCounts(stats.gpu);
        ASTRAL_CHECK(stats.cpuSceneRendersBefore == stats.cpuSceneRendersAfter);
        ShowWindow(window.window, SW_MINIMIZE); PumpNativeMessages();
        const auto previous = stats.gpu.frameId;
        ASTRAL_CHECK(viewport.Render(*fixture.host, stats, error) == GPU::RenderStatus::Skipped);
        ASTRAL_CHECK(stats.gpu.frameId == previous); // no frame/query allocation on minimize
        ShowWindow(window.window, SW_RESTORE); PumpNativeMessages();
        window.Resize(0, 0, "zero", cycle + 1);
        ASTRAL_CHECK(viewport.Render(*fixture.host, stats, error) == GPU::RenderStatus::Skipped && stats.gpu.frameId == previous);
        window.Resize(320, 180, "restore", cycle + 1);
        Require(viewport.Render(*fixture.host, stats, error) == GPU::RenderStatus::Rendered, error);
        CheckNativeCounts(stats.gpu);
        Require(viewport.CollectCompletedFrames(completed, error, true), error);
        Require(viewport.Deactivate(error), error);
        ASTRAL_CHECK(!viewport.Active() && !viewport.Renderer().HasWindow() && GetWindow(window.window, GW_CHILD) == nullptr);
        ASTRAL_CHECK(ProcessMemory().PrivateUsage <= 768 * kMiB);
        DebugLine("ASTRAL_NATIVE_CYCLE " + std::to_string(cycle + 1) + " complete; submitted=4/76/22/1; CPU scene raster delta=0; detached");
    }
    ASTRAL_CHECK(fixture.host->Stats().cpuSceneRenders == 0);
    DebugLine("ASTRAL_LIVE_AFTER_BEGIN");
    Require(viewport.Renderer().ReportLiveObjects(error), error);
    DebugLine("ASTRAL_LIVE_AFTER_END");
    return 0;
}

std::string ReadOwnedDebugString(HANDLE process, const OUTPUT_DEBUG_STRING_INFO& info) {
    const std::size_t unit = info.fUnicode ? sizeof(wchar_t) : 1;
    const std::size_t bytes = static_cast<std::size_t>(info.nDebugStringLength) * unit;
    if (!bytes || bytes > 128u * 1024u) return {};
    std::vector<char> data(bytes + sizeof(wchar_t), 0); SIZE_T read{};
    if (!ReadProcessMemory(process, info.lpDebugStringData, data.data(), bytes, &read) || read != bytes) return "<owned debug string read failed>\n";
    if (!info.fUnicode) return std::string(data.begin(), std::find(data.begin(), data.begin() + bytes, '\0'));
    std::wstring wide(bytes / sizeof(wchar_t), L'\0');
    std::memcpy(wide.data(), data.data(), bytes);
    while (!wide.empty() && wide.back() == L'\0') wide.pop_back();
    if (wide.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "<owned Unicode debug conversion failed>\n";
    std::string text(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), text.data(), size, nullptr, nullptr);
    return text;
}

std::string RunOwnedLifetimeProbe() {
    std::vector<wchar_t> executable(32768);
    const DWORD length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    ASTRAL_CHECK(length > 0 && length < executable.size());
    std::wstring command = L"\"" + std::wstring(executable.data(), length) + L"\" --native-lifetime-probe";
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION child{};
    ASTRAL_CHECK(CreateProcessW(executable.data(), command.data(), nullptr, nullptr, FALSE,
        DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child));
    CloseHandle(child.hThread); // caller's handle; debug-event thread handles are OS-owned
    const auto started = NativeClock::now();
    auto deadline = started + std::chrono::seconds(60);
    bool timedOut = false, exited = false, initialBreakpoint = true, continuationFailed = false;
    DWORD exitCode = STILL_ACTIVE;
    std::string output;
    while (!exited) {
        if (NativeClock::now() >= deadline) {
            if (timedOut) break;
            timedOut = true;
            if (WaitForSingleObject(child.hProcess, 0) == WAIT_TIMEOUT) TerminateProcess(child.hProcess, 124);
            deadline = NativeClock::now() + std::chrono::seconds(10); // only this still-live owned child
        }
        DEBUG_EVENT event{};
        if (!WaitForDebugEvent(&event, 100)) {
            if (GetLastError() != ERROR_SEM_TIMEOUT && WaitForSingleObject(child.hProcess, 0) == WAIT_OBJECT_0) break;
            continue;
        }
        DWORD continuation = DBG_CONTINUE;
        if (event.dwProcessId != child.dwProcessId) continuationFailed = true;
        switch (event.dwDebugEventCode) {
        case CREATE_PROCESS_DEBUG_EVENT:
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
            break;
        case LOAD_DLL_DEBUG_EVENT:
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
            break;
        case OUTPUT_DEBUG_STRING_EVENT:
            output += ReadOwnedDebugString(child.hProcess, event.u.DebugString);
            if (output.size() > 16 * kMiB) {
                continuationFailed = true;
                if (WaitForSingleObject(child.hProcess, 0) == WAIT_TIMEOUT) TerminateProcess(child.hProcess, 125);
            }
            break;
        case EXCEPTION_DEBUG_EVENT:
            if (initialBreakpoint && event.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT) initialBreakpoint = false;
            else continuation = DBG_EXCEPTION_NOT_HANDLED;
            break;
        case EXIT_PROCESS_DEBUG_EVENT:
            exitCode = event.u.ExitProcess.dwExitCode; exited = true;
            break;
        default: break;
        }
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, continuation)) continuationFailed = true;
    }
    if (!exited && WaitForSingleObject(child.hProcess, 0) == WAIT_TIMEOUT) {
        TerminateProcess(child.hProcess, 124);
        DebugActiveProcessStop(child.dwProcessId); // detach only this owned failed child so it can finish exiting
    }
    const DWORD reaped = WaitForSingleObject(child.hProcess, 10000);
    DWORD actual = STILL_ACTIVE; const BOOL queried = GetExitCodeProcess(child.hProcess, &actual);
    CloseHandle(child.hProcess);
    std::printf("Owned native lifetime probe pid=%lu elapsed=%.3f timedOut=%d exitEvent=%d eventCode=%lu actualExit=%lu reaped=%d continuationFailed=%d\n",
        child.dwProcessId, std::chrono::duration<double>(NativeClock::now() - started).count(), timedOut, exited, exitCode, actual,
        reaped == WAIT_OBJECT_0, continuationFailed);
    std::fwrite(output.data(), 1, output.size(), stdout); std::fflush(stdout);
    if (const char* directory = std::getenv("ASTRAL_CAPTURE_DIR")) {
        std::ofstream log(std::filesystem::u8path(directory) / "native-live-objects.log", std::ios::binary);
        log.write(output.data(), static_cast<std::streamsize>(output.size()));
    }
    ASTRAL_CHECK(!timedOut && exited && !continuationFailed && queried && actual == 0 && exitCode == 0 && reaped == WAIT_OBJECT_0);
    return output;
}

std::map<std::string, int> LiveCounts(const std::string& output, const char* begin, const char* end) {
    const auto start = output.find(begin), finish = output.find(end);
    ASTRAL_CHECK(start != std::string::npos && finish != std::string::npos && finish > start);
    const std::string report = output.substr(start, finish - start);
    std::map<std::string, int> counts;
    const std::regex live("Live (ID3D[^ ]+) at ");
    for (std::sregex_iterator item(report.begin(), report.end(), live), last; item != last; ++item) ++counts[(*item)[1].str()];
    ASTRAL_CHECK(!counts.empty() && counts["ID3D11Device"] > 0 && counts["ID3D11Buffer"] > 0);
    return counts;
}
}

ASTRAL_TEST(NativeTwentyCyclesHaveActualEquivalentDebugLayerLiveReports) {
    MemoryGuard();
    const auto output = RunOwnedLifetimeProbe();
    const auto baseline = LiveCounts(output, "ASTRAL_LIVE_BASELINE_BEGIN", "ASTRAL_LIVE_BASELINE_END");
    const auto after = LiveCounts(output, "ASTRAL_LIVE_AFTER_BEGIN", "ASTRAL_LIVE_AFTER_END");
    ASTRAL_CHECK(output.find("ASTRAL_NATIVE_CYCLE 20 complete") != std::string::npos);
    for (const auto& item : baseline) std::printf("Actual live type %s baseline=%d after=%d\n", item.first.c_str(), item.second,
        after.count(item.first) ? after.at(item.first) : 0);
    ASTRAL_CHECK(baseline == after);
}

ASTRAL_TEST(NativeFixed720pFullApplicationFrameMeetsDeclared60HzTarget) {
    MemoryGuard(); NativeFixture fixture; NativeWindow window;
    CheckAuthoredCounts(*fixture.host);
    Platform::D3D11Viewport viewport; std::string error;
    Require(viewport.Activate(window.window, *fixture.host, error), error);
    std::printf("Native fixed workload metadata: %s; 30 warmup+120 measured; fixed Update dt=0; submitted4/76/22/1\n", viewport.Diagnostics().c_str());
    std::vector<double> full, prepare, validation, ui, upload, overlay, submit, present;
    std::vector<GPU::FrameStats> submittedFrames;
    submittedFrames.reserve(120);
    std::map<std::uint64_t, double> measuredIds;
    std::vector<GPU::FrameStats> completed;
    for (int frame = 0; frame < 150; ++frame) {
        const auto begin = NativeClock::now(); // complete loop, including the periodic safety observation
        if (frame % 30 == 0) MemoryGuard();
        PumpNativeMessages();
        fixture.host->Update(0, {});
        Platform::NativeViewportStats stats;
        Require(viewport.Render(*fixture.host, stats, error, nullptr, &completed) == GPU::RenderStatus::Rendered, error);
        CheckNativeCounts(stats.gpu);
        ASTRAL_CHECK(stats.cpuSceneRendersBefore == stats.cpuSceneRendersAfter && fixture.host->Stats().cpuSceneRenders == 0);
        const double elapsed = std::chrono::duration<double, std::milli>(NativeClock::now() - begin).count();
        if (frame >= 30) {
            ASTRAL_CHECK(measuredIds.emplace(stats.gpu.frameId, elapsed).second);
            full.push_back(elapsed); prepare.push_back(stats.scenePrepareMs); ui.push_back(stats.cpuUiMs);
            validation.push_back(stats.gpu.validationMs);
            upload.push_back(stats.gpu.uploadMs); overlay.push_back(stats.gpu.overlayUploadMs);
            submit.push_back(stats.gpu.submissionMs); present.push_back(stats.gpu.presentMs);
            submittedFrames.push_back(stats.gpu);
        }
    }
    std::vector<GPU::FrameStats> tail;
    Require(viewport.CollectCompletedFrames(tail, error, true), error);
    completed.insert(completed.end(), tail.begin(), tail.end());
    std::vector<double> gpu, gpuUpload, scene, display;
    std::map<std::uint64_t, bool> seen;
    for (const auto& timing : completed) {
        if (!measuredIds.count(timing.frameId)) continue;
        ASTRAL_CHECK(timing.gpuTimingValid && seen.emplace(timing.frameId, true).second);
        gpu.push_back(timing.gpuMs); gpuUpload.push_back(timing.gpuUploadMs); scene.push_back(timing.gpuSceneMs); display.push_back(timing.gpuDisplayMs);
        std::printf("Native actual completed GPU frame=%llu total=%.6f upload=%.6f scene=%.6f display=%.6f localUsage=%llu localBudget=%llu available=%d\n",
            static_cast<unsigned long long>(timing.frameId), timing.gpuMs, timing.gpuUploadMs, timing.gpuSceneMs, timing.gpuDisplayMs,
            static_cast<unsigned long long>(timing.localVideoUsage), static_cast<unsigned long long>(timing.localVideoBudget), timing.videoMemoryAvailable);
    }
    ASTRAL_CHECK(full.size() == 120 && gpu.size() == 120 && seen.size() == measuredIds.size());
    for (std::size_t i = 0; i < full.size(); ++i) std::printf(
        "Native measured frame=%llu full=%.6f prepare=%.6f ui=%.6f upload=%.6f submit=%.6f present=%.6f logicalBytes=%zu queryCount=%zu\n",
        static_cast<unsigned long long>(submittedFrames[i].frameId), full[i], prepare[i], ui[i], upload[i], submit[i], present[i],
        submittedFrames[i].resourceBytes, submittedFrames[i].queryCount);
    PrintTimes("Native GPU work (upload+indexed scene+display)", gpu);
    PrintTimes("Native GPU upload interval", gpuUpload);
    PrintTimes("Native GPU scene interval", scene); PrintTimes("Native GPU display interval", display);
    PrintTimes("Native CPU scene prepare", prepare); PrintTimes("Native CPU UI draw", ui);
    PrintTimes("Native CPU support validation", validation); PrintTimes("Native CPU prepare/upload", upload); PrintTimes("Native CPU UI upload", overlay);
    PrintTimes("Native CPU submit", submit); PrintTimes("Native Present(1) wait", present);
    PrintTimes("Native presentation-inclusive full application frame", full);
    std::printf("Native fixed workload end metadata: %s\n", viewport.Diagnostics().c_str());
    std::fflush(stdout);
    Require(viewport.Deactivate(error), error);
    ASTRAL_CHECK(ProcessMemory().PrivateUsage <= 768 * kMiB);
    ASTRAL_CHECK(Percentile(gpu, .95) <= 16.67 && Percentile(full, .95) <= 16.67);
}

int main(int argc, char** argv) {
    if (argc == 2 && std::strcmp(argv[1], "--native-lifetime-probe") == 0) return NativeLifetimeProbe();
    if (argc == 2 && std::strcmp(argv[1], "--compare-nonfinite-probe") == 0) {
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        return CompareProbe("cpu", "nan", false);
    }
    if (argc == 5 && std::strcmp(argv[1], "--compare-probe") == 0) {
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        ASTRAL_CHECK(std::strcmp(argv[4], "0") == 0 || std::strcmp(argv[4], "1") == 0);
        return CompareProbe(argv[2], argv[3], std::strcmp(argv[4], "1") == 0);
    }
    // A fresh checkout can run the hardware suite without external evidence folders.
    // Preserve explicit caller paths; children inherit the same scoped capture root.
    if (!std::getenv("ASTRAL_NATIVE_GPU_FIXTURE")) {
        if (_putenv_s("ASTRAL_NATIVE_GPU_FIXTURE", ASTRAL_TEST_NATIVE_GPU_FIXTURE) != 0) return 2;
    }
    if (!std::getenv("ASTRAL_CAPTURE_DIR")) {
        const auto stamp = NativeClock::now().time_since_epoch().count();
        const auto root = std::filesystem::current_path() / "native-gpu-captures"
            / (std::to_string(GetCurrentProcessId()) + "-" + std::to_string(stamp));
        std::filesystem::create_directories(root.parent_path());
        if (!std::filesystem::create_directory(root)) return 2;
        if (_putenv_s("ASTRAL_CAPTURE_DIR", root.u8string().c_str()) != 0) return 2;
        std::printf("Native GPU capture directory: %s\n", root.u8string().c_str());
    }
    return Test::RunAll(argc, argv, "EngineD3D11Tests");
}
