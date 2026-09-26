#include <windows.h>

#include "Tests/WindowInput.h"

#include <iostream>
#include <string>


namespace {
Astral::Tests::WindowInputTarget g_inputTarget;
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
    wchar_t title[2048]{};
    GetWindowTextW(window, title, 2048);
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
    return g_inputTarget.SendKey(key);
}

bool HoldKey(WORD key, DWORD milliseconds) {
    return g_inputTarget.HoldKey(key, milliseconds, 100);
}

int CountColor(HWND window, COLORREF expected) {
    HDC source = GetDC(window);
    int count = 0;
    for (int y = 108; y < 126; ++y) {
        for (int x = 20; x < 300; ++x) {
            if (GetPixel(source, x, y) == expected) ++count;
        }
    }
    ReleaseDC(window, source);
    return count;
}
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::cerr << "usage: M9RuntimeSmoke <AstralGame> <working-directory>\n";
        return 1;
    }

    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\"";
    STARTUPINFOW startup{sizeof(startup)};
    Astral::Tests::PrepareNoActivate(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, argv[2],
            &startup, &process)) {
        std::cerr << "M9 AUTOMATED NATIVE RUNTIME SMOKE: FAIL (launch)\n";
        return 1;
    }

    bool passed = false;
    HWND window = WaitForWindow(process.dwProcessId);
    std::wstring initialTitle;
    std::wstring selectedTitle;
    std::wstring discoveredTitle;
    std::wstring repeatedTitle;
    int selectedPixels = 0;
    int visitedPixels = 0;
    if (window && g_inputTarget.Bind(window, process.dwProcessId, process.dwThreadId, process.hProcess)
        && WaitForTitle(window, L"M9 Landmark Interaction", initialTitle)) {
        Sleep(200);
        const bool spentResource = SendKey('2')
            && WaitForTitle(window, L"Command: fatal | ACCEPTED", selectedTitle)
            && SendKey('1')
            && WaitForTitle(window, L"Command: dash | ACCEPTED", selectedTitle);
        const bool approached = spentResource && HoldKey('W', 1200)
            && WaitForTitle(window, L"Selected: Lincoln", selectedTitle);
        Sleep(100);
        selectedPixels = approached ? CountColor(window, RGB(255, 90, 220)) : 0;
        const bool discovered = approached && selectedPixels > 5 && SendKey('E')
            && WaitForTitle(window, L"Interaction Last: Discovered Reward Shadow Restored",
                discoveredTitle)
            && discoveredTitle.find(L"Visited: 1/3") != std::wstring::npos;
        Sleep(100);
        visitedPixels = discovered ? CountColor(window, RGB(80, 235, 125)) : 0;
        const bool repeatSafe = discovered && visitedPixels > 5 && SendKey('E')
            && WaitForTitle(window, L"Interaction Last: Already Visited", repeatedTitle)
            && repeatedTitle.find(L"Visited: 1/3") != std::wstring::npos;
        if (repeatSafe && SendKey(VK_ESCAPE)) {
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
        std::wcerr << L"M9 AUTOMATED NATIVE RUNTIME SMOKE: FAIL\nInitial: " << initialTitle
            << L"\nSelected: " << selectedTitle << L"\nDiscovered: " << discoveredTitle
            << L"\nRepeated: " << repeatedTitle << L"\nSelected/visited pixels: "
            << selectedPixels << L"/" << visitedPixels << L"\nExit: " << exitCode << L"\n";
        return 1;
    }

    std::wcout << L"M9 AUTOMATED NATIVE RUNTIME SMOKE: PASS\nSelected: " << selectedTitle
        << L"\nDiscovered: " << discoveredTitle << L"\nRepeated: " << repeatedTitle
        << L"\nVisible selected/visited pixels: " << selectedPixels << L"/" << visitedPixels
        << L"\nClean Escape exit code: " << exitCode << L"\n";
    return 0;
}
