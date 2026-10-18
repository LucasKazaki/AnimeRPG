#include "Engine/Platform/Win32/D3D11Viewport.h"
#include "Engine/Framework/GameHost.h"

#include <windows.h>

#include <chrono>
#include <sstream>
#include <thread>

namespace Astral::Platform {
namespace {
using Clock = std::chrono::steady_clock;
using Graphics::D3D11::RenderStatus;
constexpr wchar_t kGpuWindowClass[] = L"AstralOpaqueUnlitGpuViewportV1";

double Elapsed(Clock::time_point from) {
    return std::chrono::duration<double, std::milli>(Clock::now() - from).count();
}
std::string Utf8(const wchar_t* text) {
    const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "<conversion failed>";
    std::string value(static_cast<std::size_t>(size), '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, text, -1, value.data(), size, nullptr, nullptr)) return "<conversion failed>";
    value.pop_back(); return value;
}
LRESULT CALLBACK GpuWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_NCHITTEST: return HTTRANSPARENT; // same-thread parent owns input
    case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: ValidateRect(window, nullptr); return 0; // no GDI on this HWND
    default: return DefWindowProcW(window, message, wParam, lParam);
    }
}
bool RegisterGpuWindow(std::string& error) {
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = GpuWindowProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = kGpuWindowClass;
    if (RegisterClassW(&windowClass)) return true;
    const DWORD failure = GetLastError();
    WNDCLASSW existing{};
    if (failure == ERROR_CLASS_ALREADY_EXISTS && GetClassInfoW(windowClass.hInstance, kGpuWindowClass, &existing)
        && existing.lpfnWndProc == GpuWindowProc) return true;
    error = "Cannot register GPU viewport class: Windows error " + std::to_string(failure); return false;
}
}

struct D3D11Viewport::Impl {
    std::thread::id owner{std::this_thread::get_id()};
    HWND parent{}, child{};
    Graphics::D3D11::D3D11Renderer renderer;
    bool Thread(std::string& error) const {
        if (owner == std::this_thread::get_id()) return true;
        error = "GPU viewport called outside its owner thread"; return false;
    }
};

D3D11Viewport::D3D11Viewport() : impl_(std::make_unique<Impl>()) {}
D3D11Viewport::~D3D11Viewport() {
    std::string ignored; Deactivate(ignored); impl_->renderer.Shutdown();
}

bool D3D11Viewport::Activate(void* parentWindow, Framework::GameHost& host, std::string& error) {
    error.clear();
    if (!impl_->Thread(error)) return false;
    HWND parent = static_cast<HWND>(parentWindow);
    if (!parent || !IsWindow(parent) || GetWindowThreadProcessId(parent, nullptr) != GetCurrentThreadId()) {
        error = "GPU viewport parent must belong to this thread"; return false;
    }
    RECT client{};
    if (!GetClientRect(parent, &client) || client.right <= 0 || client.bottom <= 0) {
        error = "GPU selection requires a nonzero client"; return false;
    }
    Graphics::RenderScene scene; Graphics::RenderView view;
    if (!host.PrepareRenderScene(client.right, client.bottom, scene, view)) {
        error = "No active camera for GPU selection"; return false;
    }
    // Strict support validation precedes every initial device/window creation.
    if (!impl_->renderer.ValidateOpaqueUnlit(scene, view, error)) return false;
    if (!Deactivate(error) || !RegisterGpuWindow(error)) return false;
    if (!impl_->renderer.Ready() && !impl_->renderer.Initialize(error)) return false;
    impl_->parent = parent;
    impl_->child = CreateWindowExW(WS_EX_NOACTIVATE, kGpuWindowClass, L"", WS_CHILD | WS_CLIPSIBLINGS,
        0, 0, client.right, client.bottom, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!impl_->child) { error = "Cannot create GPU viewport child: Windows error " + std::to_string(GetLastError()); impl_->parent = nullptr; return false; }
    if (!impl_->renderer.AttachWindow(impl_->child, error)) {
        DestroyWindow(impl_->child); impl_->child = impl_->parent = nullptr; return false;
    }
    host.SetExternalRenderStats({}, 0, "D3D11 Opaque Unlit");
    return true;
}

bool D3D11Viewport::Deactivate(std::string& error) {
    error.clear();
    if (!impl_->Thread(error)) return false;
    const bool detached = impl_->renderer.DetachWindow(error);
    if (impl_->child) { ShowWindow(impl_->child, SW_HIDE); DestroyWindow(impl_->child); }
    impl_->child = impl_->parent = nullptr;
    return detached;
}

bool D3D11Viewport::Active() const { return impl_->child && impl_->renderer.HasWindow(); }

RenderStatus D3D11Viewport::Render(Framework::GameHost& host, NativeViewportStats& stats,
    std::string& error, Graphics::D3D11::PresentationCapture* capture,
    std::vector<Graphics::D3D11::FrameStats>* completed) {
    error.clear();
    if (!impl_->Thread(error)) return RenderStatus::Failed;
    if (!Active()) { error = "GPU viewport is not active"; return RenderStatus::Failed; }
    RECT client{};
    if (!GetClientRect(impl_->parent, &client)) { error = "Cannot query native client size"; return RenderStatus::Failed; }
    if (IsIconic(GetAncestor(impl_->parent, GA_ROOT)) || client.right <= 0 || client.bottom <= 0) return RenderStatus::Skipped;
    const auto begin = Clock::now();
    stats = {};
    stats.cpuSceneRendersBefore = host.Stats().cpuSceneRenders;
    RECT childClient{};
    if (!GetClientRect(impl_->child, &childClient)) { error = "Cannot query GPU child size"; return RenderStatus::Failed; }
    if ((childClient.right != client.right || childClient.bottom != client.bottom)
        && !MoveWindow(impl_->child, 0, 0, client.right, client.bottom, FALSE)) {
        error = "Cannot resize GPU child: Windows error " + std::to_string(GetLastError()); return RenderStatus::Failed;
    }
    Graphics::RenderScene scene; Graphics::RenderView view;
    if (!host.PrepareRenderScene(client.right, client.bottom, scene, view)) {
        error = "No active camera after scene reload"; return RenderStatus::Failed;
    }
    // Admit the full-size frame before CPU UI allocation. Present validates
    // again at its public boundary; both validations remain in full-frame time.
    if (!impl_->renderer.ValidateOpaqueUnlit(scene, view, error)) return RenderStatus::Failed;
    stats.scenePrepareMs = Elapsed(begin);
    const auto uiBegin = Clock::now();
    const Graphics::ImageRgba8& overlay = host.RenderOverlay(client.right, client.bottom);
    stats.cpuUiMs = Elapsed(uiBegin);
    const RenderStatus result = impl_->renderer.PresentOpaqueUnlit(scene, view, overlay, stats.gpu, error, capture);
    if (result == RenderStatus::Failed) return result;
    stats.cpuSceneRendersAfter = host.Stats().cpuSceneRenders;
    stats.renderWallMs = Elapsed(begin);
    if (stats.cpuSceneRendersAfter != stats.cpuSceneRendersBefore) {
        error = "CPU scene rasterization occurred during a supported GPU frame"; return RenderStatus::Failed;
    }
    Graphics::RenderStats submitted;
    submitted.drawsSubmitted = stats.gpu.draws;
    submitted.trianglesSubmitted = stats.gpu.triangles;
    submitted.memoryBytes = stats.gpu.resourceBytes;
    host.SetExternalRenderStats(submitted, stats.renderWallMs, "D3D11 Opaque Unlit", stats.gpu.frameId);
    std::vector<Graphics::D3D11::FrameStats> ready;
    if (!impl_->renderer.CollectCompletedFrames(ready, error)) return RenderStatus::Failed;
    if (completed) completed->insert(completed->end(), ready.begin(), ready.end());
    if (result == RenderStatus::Rendered) ShowWindow(impl_->child, SW_SHOWNOACTIVATE);
    return result;
}

bool D3D11Viewport::CollectCompletedFrames(std::vector<Graphics::D3D11::FrameStats>& frames, std::string& error, bool drain) {
    return impl_->renderer.CollectCompletedFrames(frames, error, drain);
}

std::string D3D11Viewport::Diagnostics() const {
    std::ostringstream result;
    result << impl_->renderer.PresentationDiagnostics();
    result << " parentHwnd=" << impl_->parent << " gpuHwnd=" << impl_->child;
    RECT client{};
    if (impl_->parent && GetClientRect(impl_->parent, &client)) result << " hwndReportedClient=" << client.right << 'x' << client.bottom;
    // These are read-only metadata queries. Never change DPI awareness/settings.
    const HMODULE user = GetModuleHandleW(L"user32.dll");
    using GetContext = HANDLE(WINAPI*)();
    using GetAwareness = int(WINAPI*)(HANDLE);
    const auto context = reinterpret_cast<GetContext>(GetProcAddress(user, "GetThreadDpiAwarenessContext"));
    const auto awareness = reinterpret_cast<GetAwareness>(GetProcAddress(user, "GetAwarenessFromDpiAwarenessContext"));
    result << " threadDpiAwareness=" << (context && awareness ? awareness(context()) : -1);
    using GetProcessContext = HANDLE(WINAPI*)(HANDLE);
    const auto processContext = reinterpret_cast<GetProcessContext>(GetProcAddress(user, "GetDpiAwarenessContextForProcess"));
    result << " processDpiAwareness=" << (processContext && awareness ? awareness(processContext(GetCurrentProcess())) : -1);
    MONITORINFOEXW monitor{}; monitor.cbSize = sizeof(monitor);
    if (impl_->parent && GetMonitorInfoW(MonitorFromWindow(impl_->parent, MONITOR_DEFAULTTONEAREST), &monitor)) {
        result << " windowMonitor=" << Utf8(monitor.szDevice);
        DEVMODEW mode{}; mode.dmSize = sizeof(mode);
        if (EnumDisplaySettingsW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &mode))
            result << " monitorMode=" << mode.dmPelsWidth << 'x' << mode.dmPelsHeight << '@' << mode.dmDisplayFrequency << "Hz";
        else result << " monitorModeQueryError=" << GetLastError();
    } else result << " windowMonitorQueryError=" << GetLastError();
    return result.str();
}

Graphics::D3D11::D3D11Renderer& D3D11Viewport::Renderer() { return impl_->renderer; }
} // namespace Astral::Platform
