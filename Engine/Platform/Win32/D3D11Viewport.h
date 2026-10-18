#pragma once

#include "Engine/Graphics/D3D11/D3D11Renderer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Astral::Framework { class GameHost; }

namespace Astral::Platform {

struct NativeViewportStats {
    Graphics::D3D11::FrameStats gpu;
    double scenePrepareMs{}, cpuUiMs{}, renderWallMs{};
    std::uint64_t cpuSceneRendersBefore{}, cpuSceneRendersAfter{};
};

// Shared native consumer. It owns a GPU-only child HWND and its renderer on one
// thread; the CPU/GDI parent and the host remain owned by the native application.
// No host/world/asset pointer is retained between calls.
class D3D11Viewport {
public:
    D3D11Viewport();
    ~D3D11Viewport();
    D3D11Viewport(const D3D11Viewport&) = delete;
    D3D11Viewport& operator=(const D3D11Viewport&) = delete;

    bool Activate(void* parentWindow, Framework::GameHost& host, std::string& error);
    bool Deactivate(std::string& error); // must precede parent/host destruction
    bool Active() const;
    // Queries the actual parent's full client size; there is no render-scale
    // parameter. Zero/minimized clients skip without allocating/drawing/Present.
    Graphics::D3D11::RenderStatus Render(Framework::GameHost& host,
        NativeViewportStats& stats, std::string& error,
        Graphics::D3D11::PresentationCapture* capture = nullptr,
        std::vector<Graphics::D3D11::FrameStats>* completed = nullptr);
    bool CollectCompletedFrames(std::vector<Graphics::D3D11::FrameStats>& frames,
        std::string& error, bool drain = false);
    std::string Diagnostics() const;
    Graphics::D3D11::D3D11Renderer& Renderer();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace Astral::Platform
