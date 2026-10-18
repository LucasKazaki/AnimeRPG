// Win32 engine player: a window around Framework::GameHost (the role of a
// packaged Unity player / UE standalone game). It turns window messages into
// HostInput, runs the host with real frame times, streams the mixer through
// waveOut, and blits the rendered frame with StretchDIBits.
//
//   AstralPlayerWin32.exe [content-root] [project-or-scene.json]
//   (legacy defaults: Content/Samples/Playground and project.json)
//   AstralEnginePlayerWin32.exe [content-root] [project-or-scene.json]
//   (engine defaults: Content/project.json beside the actual executable)
//
// Keys: the project's input map (Playground: WASD/arrows move, Space jumps,
// Shift sprints, Q/E orbit), ` opens the console, F3 toggles stats, F5
// restarts the scene, F6 halves/restores the render resolution, F9 starts and
// stops input recording (written to recording.input for
// `AstralPlayer --replay`), Esc closes the console or quits.
//
// Scene files hot-reload: save an edit to the running scene's JSON (or an
// asset it uses) and the scene reloads within half a second; a broken edit is
// reported in the console and the running scene is kept.

#include "Engine/Framework/GameHost.h"
#include "Engine/Input/InputSystem.h"
#include "Engine/Platform/Win32/Win32WaveOutput.h"
#if defined(ASTRAL_NATIVE_D3D11)
#include "Engine/Platform/Win32/D3D11Viewport.h"
#endif
#if defined(ASTRAL_PLAYER_PLAYGROUND)
#include "Game/Samples/Playground/PlaygroundBehaviours.h"
#endif

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace {

using namespace Astral;

std::string Utf8(const wchar_t* text) {
    if (!text || !*text) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (bytes <= 1) return {};
    std::string out(static_cast<std::size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, out.data(), bytes, nullptr, nullptr);
    out.resize(static_cast<std::size_t>(bytes - 1));
    return out;
}

std::wstring Wide(const std::string& text) {
    if (text.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (count <= 1) return {};
    std::wstring out(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), count);
    out.resize(static_cast<std::size_t>(count - 1));
    return out;
}

void AppendUtf8(std::string& out, wchar_t c) {
    const wchar_t pair[2] = {c, L'\0'};
    out += Utf8(pair);
}

#if !defined(ASTRAL_PLAYER_PLAYGROUND)
bool PackagedContentRoot(std::string& root, std::string& error) {
    std::vector<wchar_t> module(32768);
    const DWORD length = GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()));
    if (length == 0 || length >= module.size()) {
        error = "Cannot resolve the player executable path (Windows error " + std::to_string(GetLastError()) + ")";
        return false;
    }
    try {
        root = (std::filesystem::path(std::wstring(module.data(), length)).parent_path() / L"Content").u8string();
        return true;
    } catch (const std::system_error& exception) {
        error = "Cannot resolve packaged Content: " + exception.code().message();
        return false;
    }
}
#endif

// Everything the window procedure feeds and the frame loop consumes.
struct PlayerWindow {
    Framework::GameHost* host{};
    Framework::HostInput input;  // held keys persist; edges and text reset each frame
    int clientWidth{1280};
    int clientHeight{720};
    int renderScale{1};          // 1 = native, 2 = half resolution
    POINT lastMouse{};
    bool haveMouse{};
    bool recording{};
    Input::InputRecording recorded;
    bool quit{};
    std::vector<std::uint32_t> bgra;
#if defined(ASTRAL_NATIVE_D3D11)
    HWND window{};
    std::unique_ptr<Platform::D3D11Viewport> gpu;
    std::string rendererReason;
    bool gpuSkipped{};
#endif
};

#if defined(ASTRAL_NATIVE_D3D11)
constexpr UINT kRendererCpu = 5001, kRendererGpu = 5002;
int EffectiveScale(const PlayerWindow& player);
bool GpuActive(const PlayerWindow& player) { return player.gpu && player.gpu->Active(); }
void SetPointer(PlayerWindow& player, POINT point) {
    const float scale = static_cast<float>(EffectiveScale(player));
    player.input.ui.pointer = {static_cast<float>(point.x) / scale, static_cast<float>(point.y) / scale};
}
void RefreshPointer(PlayerWindow& player) {
    POINT point{};
    if (GetCursorPos(&point) && ScreenToClient(player.window, &point)) {
        SetPointer(player, point); player.lastMouse = point; player.haveMouse = true;
    }
    player.input.keys.mouseDelta = {};
}
std::string RendererText(const PlayerWindow& player) {
    if (GpuActive(player)) return "D3D11 Opaque Unlit | neutral display | CPU UI";
    return player.rendererReason.empty() ? "CPU" : "CPU | " + player.rendererReason;
}
void SelectCpu(PlayerWindow& player, const std::string& reason = {}) {
    player.rendererReason = reason;
    if (player.gpu) {
        std::string error;
        if (!player.gpu->Deactivate(error)) player.rendererReason += " | detach: " + error;
        player.gpu.reset();
    }
    player.gpuSkipped = false;
    Graphics::RenderScene cpuScene; Graphics::RenderView cpuView;
    player.host->PrepareRenderScene(std::max(1, player.clientWidth / player.renderScale),
        std::max(1, player.clientHeight / player.renderScale), cpuScene, cpuView);
    player.host->SetExternalRenderStats({}, 0, "CPU");
    RefreshPointer(player);
    CheckMenuRadioItem(GetMenu(player.window), kRendererCpu, kRendererGpu, kRendererCpu, MF_BYCOMMAND);
    player.host->Console().Print("Renderer: " + RendererText(player));
}
void SelectGpu(PlayerWindow& player) {
    if (GpuActive(player)) return;
    auto candidate = std::make_unique<Platform::D3D11Viewport>();
    std::string error;
    if (!candidate->Activate(player.window, *player.host, error)) { SelectCpu(player, error); return; }
    Platform::NativeViewportStats stats;
    if (candidate->Render(*player.host, stats, error) == Graphics::D3D11::RenderStatus::Failed) {
        candidate.reset(); SelectCpu(player, error); return;
    }
    player.gpu = std::move(candidate); player.rendererReason.clear();
    RefreshPointer(player);
    CheckMenuRadioItem(GetMenu(player.window), kRendererCpu, kRendererGpu, kRendererGpu, MF_BYCOMMAND);
    player.host->Console().Print("Renderer: D3D11 Opaque Unlit; neutral display and CPU UI; lighting/sky/fog/shadows/general post unsupported");
    player.host->Console().Print(player.gpu->Diagnostics());
}
#endif

int EffectiveScale(const PlayerWindow& player) {
#if defined(ASTRAL_NATIVE_D3D11)
    if (GpuActive(player)) return 1;
#endif
    return player.renderScale;
}

void Present(PlayerWindow& player, HDC deviceContext) {
#if defined(ASTRAL_NATIVE_D3D11)
    if (GpuActive(player)) {
        std::string error; Platform::NativeViewportStats stats;
        const auto result = player.gpu->Render(*player.host, stats, error);
        player.gpuSkipped = result == Graphics::D3D11::RenderStatus::Skipped;
        if (result != Graphics::D3D11::RenderStatus::Failed) return;
        SelectCpu(player, error); // one explicit return; no automatic retry
    }
#endif
    const int width = std::max(1, player.clientWidth / player.renderScale);
    const int height = std::max(1, player.clientHeight / player.renderScale);
    const Graphics::ImageRgba8& image = player.host->Render(width, height);
    const std::size_t count = static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height);
    player.bgra.resize(count);
    const std::uint8_t* source = image.pixels.data();
    for (std::size_t index = 0; index < count; ++index, source += 4) {
        player.bgra[index] = 0xFF000000u | (static_cast<std::uint32_t>(source[0]) << 16)
            | (static_cast<std::uint32_t>(source[1]) << 8) | static_cast<std::uint32_t>(source[2]);
    }
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = image.width;
    info.bmiHeader.biHeight = -image.height; // top-down rows
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(deviceContext, 0, 0, player.clientWidth, player.clientHeight, 0, 0, image.width, image.height,
        player.bgra.data(), &info, DIB_RGB_COLORS, SRCCOPY);
}

void ToggleRecording(PlayerWindow& player) {
    if (!player.recording) {
        player.recorded = {};
        player.recording = true;
        player.host->Console().Print("recording input (F9 stops)");
        return;
    }
    player.recording = false;
    std::ofstream file("recording.input", std::ios::binary | std::ios::trunc);
    file << player.recorded.Serialize();
    player.host->Console().Print(file ? "wrote recording.input (" + std::to_string(player.recorded.FrameCount()) + " frames)"
                                      : std::string("cannot write recording.input"));
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* player = reinterpret_cast<PlayerWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (!player) return DefWindowProcW(window, message, wParam, lParam);
    Framework::HostInput& input = player->input;
#if defined(ASTRAL_NATIVE_D3D11)
    // Native menus own system keys and both halves of the F10 menu gesture.
    if (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP
        || ((message == WM_KEYDOWN || message == WM_KEYUP) && wParam == VK_F10)) {
        // An ordinary-held key can be released while Alt is down.
        if (message == WM_SYSKEYUP || message == WM_KEYUP)
            input.keys.keys.reset(static_cast<std::size_t>(wParam & 0xFF));
        return DefWindowProcW(window, message, wParam, lParam);
    }
#endif
    switch (message) {
#if defined(ASTRAL_NATIVE_D3D11)
    case WM_COMMAND:
        if (LOWORD(wParam) == kRendererCpu) { SelectCpu(*player); return 0; }
        if (LOWORD(wParam) == kRendererGpu) { SelectGpu(*player); return 0; }
        return DefWindowProcW(window, message, wParam, lParam);
#endif
    case WM_CLOSE:
        player->quit = true;
        return 0;
    case WM_SIZE:
        player->clientWidth = std::max(1, static_cast<int>(LOWORD(lParam)));
        player->clientHeight = std::max(1, static_cast<int>(HIWORD(lParam)));
        return 0;
    case WM_KILLFOCUS:
        input.keys.keys.reset(); // no stuck keys after alt-tab
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        const auto key = static_cast<std::uint16_t>(wParam & 0xFF);
        const bool repeat = (lParam & (1 << 30)) != 0;
        input.keys.keys.set(key);
        if (!repeat) {
            input.ui.keysPressed.push_back(key);
            if (key == VK_ESCAPE && !player->host->Console().IsOpen()) player->quit = true;
            if (key == VK_F3) player->host->Commands().Execute("stat");
            if (key == VK_F5) player->host->Commands().Execute("restart");
            if (key == VK_F6) {
#if defined(ASTRAL_NATIVE_D3D11)
                if (GpuActive(*player)) player->host->Console().Print("D3D11 uses the full viewport; select CPU to use Half Resolution");
                else
#endif
                    player->renderScale = player->renderScale == 1 ? 2 : 1;
            }
            if (key == VK_F9) ToggleRecording(*player);
        }
        input.ui.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        return message == WM_SYSKEYDOWN && key != VK_F10 ? DefWindowProcW(window, message, wParam, lParam) : 0;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
        input.keys.keys.reset(static_cast<std::size_t>(wParam & 0xFF));
        return message == WM_SYSKEYUP ? DefWindowProcW(window, message, wParam, lParam) : 0;
    case WM_CHAR:
        if (wParam >= 32 && wParam != 127) AppendUtf8(input.ui.text, static_cast<wchar_t>(wParam));
        return 0;
    case WM_MOUSEMOVE: {
        const POINT point{static_cast<SHORT>(LOWORD(lParam)), static_cast<SHORT>(HIWORD(lParam))};
        if (player->haveMouse) {
            input.keys.mouseDelta.x += static_cast<float>(point.x - player->lastMouse.x);
            input.keys.mouseDelta.y += static_cast<float>(point.y - player->lastMouse.y);
        }
        player->lastMouse = point;
        player->haveMouse = true;
        // UI coordinates are render pixels.
        input.ui.pointer = {static_cast<float>(point.x) / static_cast<float>(EffectiveScale(*player)),
            static_cast<float>(point.y) / static_cast<float>(EffectiveScale(*player))};
        return 0;
    }
    case WM_LBUTTONDOWN:
#if defined(ASTRAL_NATIVE_D3D11)
        SetPointer(*player, {static_cast<SHORT>(LOWORD(lParam)), static_cast<SHORT>(HIWORD(lParam))});
#endif
        input.keys.keys.set(Input::Keys::MouseLeft);
        input.ui.pointerDown = true;
        SetCapture(window);
        return 0;
    case WM_LBUTTONUP:
#if defined(ASTRAL_NATIVE_D3D11)
        SetPointer(*player, {static_cast<SHORT>(LOWORD(lParam)), static_cast<SHORT>(HIWORD(lParam))});
#endif
        input.keys.keys.reset(Input::Keys::MouseLeft);
        input.ui.pointerDown = false;
        ReleaseCapture();
        return 0;
    case WM_RBUTTONDOWN:
#if defined(ASTRAL_NATIVE_D3D11)
        SetPointer(*player, {static_cast<SHORT>(LOWORD(lParam)), static_cast<SHORT>(HIWORD(lParam))});
#endif
        input.keys.keys.set(Input::Keys::MouseRight);
        return 0;
    case WM_RBUTTONUP:
#if defined(ASTRAL_NATIVE_D3D11)
        SetPointer(*player, {static_cast<SHORT>(LOWORD(lParam)), static_cast<SHORT>(HIWORD(lParam))});
#endif
        input.keys.keys.reset(Input::Keys::MouseRight);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        Present(*player, deviceContext);
        EndPaint(window, &paint);
        return 0;
    }
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

int Fail(const std::string& text) {
    MessageBoxW(nullptr, Wide(text).c_str(), L"Astral Player", MB_OK | MB_ICONERROR);
    return 2;
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    std::string contentRoot;
    std::string startPath = "project.json";
#if defined(ASTRAL_PLAYER_PLAYGROUND)
    contentRoot = "Content/Samples/Playground";
#else
    if (__argc > 3) return Fail("Usage: AstralEnginePlayerWin32.exe [content-root] [project-or-scene.json]");
    if (__argc <= 1) {
        std::string error;
        if (!PackagedContentRoot(contentRoot, error)) return Fail(error);
    }
#endif
    if (__argc > 1) contentRoot = Utf8(__wargv[1]);
    if (__argc > 2) startPath = Utf8(__wargv[2]);

#if defined(ASTRAL_PLAYER_PLAYGROUND)
    Samples::RegisterPlaygroundBehaviours();
#endif
    Framework::HostSettings settings;
    settings.contentRoot = contentRoot;
    settings.hotReload = true; // edit-and-see iteration while the player runs
    auto host = std::make_unique<Framework::GameHost>(settings);
    std::string error;
    const auto probe = host->Assets().Load<Core::JsonValue>(startPath);
    const bool project = probe.Ready() && probe->String("format") == "astral-project";
    if (!(project ? host->LoadProject(startPath, error) : host->LoadScene(startPath, error))) {
        return Fail("Cannot start " + contentRoot + "/" + startPath + ":\n" + error);
    }

    PlayerWindow player;
    player.host = host.get();
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = L"AstralPlayerWindow";
    if (!RegisterClassW(&windowClass)) return Fail("RegisterClassW failed");
    HMENU menu = nullptr;
    DWORD windowStyle = WS_OVERLAPPEDWINDOW;
#if defined(ASTRAL_NATIVE_D3D11)
    menu = CreateMenu();
    HMENU rendererMenu = CreatePopupMenu();
    if (!menu || !rendererMenu) { if (menu) DestroyMenu(menu); if (rendererMenu) DestroyMenu(rendererMenu); return Fail("Cannot create renderer menu"); }
    if (!AppendMenuW(rendererMenu, MF_STRING | MF_CHECKED, kRendererCpu, L"&CPU")
        || !AppendMenuW(rendererMenu, MF_STRING, kRendererGpu, L"&D3D11 Opaque Unlit")
        || !AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(rendererMenu), L"&Renderer")) {
        DestroyMenu(rendererMenu); DestroyMenu(menu); return Fail("Cannot populate renderer menu");
    }
    windowStyle |= WS_CLIPCHILDREN;
#endif
    RECT frame{0, 0, player.clientWidth, player.clientHeight};
    AdjustWindowRectEx(&frame, windowStyle, menu != nullptr, 0);
    const std::wstring title = L"Astral Player - " + Wide(project && !host->Project().name.empty() ? host->Project().name : startPath);
    HWND window = CreateWindowExW(0, windowClass.lpszClassName, title.c_str(), windowStyle, CW_USEDEFAULT,
        CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top, nullptr, menu, instance, nullptr);
    if (!window) { if (menu) DestroyMenu(menu); return Fail("CreateWindowExW failed"); }
#if defined(ASTRAL_NATIVE_D3D11)
    player.window = window;
    host->Commands().RegisterCommand("renderer.info", "renderer.info: actual renderer, HWND/DPI/output metadata", [&player](const std::vector<std::string>& arguments) {
        if (!arguments.empty()) return std::string("usage: renderer.info");
        return GpuActive(player) ? player.gpu->Diagnostics() : "Renderer: " + RendererText(player);
    });
#endif
    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&player));
    ShowWindow(window, showCommand);

    std::unique_ptr<Platform::Win32WaveOutput> audio;
    if (host->Audio()) {
        audio = std::make_unique<Platform::Win32WaveOutput>(*host->Audio());
        host->Console().Print(audio->Active() ? "Audio output device active" : "Audio output unavailable");
    }

    using Clock = std::chrono::steady_clock;
    Clock::time_point last = Clock::now();
    Clock::time_point titleTime = last;
    int framesSinceTitle = 0;
    while (!player.quit && !host->QuitRequested()) {
        MSG message;
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) player.quit = true;
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        if (player.quit) break;
        const Clock::time_point now = Clock::now();
        const float dt = std::min(0.1f, std::chrono::duration<float>(now - last).count());
        last = now;
        if (player.recording) {
            Input::InputSnapshot snapshot = player.input.keys;
            snapshot.dt = dt;
            player.recorded.Record(snapshot);
        }
        host->Update(dt, player.input);
        // Edges, text and mouse motion are per frame; held keys persist.
        player.input.ui.keysPressed.clear();
        player.input.ui.text.clear();
        player.input.keys.mouseDelta = {};
        HDC deviceContext = GetDC(window);
        Present(player, deviceContext);
        ReleaseDC(window, deviceContext);
#if defined(ASTRAL_NATIVE_D3D11)
        if (GpuActive(player) && player.gpuSkipped) MsgWaitForMultipleObjects(0, nullptr, FALSE, 16, QS_ALLINPUT);
#endif

        ++framesSinceTitle;
        const float sinceTitle = std::chrono::duration<float>(now - titleTime).count();
        if (sinceTitle >= 0.5f) {
            wchar_t text[256];
            swprintf(text, 256, L"%ls  |  %.0f fps  |  %zu entities%ls", title.c_str(),
                static_cast<double>(static_cast<float>(framesSinceTitle) / sinceTitle), host->World().Stats().entities,
                player.recording ? L"  |  REC" : L"");
            SetWindowTextW(window, text);
#if defined(ASTRAL_NATIVE_D3D11)
            const std::wstring actualTitle = std::wstring(text) + L" | Renderer: " + Wide(RendererText(player));
            SetWindowTextW(window, actualTitle.c_str());
#endif
            titleTime = now;
            framesSinceTitle = 0;
        }
    }
    if (player.recording) ToggleRecording(player); // keep what was recorded
#if defined(ASTRAL_NATIVE_D3D11)
    if (player.gpu) SelectCpu(player);
#endif
    audio.reset();
    DestroyWindow(window);
    host.reset();
    return 0;
}
