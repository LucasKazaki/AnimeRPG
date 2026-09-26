#include <windows.h>

#include "Tests/WindowInput.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
Astral::Tests::WindowInputTarget g_inputTarget;
struct WindowSearch {
    DWORD processId{};
    HWND window{};
};

struct FrameEvidence {
    std::uint64_t hash{1469598103934665603ull};
    int gridPixels{};
    int lincolnPixels{};
    int poolPixels{};
    int monumentPixels{};
    int playerPixels{};
    int dummyPixels{};
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
    wchar_t title[512]{};
    GetWindowTextW(window, title, 512);
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
    return g_inputTarget.SendKey(key, 100, 0);
}

bool HoldKey(WORD key, DWORD milliseconds) {
    return g_inputTarget.HoldKey(key, milliseconds, 0);
}

bool CaptureFrame(HWND window, FrameEvidence& evidence) {
    RECT client{};
    if (!GetClientRect(window, &client)) return false;
    const int width = client.right - client.left;
    const int height = client.bottom - client.top;
    if (width <= 0 || height <= 0) return false;

    HDC source = GetDC(window);
    HDC memory = CreateCompatibleDC(source);
    HBITMAP bitmap = CreateCompatibleBitmap(source, width, height);
    HGDIOBJ previous = SelectObject(memory, bitmap);
    // PrintWindow asks the target process for its client rendering and remains
    // stable when the native smoke window is occluded by the test runner. Fall
    // back to a direct client-DC copy for environments that do not implement it.
    BOOL copied = PrintWindow(window, memory, PW_CLIENTONLY);
    if (!copied) {
        copied = BitBlt(memory, 0, 0, width, height, source, 0, 0, SRCCOPY);
    }

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
            evidence.hash ^= static_cast<std::uint64_t>(color);
            evidence.hash *= 1099511628211ull;
            if (color == RGB(35, 52, 78)) ++evidence.gridPixels;
            else if (color == RGB(235, 205, 120)) ++evidence.lincolnPixels;
            else if (color == RGB(70, 190, 235)) ++evidence.poolPixels;
            else if (color == RGB(225, 225, 235)) ++evidence.monumentPixels;
            else if (color == RGB(168, 92, 255)) ++evidence.playerPixels;
            else if (color == RGB(255, 105, 80)) ++evidence.dummyPixels;
        }
    }

    SelectObject(memory, previous);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(window, source);
    return rows == height;
}

bool HasWorldEvidence(const FrameEvidence& frame) {
    return frame.gridPixels > 100 && frame.lincolnPixels > 5 && frame.poolPixels > 5
        && frame.monumentPixels > 5 && frame.playerPixels > 5 && frame.dummyPixels > 5;
}

bool CaptureWorldFrame(HWND window, FrameEvidence& best) {
    int bestScore = -1;
    // GDI invalidation can briefly expose a partially repainted frame when the
    // test host is busy. Ask the target window to finish a paint and wait on
    // the actual world evidence instead of sampling a fixed short delay.
    for (int attempt = 0; attempt < 100; ++attempt) {
        if (!IsWindow(window)) return false;
        FrameEvidence candidate{};
        if (!CaptureFrame(window, candidate)) return false;
        const int score = candidate.gridPixels + candidate.lincolnPixels
            + candidate.poolPixels + candidate.monumentPixels + candidate.playerPixels
            + candidate.dummyPixels;
        if (score > bestScore) {
            best = candidate;
            bestScore = score;
        }
        if (HasWorldEvidence(candidate)) return true;
        Sleep(30);
    }
    return HasWorldEvidence(best);
}
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::cerr << "usage: M5RuntimeSmoke <AstralGame> <working-directory>\n";
        return 1;
    }

    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\"";
    STARTUPINFOW startup{sizeof(startup)};
    Astral::Tests::PrepareNoActivate(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, argv[2],
            &startup, &process)) {
        std::cerr << "M5 AUTOMATED NATIVE RUNTIME SMOKE: FAIL (launch)\n";
        return 1;
    }

    bool passed = false;
    HWND window = WaitForWindow(process.dwProcessId);
    std::wstring initialTitle;
    std::wstring combatTitle;
    std::wstring movedTitle;
    FrameEvidence initialFrame{};
    FrameEvidence beforeMoveFrame{};
    FrameEvidence movedFrame{};
    if (window && g_inputTarget.Bind(window, process.dwProcessId, process.dwThreadId, process.hProcess)
        && WaitForTitle(window, L"M5 Perspective Mall", initialTitle)
        && initialTitle.find(L"Pos: (0.000000, 0.000000)") != std::wstring::npos) {
        Sleep(300);
        const bool initialRendered = CaptureWorldFrame(window, initialFrame);
        const bool combatPersisted = initialRendered && SendKey('J')
            && WaitForTitle(window, L"Last: Light Hit -25", combatTitle)
            && combatTitle.find(L"Dummy: Alive HP: 75/100") != std::wstring::npos;
        Sleep(150);
        const bool capturedBeforeMove = combatPersisted
            && CaptureWorldFrame(window, beforeMoveFrame);
        const bool moved = capturedBeforeMove && HoldKey('W', 600)
            && WaitForTitle(window, L"Pos: (", movedTitle)
            && movedTitle.find(L"Pos: (0.000000, 0.000000)") == std::wstring::npos;
        Sleep(200);
        const bool perspectiveResponded = moved && CaptureWorldFrame(window, movedFrame)
            && movedFrame.hash != beforeMoveFrame.hash;
        if (perspectiveResponded && SendKey(VK_ESCAPE)) {
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

    const auto printFrame = [](const wchar_t* name, const FrameEvidence& frame) {
        std::wcout << name << L" pixels grid/lincoln/pool/monument/player/dummy: "
            << frame.gridPixels << L"/" << frame.lincolnPixels << L"/" << frame.poolPixels
            << L"/" << frame.monumentPixels << L"/" << frame.playerPixels << L"/"
            << frame.dummyPixels << L" hash=" << frame.hash << L"\n";
    };
    if (!passed || exitCode != 0) {
        std::wcerr << L"M5 AUTOMATED NATIVE RUNTIME SMOKE: FAIL\nInitial: " << initialTitle
            << L"\nCombat: " << combatTitle << L"\nMoved: " << movedTitle
            << L"\nExit: " << exitCode << L"\n";
        printFrame(L"Initial", initialFrame);
        printFrame(L"Before move", beforeMoveFrame);
        printFrame(L"Moved", movedFrame);
        return 1;
    }

    std::wcout << L"M5 AUTOMATED NATIVE RUNTIME SMOKE: PASS\nInitial: " << initialTitle
        << L"\nCombat: " << combatTitle << L"\nMoved: " << movedTitle << L"\n";
    printFrame(L"Initial", initialFrame);
    printFrame(L"Before move", beforeMoveFrame);
    printFrame(L"Moved", movedFrame);
    std::wcout << L"Clean Escape exit code: " << exitCode << L"\n";
    return 0;
}
