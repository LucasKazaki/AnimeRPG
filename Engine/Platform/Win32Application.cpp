#include "Engine/Platform/Win32Application.h"

#include "Engine/Core/Clock.h"
#include "Engine/Core/Logger.h"
#include "Engine/Renderer/Renderer.h"

#include <string>

namespace {
constexpr wchar_t kWindowClass[] = L"AstralEngineWindow";
Astral::Renderer::Renderer g_renderer;
Astral::Core::Logger g_logger("astral.log");

void UpdateTitle(HWND window, float fps) {
    const std::wstring title = L"Astral Engine | Milestone 1 | FPS: " + std::to_wstring(static_cast<int>(fps));
    SetWindowTextW(window, title.c_str());
}
} // namespace

namespace Astral::Platform {

bool Win32Application::Create(HINSTANCE instance, int showCommand) {
    WNDCLASSW windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.lpszClassName = kWindowClass;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    if (!RegisterClassW(&windowClass)) {
        g_logger.Info("RegisterClassW failed");
        return false;
    }

    window_ = CreateWindowExW(0, kWindowClass, L"Astral Engine", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, nullptr, nullptr, instance, nullptr);
    if (!window_) {
        g_logger.Info("CreateWindowExW failed");
        return false;
    }

    ShowWindow(window_, showCommand);
    UpdateWindow(window_);
    if (!debugMesh_.LoadFromFile("Game/Assets/debug_triangle.mesh")) {
        g_logger.Info("Failed to load Game/Assets/debug_triangle.mesh");
        return false;
    }
    g_logger.Info("Window created; debug scene mesh loaded; press Escape or close the window to exit");
    return true;
}

int Win32Application::Run() {
    Astral::Core::Clock clock;
    MSG message{};
    double fpsAccumulator = 0.0;
    int frameCount = 0;

    while (message.message != WM_QUIT) {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        const float deltaSeconds = clock.Tick();
        fpsAccumulator += deltaSeconds;
        ++frameCount;
        if (fpsAccumulator >= 1.0) {
            UpdateTitle(window_, static_cast<float>(frameCount / fpsAccumulator));
            g_logger.Info("Frame timing active");
            fpsAccumulator = 0.0;
            frameCount = 0;
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            PostMessageW(window_, WM_CLOSE, 0, 0);
        }

        HDC deviceContext = GetDC(window_);
        RECT viewport{};
        GetClientRect(window_, &viewport);
        g_renderer.Clear(deviceContext, viewport);
        g_renderer.RenderDebugScene(deviceContext, viewport, camera_, debugMesh_, debugTransform_);
        ReleaseDC(window_, deviceContext);
        Sleep(1);
    }

    return static_cast<int>(message.wParam);
}

LRESULT CALLBACK Win32Application::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        BeginPaint(window, &paint);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

} // namespace Astral::Platform
