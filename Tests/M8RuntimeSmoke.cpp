#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

namespace {
struct WindowSearch {
    DWORD processId{};
    HWND window{};
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
    wchar_t title[1400]{};
    GetWindowTextW(window, title, 1400);
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

bool SendKey(WORD key) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    if (SendInput(1, &input, sizeof(INPUT)) != 1) return false;
    Sleep(100);
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    if (SendInput(1, &input, sizeof(INPUT)) != 1) return false;
    Sleep(100);
    return true;
}

bool CaptureFocusRegion(HWND window, std::vector<COLORREF>& colors) {
    HDC source = GetDC(window);
    colors.clear();
    for (int y = 65; y < 82; ++y) {
        for (int x = 20; x < 300; ++x) colors.push_back(GetPixel(source, x, y));
    }
    ReleaseDC(window, source);
    return colors.size() == 4760;
}

bool HasVisibleFocusChange(HWND window, const std::vector<COLORREF>& baseline,
    int& changedPixels) {
    for (int attempt = 0; attempt < 30; ++attempt) {
        std::vector<COLORREF> focused;
        if (!CaptureFocusRegion(window, focused) || focused.size() != baseline.size()) return false;
        changedPixels = 0;
        for (std::size_t index = 0; index < focused.size(); ++index) {
            if (focused[index] != baseline[index]) ++changedPixels;
        }
        if (changedPixels > 1000) return true;
        Sleep(20);
    }
    return false;
}
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::cerr << "usage: M8RuntimeSmoke <AstralGame> <working-directory>\n";
        return 1;
    }

    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\"";
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, argv[2],
            &startup, &process)) {
        std::cerr << "M8 AUTOMATED NATIVE RUNTIME SMOKE: FAIL (launch)\n";
        return 1;
    }

    bool passed = false;
    HWND window = WaitForWindow(process.dwProcessId);
    std::wstring initialTitle;
    std::wstring successTitle;
    std::wstring rejectionTitle;
    std::wstring focusTitle;
    int focusPixels = 0;
    std::vector<COLORREF> focusBaseline;
    if (window && WaitForTitle(window, L"M8 Thought Commands", initialTitle)) {
        SetForegroundWindow(window);
        Sleep(250);
        const bool commandSucceeded = SendKey('1')
            && WaitForTitle(window, L"Command: dash | ACCEPTED", successTitle)
            && successTitle.find(L"Pos: (0.000000, 6.000000)") != std::wstring::npos;
        const bool commandRejected = commandSucceeded && SendKey('0')
            && WaitForTitle(window, L"Command: unsupported | REJECTED: Unsupported",
                rejectionTitle)
            && rejectionTitle.find(L"Pos: (0.000000, 6.000000)") != std::wstring::npos;
        const bool baselineCaptured = commandRejected
            && CaptureFocusRegion(window, focusBaseline);
        const bool focusVisible = baselineCaptured && SendKey('5')
            && WaitForTitle(window, L"Command: focus | ACCEPTED | Focus: ON x0.35", focusTitle)
            && HasVisibleFocusChange(window, focusBaseline, focusPixels);
        if (focusVisible && SendKey(VK_ESCAPE)) {
            passed = WaitForSingleObject(process.hProcess, 5000) == WAIT_OBJECT_0;
        }
    }

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
        std::wcerr << L"M8 AUTOMATED NATIVE RUNTIME SMOKE: FAIL\nInitial: " << initialTitle
            << L"\nSuccess: " << successTitle << L"\nRejection: " << rejectionTitle
            << L"\nFocus: " << focusTitle << L"\nFocus pixels: " << focusPixels
            << L"\nExit: " << exitCode << L"\n";
        return 1;
    }

    std::wcout << L"M8 AUTOMATED NATIVE RUNTIME SMOKE: PASS\nSuccess: " << successTitle
        << L"\nRejection: " << rejectionTitle << L"\nFocus: " << focusTitle
        << L"\nVisible focus pixels: " << focusPixels
        << L"\nClean Escape exit code: " << exitCode << L"\n";
    return 0;
}
