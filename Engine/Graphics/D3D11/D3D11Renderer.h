#pragma once

// Optional Windows backend; its public boundary has no Windows/D3D headers.
#include "Engine/Graphics/RenderScene.h"
#include "Engine/Graphics/Image.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Astral::Graphics::D3D11 {

struct Limits {
    std::size_t maxDraws{256};
    std::size_t maxGeometryBytes{32u * 1024u * 1024u};
    std::size_t maxTextureBytes{64u * 1024u * 1024u};
    std::size_t maxPixels{1920u * 1080u};
    std::size_t maxResourceBytes{256u * 1024u * 1024u};
};

struct DeviceInfo {
    std::string adapter;
    std::uint32_t vendorId{}, deviceId{}, featureLevel{};
    std::uint64_t adapterLuid{};
    bool hardware{}, debugLayer{};
    std::string diagnostics;
};

struct FrameStats {
    std::size_t draws{}, triangles{}, vertices{}, textureCount{};
    std::size_t resourceBytes{}, resourceCount{}; // logical GPU allocations, not driver residency
    double uploadMs{}, submissionMs{}, completionReadbackMs{}, totalMs{};
    double gpuMs{};
    bool gpuTimingValid{};
    bool videoMemoryAvailable{};
    std::uint64_t localVideoUsage{}, localVideoBudget{};
    std::uint64_t frameId{}; // native presentation identity; zero for the offscreen API
    double gpuUploadMs{}, gpuSceneMs{}, gpuDisplayMs{}, validationMs{}, overlayUploadMs{}, presentMs{};
    std::size_t inFlightFrames{}, queryCount{};
    bool occluded{};
};

struct Frame {
    int width{}, height{};
    std::vector<Color> hdr; // row-major, linear HDR; background is the supplied clear color
    std::vector<std::uint32_t> objectId; // 0 = background; DrawItem ID or draw index + 1
    FrameStats stats;
};

struct PresentationCapture {
    Frame geometry;
    ImageRgba8 display; // actual swap-chain backbuffer, before this frame's Present
    ImageRgba8 overlay; // exact transparent CPU UI bytes uploaded for this frame
};

enum class RenderStatus { Failed, Skipped, Rendered };

// All calls and destruction belong to the same thread; input resources must not
// mutate during Render. Nothing borrowed from a RenderScene survives that call.
// This is only the opaque-Unlit pass; it never processes environment, lighting,
// fog, shadows or post. Other material/primitive passes are rejected explicitly.
class D3D11Renderer {
public:
    D3D11Renderer();
    ~D3D11Renderer();
    D3D11Renderer(const D3D11Renderer&) = delete;
    D3D11Renderer& operator=(const D3D11Renderer&) = delete;

    // Optional debug-layer absence is diagnosed; no WARP/software fallback.
    // Reinitialization is explicit; failed Initialize leaves the renderer empty.
    bool Initialize(std::string& error, Limits limits = {}, bool requestDebugLayer = true);
    void Shutdown();
    bool Ready() const;
    const DeviceInfo& Info() const;
    // A zero viewport skips without allocation/output mutation. Invalid input,
    // failed upload/readback or device failure leaves output unchanged.
    RenderStatus RenderOpaqueUnlit(const RenderScene& scene, const RenderView& view,
        Frame& output, std::string& error, Color clear = {});

    // CPU validation only; no device/shader/window is created. The caller owns
    // all inputs and no borrowed pointer survives this call.
    bool ValidateOpaqueUnlit(const RenderScene& scene, const RenderView& view, std::string& error,
        Color clear = {0.02f, 0.025f, 0.04f});
    // nativeWindow is an owned, dedicated GPU-only HWND, used on this thread.
    // Caller retains the HWND until successful DetachWindow/Shutdown.
    bool AttachWindow(void* nativeWindow, std::string& error);
    bool DetachWindow(std::string& error);
    bool HasWindow() const;
    RenderStatus PresentOpaqueUnlit(const RenderScene& scene, const RenderView& view,
        const ImageRgba8& overlay, FrameStats& stats, std::string& error,
        PresentationCapture* capture = nullptr, Color clear = {0.02f, 0.025f, 0.04f});
    // Returns each completed native timestamp exactly once. Normal consumers
    // poll every frame; tests may drain, with the same bounded GPU wait.
    bool CollectCompletedFrames(std::vector<FrameStats>& frames, std::string& error, bool drain = false);
    std::string PresentationDiagnostics() const;
    // Emits the actual debug-layer live-device report, not logical counters.
    // A test may capture its debug output in an owned child process.
    bool ReportLiveObjects(std::string& error) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Astral::Graphics::D3D11
