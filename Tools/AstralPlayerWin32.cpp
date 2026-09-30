// Win32 engine player: a window around Framework::GameHost (the role of a
// packaged Unity player / UE standalone game). It turns window messages into
// HostInput, runs the host with real frame times, streams the mixer through
// waveOut, and blits the rendered frame with StretchDIBits.
//
//   AstralPlayerWin32.exe [content-root] [project-or-scene.json]
//   (defaults: Content/Samples/Playground and project.json)
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
#include "Game/Samples/Playground/PlaygroundBehaviours.h"
#include "Game/Showcase/Win32AstralPresenter.h" // Win32WaveOutput

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <fstream>
#include <memory>
#include <string>
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
};

void Present(PlayerWindow& player, HDC deviceContext) {
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
    switch (message) {
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
            if (key == VK_F6) player->renderScale = player->renderScale == 1 ? 2 : 1;
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
        input.ui.pointer = {static_cast<float>(point.x) / static_cast<float>(player->renderScale),
            static_cast<float>(point.y) / static_cast<float>(player->renderScale)};
        return 0;
    }
    case WM_LBUTTONDOWN:
        input.keys.keys.set(Input::Keys::MouseLeft);
        input.ui.pointerDown = true;
        SetCapture(window);
        return 0;
    case WM_LBUTTONUP:
        input.keys.keys.reset(Input::Keys::MouseLeft);
        input.ui.pointerDown = false;
        ReleaseCapture();
        return 0;
    case WM_RBUTTONDOWN:
        input.keys.keys.set(Input::Keys::MouseRight);
        return 0;
    case WM_RBUTTONUP:
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
    std::string contentRoot = "Content/Samples/Playground";
    std::string startPath = "project.json";
    if (__argc > 1) contentRoot = Utf8(__wargv[1]);
    if (__argc > 2) startPath = Utf8(__wargv[2]);

    Samples::RegisterPlaygroundBehaviours();
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
    RECT frame{0, 0, player.clientWidth, player.clientHeight};
    AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);
    const std::wstring title = L"Astral Player - " + Wide(project && !host->Project().name.empty() ? host->Project().name : startPath);
    HWND window = CreateWindowExW(0, windowClass.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
        CW_USEDEFAULT, frame.right - frame.left, frame.bottom - frame.top, nullptr, nullptr, instance, nullptr);
    if (!window) return Fail("CreateWindowExW failed");
    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&player));
    ShowWindow(window, showCommand);

    std::unique_ptr<Showcase::Win32WaveOutput> audio;
    if (host->Audio()) audio = std::make_unique<Showcase::Win32WaveOutput>(*host->Audio());

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

        ++framesSinceTitle;
        const float sinceTitle = std::chrono::duration<float>(now - titleTime).count();
        if (sinceTitle >= 0.5f) {
            wchar_t text[256];
            swprintf(text, 256, L"%ls  |  %.0f fps  |  %zu entities%ls", title.c_str(),
                static_cast<double>(static_cast<float>(framesSinceTitle) / sinceTitle), host->World().Stats().entities,
                player.recording ? L"  |  REC" : L"");
            SetWindowTextW(window, text);
            titleTime = now;
            framesSinceTitle = 0;
        }
    }
    if (player.recording) ToggleRecording(player); // keep what was recorded
    audio.reset();
    DestroyWindow(window);
    host.reset();
    return 0;
}
