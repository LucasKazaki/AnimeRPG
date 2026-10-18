#pragma once

// Optional Windows backend; its public boundary has no Windows/D3D headers.
#include "Engine/Graphics/RenderScene.h"

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
};

struct Frame {
    int width{}, height{};
    std::vector<Color> hdr; // row-major, linear HDR; background is the supplied clear color
    std::vector<std::uint32_t> objectId; // 0 = background; DrawItem ID or draw index + 1
    FrameStats stats;
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

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Astral::Graphics::D3D11
