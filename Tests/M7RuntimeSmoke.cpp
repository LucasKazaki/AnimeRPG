#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

namespace {
struct WindowSearch {
    DWORD processId{};
    HWND window{};
};

struct ShadowPixels {
    int resource{};
    int guard{};
};

BOOL CALLBACK FindProcessWindow(HWND window, LPARAM parameter) {
    auto& search = *reinterpret_cast<WindowSearch*>(parameter);
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId == search.processId && IsWindowVisible(window)) {
        search.window = window;
        return FALSE;
    }
    return TRUE;
}

HWND WaitForWindow(DWORD processId) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        WindowSearch search{processId, nullptr};
        EnumWindows(FindProcessWindow, reinterpret_cast<LPARAM>(&search));
        if (search.window) return search.window;
        Sleep(50);
    }
    return nullptr;
}

std::wstring WindowTitle(HWND window) {
    wchar_t title[1024]{};
    GetWindowTextW(window, title, 1024);
    return title;
}

bool WaitForTitle(HWND window, const std::wstring& marker, std::wstring& observed) {
    for (int attempt = 0; attempt < 100; ++attempt) {
        observed = WindowTitle(window);
        if (observed.find(marker) != std::wstring::npos) return true;
        Sleep(50);
    }
    return false;
}

bool SetKey(WORD key, bool down) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    if (!down) input.ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(1, &input, sizeof(INPUT)) == 1;
}

bool SendKey(WORD key) {
    if (!SetKey(key, true)) return false;
    Sleep(100);
    if (!SetKey(key, false)) return false;
    // Let the live edge-triggered input loop observe the release before a
    // second press of the same key.
    Sleep(100);
    return true;
}

bool CaptureShadowPixels(HWND window, ShadowPixels& evidence) {
    RECT client{};
    if (!GetClientRect(window, &client)) return false;
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    if (width <= 0 || height <= 0) return false;

    HDC source = GetDC(window);
    HDC memory = CreateCompatibleDC(source);
    HBITMAP bitmap = CreateCompatibleBitmap(source, width, height);
    const HGDIOBJ previous = SelectObject(memory, bitmap);
    const BOOL copied = BitBlt(memory, 0, 0, width, height, source, 0, 0, SRCCOPY);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);
    const int rows = copied ? GetDIBits(memory, bitmap, 0, static_cast<UINT>(height),
        pixels.data(), &info, DIB_RGB_COLORS) : 0;
    if (rows == height) {
        for (std::size_t index = 0; index < pixels.size(); index += 4) {
            const COLORREF color = RGB(pixels[index + 2], pixels[index + 1], pixels[index]);
            if (color == RGB(70, 220, 235)) ++evidence.resource;
            if (color == RGB(255, 220, 70)) ++evidence.guard;
        }
    }
    SelectObject(memory, previous);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(window, source);
    return rows == height;
}

bool CaptureVisibleShadowState(HWND window, bool requireGuard, ShadowPixels& best) {
    for (int attempt = 0; attempt < 30; ++attempt) {
        ShadowPixels current{};
        if (!CaptureShadowPixels(window, current)) return false;
        if (current.resource > best.resource) best.resource = current.resource;
        if (current.guard > best.guard) best.guard = current.guard;
        if (current.resource > 100 && (!requireGuard || current.guard > 20)) return true;
        Sleep(20);
    }
    return best.resource > 100 && (!requireGuard || best.guard > 20);
}
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::cerr << "usage: M7RuntimeSmoke <AstralGame> <working-directory>\n";
        return 1;
    }

    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\"";
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, argv[2],
            &startup, &process)) {
        std::cerr << "M7 AUTOMATED NATIVE RUNTIME SMOKE: FAIL (launch)\n";
        return 1;
    }

    bool passed = false;
    HWND window = WaitForWindow(process.dwProcessId);
    std::wstring initialTitle;
    std::wstring guardTitle;
    std::wstring blockedTitle;
    std::wstring releasedTitle;
    std::wstring fatalTitle;
    std::wstring dashTitle;
    std::wstring cooldownTitle;
    ShadowPixels initialPixels{};
    ShadowPixels guardPixels{};
    if (window && WaitForTitle(window, L"M7 Shadowblade", initialTitle)
        && initialTitle.find(L"Shadow: 100/100 | Guard: OFF") != std::wstring::npos
        && initialTitle.find(L"Pos: (0.000000, 0.000000)") != std::wstring::npos) {
        SetForegroundWindow(window);
        Sleep(250);
        const bool initialVisible = CaptureVisibleShadowState(window, false, initialPixels);
        const bool guardDown = initialVisible && SetKey(VK_LSHIFT, true)
            && WaitForTitle(window, L"Guard: ON", guardTitle);
        Sleep(150);
        const bool guardVisible = guardDown
            && CaptureVisibleShadowState(window, true, guardPixels);
        const bool guardBlockedDash = guardVisible && SendKey('Q')
            && WaitForTitle(window, L"Shadow Last: Dash Blocked by Guard", blockedTitle)
            && blockedTitle.find(L"Pos: (0.000000, 0.000000)") != std::wstring::npos
            && blockedTitle.find(L"Shadow: 100/100") != std::wstring::npos;
        const bool guardUp = SetKey(VK_LSHIFT, false)
            && WaitForTitle(window, L"Guard: OFF", releasedTitle);
        const bool fatalHit = guardBlockedDash && guardUp && SendKey('L')
            && WaitForTitle(window, L"Shadow Last: Fatal Activated -80", fatalTitle)
            && fatalTitle.find(L"Dummy: Alive HP: 20/100") != std::wstring::npos;
        const bool dashed = fatalHit && SendKey('Q')
            && WaitForTitle(window, L"Shadow Last: Dash Activated", dashTitle)
            && dashTitle.find(L"Pos: (0.000000, 6.000000)") != std::wstring::npos;
        const bool cooldownRejected = dashed && SendKey('Q')
            && WaitForTitle(window, L"Shadow Last: Dash Cooldown", cooldownTitle)
            && cooldownTitle.find(L"Pos: (0.000000, 6.000000)") != std::wstring::npos;
        if (cooldownRejected && SendKey(VK_ESCAPE)) {
            passed = WaitForSingleObject(process.hProcess, 5000) == WAIT_OBJECT_0;
        }
    }

    SetKey(VK_LSHIFT, false);
    if (!passed) {
        if (window) PostMessageW(window, WM_CLOSE, 0, 0);
        if (WaitForSingleObject(process.hProcess, 2000) != WAIT_OBJECT_0) {
            TerminateProcess(process.hProcess, 2);
            WaitForSingleObject(process.hProcess, 2000);
        }
    }

    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    if (!passed || exitCode != 0) {
        std::wcerr << L"M7 AUTOMATED NATIVE RUNTIME SMOKE: FAIL\nInitial: " << initialTitle
            << L"\nGuard: " << guardTitle << L"\nBlocked: " << blockedTitle
            << L"\nReleased: " << releasedTitle << L"\nFatal: " << fatalTitle
            << L"\nDash: " << dashTitle << L"\nCooldown: " << cooldownTitle
            << L"\nPixels initial resource/guard: " << initialPixels.resource << L"/"
            << initialPixels.guard << L" guard resource/guard: " << guardPixels.resource
            << L"/" << guardPixels.guard << L"\nExit: " << exitCode << L"\n";
        return 1;
    }

    std::wcout << L"M7 AUTOMATED NATIVE RUNTIME SMOKE: PASS\nInitial: " << initialTitle
        << L"\nGuard: " << guardTitle << L"\nBlocked: " << blockedTitle
        << L"\nFatal: " << fatalTitle << L"\nDash: " << dashTitle
        << L"\nCooldown: " << cooldownTitle
        << L"\nPixels initial resource/guard: " << initialPixels.resource << L"/"
        << initialPixels.guard << L" guard resource/guard: " << guardPixels.resource
        << L"/" << guardPixels.guard << L"\nClean Escape exit code: " << exitCode << L"\n";
    return 0;
}
