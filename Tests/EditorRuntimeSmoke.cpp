#include <windows.h>

#include <array>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr int kOutlinerId = 1001;
constexpr int kAssetListId = 1002;
constexpr DWORD kMessageTimeoutMs = 1000;
constexpr DWORD kProcessExitTimeoutMs = 5000;
constexpr DWORD kCleanupTimeoutMs = 2000;

struct ProcessWindowCollection {
    DWORD processId{};
    std::vector<HWND> windows;
};

struct ChildControl {
    HWND handle{};
    std::wstring className;
    std::wstring text;
};

struct ChildCollection {
    HWND parent{};
    std::vector<ChildControl> controls;
};

BOOL CALLBACK CollectVisibleProcessWindow(HWND window, LPARAM parameter) {
    auto& collection = *reinterpret_cast<ProcessWindowCollection*>(parameter);
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId == collection.processId && IsWindowVisible(window)) {
        collection.windows.push_back(window);
    }
    return TRUE;
}

bool VisibleProcessWindows(DWORD processId, std::vector<HWND>& windows) {
    ProcessWindowCollection collection{processId, {}};
    if (!EnumWindows(CollectVisibleProcessWindow, reinterpret_cast<LPARAM>(&collection))) {
        return false;
    }
    windows = std::move(collection.windows);
    return true;
}

HWND WaitForSingleWindow(DWORD processId, int& observedCount, bool& enumerationFailed) {
    observedCount = 0;
    enumerationFailed = false;
    for (int attempt = 0; attempt < 100; ++attempt) {
        std::vector<HWND> windows;
        if (!VisibleProcessWindows(processId, windows)) {
            enumerationFailed = true;
            return nullptr;
        }
        observedCount = static_cast<int>(windows.size());
        if (windows.size() == 1) return windows.front();
        Sleep(50);
    }
    return nullptr;
}

std::wstring ClassName(HWND window) {
    wchar_t buffer[128]{};
    if (GetClassNameW(window, buffer, 128) <= 0) return {};
    return buffer;
}

bool SendMessageBounded(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
    LRESULT& result) {
    DWORD_PTR rawResult = 0;
    const LRESULT sent = SendMessageTimeoutW(window, message, wParam, lParam,
        SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, kMessageTimeoutMs, &rawResult);
    if (sent == 0) return false;
    result = static_cast<LRESULT>(rawResult);
    return true;
}

std::wstring WindowText(HWND window) {
    LRESULT textLength = 0;
    if (!SendMessageBounded(window, WM_GETTEXTLENGTH, 0, 0, textLength) || textLength < 0
        || textLength > 8192) {
        return {};
    }

    std::wstring text(static_cast<std::size_t>(textLength) + 1, L'\0');
    LRESULT copied = 0;
    if (!SendMessageBounded(window, WM_GETTEXT, text.size(),
            reinterpret_cast<LPARAM>(text.data()), copied)
        || copied < 0) {
        return {};
    }
    text.resize(static_cast<std::size_t>(copied));
    return text;
}

BOOL CALLBACK CollectDirectChild(HWND child, LPARAM parameter) {
    auto& collection = *reinterpret_cast<ChildCollection*>(parameter);
    if (GetParent(child) == collection.parent) {
        collection.controls.push_back({child, ClassName(child), WindowText(child)});
    }
    return TRUE;
}

std::vector<ChildControl> DirectChildren(HWND window) {
    ChildCollection collection{window, {}};
    EnumChildWindows(window, CollectDirectChild, reinterpret_cast<LPARAM>(&collection));
    return collection.controls;
}

int CountClass(const std::vector<ChildControl>& controls, const wchar_t* className) {
    int count = 0;
    for (const auto& control : controls) {
        if (control.className == className) ++count;
    }
    return count;
}

HWND FindControlByText(const std::vector<ChildControl>& controls,
    const wchar_t* className, const std::wstring& exactText) {
    for (const auto& control : controls) {
        if (control.className == className && control.text == exactText) return control.handle;
    }
    return nullptr;
}

HWND FindControlByPrefix(const std::vector<ChildControl>& controls,
    const wchar_t* className, const std::wstring& prefix) {
    for (const auto& control : controls) {
        if (control.className == className && control.text.rfind(prefix, 0) == 0) {
            return control.handle;
        }
    }
    return nullptr;
}

bool DirectChildrenContained(HWND window, std::wstring& failure) {
    RECT client{};
    if (!GetClientRect(window, &client)) {
        failure = L"GetClientRect failed";
        return false;
    }

    const auto controls = DirectChildren(window);
    if (controls.size() != 12) {
        failure = L"expected 12 direct controls, observed " + std::to_wstring(controls.size());
        return false;
    }

    for (const auto& control : controls) {
        RECT rect{};
        if (!GetWindowRect(control.handle, &rect)) {
            failure = L"GetWindowRect failed for " + control.className;
            return false;
        }
        POINT points[2]{{rect.left, rect.top}, {rect.right, rect.bottom}};
        MapWindowPoints(HWND_DESKTOP, window, points, 2);
        if (points[0].x < client.left || points[0].y < client.top
            || points[1].x > client.right || points[1].y > client.bottom
            || points[1].x < points[0].x || points[1].y < points[0].y) {
            failure = L"child outside client: " + control.className + L" text='" + control.text
                + L"' child=" + std::to_wstring(points[0].x) + L","
                + std::to_wstring(points[0].y) + L".." + std::to_wstring(points[1].x) + L","
                + std::to_wstring(points[1].y) + L" client=" + std::to_wstring(client.right)
                + L"x" + std::to_wstring(client.bottom);
            return false;
        }
    }
    return true;
}

bool ResizeAndCheck(HWND window, int width, int height, std::wstring& failure) {
    if (!SetWindowPos(window, nullptr, 0, 0, width, height,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)) {
        failure = L"SetWindowPos failed";
        return false;
    }
    Sleep(150);
    return DirectChildrenContained(window, failure);
}

bool ReadListboxValue(HWND listbox, UINT message, WPARAM wParam, LRESULT& value) {
    return SendMessageBounded(listbox, message, wParam, 0, value) && value != LB_ERR;
}

bool CloseEditor(HWND window, HANDLE process, DWORD& exitCode) {
    if (window) PostMessageW(window, WM_CLOSE, 0, 0);
    if (WaitForSingleObject(process, kProcessExitTimeoutMs) != WAIT_OBJECT_0) return false;
    return GetExitCodeProcess(process, &exitCode) != FALSE;
}

void CleanupProcess(HWND window, HANDLE process) {
    if (window) PostMessageW(window, WM_CLOSE, 0, 0);
    if (WaitForSingleObject(process, kCleanupTimeoutMs) != WAIT_OBJECT_0) {
        TerminateProcess(process, 2);
        WaitForSingleObject(process, kCleanupTimeoutMs);
    }
}
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::cerr << "usage: EditorRuntimeSmoke <AstralEditor> <working-directory>\n";
        return 1;
    }

    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\"";
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, argv[2],
            &startup, &process)) {
        std::cerr << "EDITOR AUTOMATED NATIVE RUNTIME SMOKE: FAIL (launch)\n";
        return 1;
    }

    HWND window = nullptr;
    bool passed = false;
    std::wstring failure;
    DWORD exitCode = 1;

    WaitForInputIdle(process.hProcess, 5000);
    int visibleTopLevelCount = 0;
    bool topLevelEnumerationFailed = false;
    window = WaitForSingleWindow(
        process.dwProcessId, visibleTopLevelCount, topLevelEnumerationFailed);
    if (!window) {
        if (topLevelEnumerationFailed) {
            failure = L"EnumWindows failed while locating the editor top-level window";
        } else {
            failure = L"expected exactly one visible process-owned top-level window, observed "
                + std::to_wstring(visibleTopLevelCount);
        }
    } else if (ClassName(window) != L"AstralEditorWindow") {
        failure = L"unexpected editor window class: " + ClassName(window);
    } else if (WindowText(window) != L"Astral Editor 0.1") {
        failure = L"unexpected editor title: " + WindowText(window);
    } else {
        const auto controls = DirectChildren(window);
        if (controls.size() != 12) {
            failure = L"expected 12 direct controls, observed " + std::to_wstring(controls.size());
        } else if (CountClass(controls, L"Button") != 5
            || CountClass(controls, L"ListBox") != 2
            || CountClass(controls, L"Static") != 5) {
            failure = L"unexpected child-control class counts";
        } else {
            const std::array<std::wstring, 5> pendingButtons{{
                L"Select (pending)", L"Move (pending)", L"Rotate (pending)",
                L"Scale (pending)", L"Play (pending)"}};
            bool pendingStateOk = true;
            for (const auto& caption : pendingButtons) {
                const HWND button = FindControlByText(controls, L"Button", caption);
                if (!button || !IsWindowVisible(button) || IsWindowEnabled(button)) {
                    pendingStateOk = false;
                    failure = L"pending tool is missing, hidden, or enabled: " + caption;
                    break;
                }
            }

            const HWND outliner = GetDlgItem(window, kOutlinerId);
            const HWND assets = GetDlgItem(window, kAssetListId);
            const HWND inspector = FindControlByPrefix(controls, L"Static", L"Name: Scene Root");
            if (pendingStateOk && (!outliner || !assets || !inspector)) {
                failure = L"required Outliner/assets/Inspector control not found";
                pendingStateOk = false;
            }

            LRESULT outlinerCount = LB_ERR;
            LRESULT assetCount = LB_ERR;
            LRESULT selection = LB_ERR;
            if (pendingStateOk
                && (!ReadListboxValue(outliner, LB_GETCOUNT, 0, outlinerCount)
                    || !ReadListboxValue(assets, LB_GETCOUNT, 0, assetCount)
                    || !ReadListboxValue(outliner, LB_GETCURSEL, 0, selection)
                    || outlinerCount != 5 || assetCount != 4 || selection != 0)) {
                failure = L"unexpected initial Outliner/assets state";
                pendingStateOk = false;
            }

            if (pendingStateOk) {
                LRESULT newSelection = LB_ERR;
                LRESULT commandResult = 0;
                const bool selected = ReadListboxValue(outliner, LB_SETCURSEL, 3, newSelection)
                    && newSelection == 3;
                const bool notified = selected && SendMessageBounded(window, WM_COMMAND,
                    MAKEWPARAM(kOutlinerId, LBN_SELCHANGE),
                    reinterpret_cast<LPARAM>(outliner), commandResult);
                if (!notified || WindowText(inspector).rfind(L"Name: Cube", 0) != 0) {
                    failure = L"Outliner Cube selection did not update Inspector";
                    pendingStateOk = false;
                }
            }

            if (pendingStateOk && !ResizeAndCheck(window, 800, 600, failure)) {
                pendingStateOk = false;
            }
            if (pendingStateOk && !ResizeAndCheck(window, 420, 260, failure)) {
                pendingStateOk = false;
            }

            if (pendingStateOk && CloseEditor(window, process.hProcess, exitCode)
                && exitCode == 0) {
                passed = true;
            } else if (pendingStateOk) {
                failure = L"editor did not close cleanly with exit code 0";
            }
        }
    }

    if (!passed) CleanupProcess(window, process.hProcess);
    if (!passed && GetExitCodeProcess(process.hProcess, &exitCode) == FALSE) exitCode = 1;

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    if (!passed) {
        std::wcerr << L"EDITOR AUTOMATED NATIVE RUNTIME SMOKE: FAIL\nReason: " << failure
            << L"\nExit: " << exitCode << L"\n";
        return 1;
    }

    std::wcout << L"EDITOR AUTOMATED NATIVE RUNTIME SMOKE: PASS\n"
        << L"Observed exactly one visible process-owned top-level editor window, 12 required "
        << L"controls, disabled pending tools, Outliner/Inspector selection sync, contained "
        << L"normal+narrow layouts, and clean exit.\n";
    return 0;
}
