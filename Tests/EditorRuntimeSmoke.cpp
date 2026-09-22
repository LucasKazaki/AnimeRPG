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
constexpr DWORD kWindowPollIntervalMs = 50;
constexpr DWORD kResizePollIntervalMs = 25;
constexpr DWORD kResizeTimeoutMs = 1500;
constexpr int kStableWindowSamples = 20;
constexpr wchar_t kSceneRootInspectorText[] =
    L"Name: Scene Root\r\nType: Scene\r\n\r\nTransform: n/a\r\n\r\nE11.0 editor fixture";
constexpr wchar_t kCubeInspectorText[] =
    L"Name: Cube\r\nType: Primitive placeholder\r\n\r\nPosition: 0, 0.5, 0\r\n"
    L"Rotation: 0, 0, 0\r\nScale: 1, 1, 1";
constexpr wchar_t kOutlinerLabelText[] = L"OUTLINER";
constexpr wchar_t kInspectorLabelText[] = L"INSPECTOR";
constexpr wchar_t kAssetsLabelText[] = L"ASSETS / DEFAULT PRIMITIVES";
constexpr wchar_t kStatusText[] =
    L"E11.0 editor shell | Outliner selection works | viewport transform tools, undo/redo, "
    L"save/reopen, Play and real asset import pending";
constexpr std::array<const wchar_t*, 4> kExpectedAssetItems{{
    L"Primitive/Cube", L"Primitive/Plane", L"Camera", L"DirectionalLight"}};
constexpr std::array<const wchar_t*, 4> kExpectedShellStaticTexts{{
    kOutlinerLabelText, kInspectorLabelText, kAssetsLabelText, kStatusText}};

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

HWND WaitForStableSingleWindow(DWORD processId, int& observedCount, bool& enumerationFailed) {
    observedCount = 0;
    enumerationFailed = false;
    HWND candidate = nullptr;
    int stableSamples = 0;
    for (int attempt = 0; attempt < 100; ++attempt) {
        std::vector<HWND> windows;
        if (!VisibleProcessWindows(processId, windows)) {
            enumerationFailed = true;
            return nullptr;
        }
        observedCount = static_cast<int>(windows.size());
        if (windows.size() == 1) {
            if (windows.front() == candidate) {
                ++stableSamples;
            } else {
                candidate = windows.front();
                stableSamples = 1;
            }
            if (stableSamples >= kStableWindowSamples) return candidate;
        } else {
            candidate = nullptr;
            stableSamples = 0;
        }
        Sleep(kWindowPollIntervalMs);
    }
    return nullptr;
}

bool RevalidateStableSingleWindow(DWORD processId, HWND expectedWindow,
    int& observedCount, bool& enumerationFailed) {
    const HWND observed = WaitForStableSingleWindow(processId, observedCount, enumerationFailed);
    return observed != nullptr && observed == expectedWindow;
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

bool WindowOwnedByProcess(HWND window, DWORD processId);

bool ResizeAndCheck(HWND window, DWORD processId, int width, int height, std::wstring& failure) {
    if (!WindowOwnedByProcess(window, processId)) {
        failure = L"editor HWND is no longer owned by launched process before resize";
        return false;
    }
    if (!SetWindowPos(window, nullptr, 0, 0, width, height,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS)) {
        failure = L"SetWindowPos failed";
        return false;
    }

    const ULONGLONG deadline = GetTickCount64() + kResizeTimeoutMs;
    while (true) {
        RECT rect{};
        if (!GetWindowRect(window, &rect)) {
            failure = L"GetWindowRect failed while waiting for asynchronous resize";
            return false;
        }
        if (rect.right - rect.left == width && rect.bottom - rect.top == height) {
            return DirectChildrenContained(window, failure);
        }
        if (GetTickCount64() >= deadline) {
            failure = L"asynchronous resize did not complete within deadline";
            return false;
        }
        Sleep(kResizePollIntervalMs);
    }
}

bool ReadListboxValue(HWND listbox, UINT message, WPARAM wParam, LRESULT& value) {
    return SendMessageBounded(listbox, message, wParam, 0, value) && value != LB_ERR;
}

bool ReadListboxText(HWND listbox, int index, std::wstring& text) {
    LRESULT textLength = LB_ERR;
    if (!SendMessageBounded(listbox, LB_GETTEXTLEN, static_cast<WPARAM>(index), 0, textLength)
        || textLength == LB_ERR || textLength < 0 || textLength > 8192) {
        return false;
    }

    std::wstring buffer(static_cast<std::size_t>(textLength) + 1, L'\0');
    LRESULT copied = LB_ERR;
    if (!SendMessageBounded(listbox, LB_GETTEXT, static_cast<WPARAM>(index),
            reinterpret_cast<LPARAM>(buffer.data()), copied)
        || copied == LB_ERR || copied < 0 || copied > textLength) {
        return false;
    }
    buffer.resize(static_cast<std::size_t>(copied));
    text = std::move(buffer);
    return true;
}

bool WindowOwnedByProcess(HWND window, DWORD processId) {
    if (!window) return false;
    DWORD ownerProcessId = 0;
    if (GetWindowThreadProcessId(window, &ownerProcessId) == 0) return false;
    return ownerProcessId == processId;
}

bool DirectVisibleChildOwnedByProcessAndParent(HWND control, HWND parent, DWORD processId,
    const wchar_t* expectedClassName) {
    if (!WindowOwnedByProcess(parent, processId)
        || !WindowOwnedByProcess(control, processId)) {
        return false;
    }
    return GetParent(control) == parent
        && ClassName(control) == expectedClassName
        && IsWindowVisible(control);
}

bool DirectVisibleControlOwnedByProcessAndParent(HWND control, HWND parent, DWORD processId,
    int expectedControlId, const wchar_t* expectedClassName) {
    return DirectVisibleChildOwnedByProcessAndParent(
               control, parent, processId, expectedClassName)
        && GetDlgItem(parent, expectedControlId) == control;
}

bool CloseEditor(HWND window, DWORD processId, HANDLE process, DWORD& exitCode) {
    if (!WindowOwnedByProcess(window, processId)) return false;
    if (!PostMessageW(window, WM_CLOSE, 0, 0)) return false;
    if (WaitForSingleObject(process, kProcessExitTimeoutMs) != WAIT_OBJECT_0) return false;
    return GetExitCodeProcess(process, &exitCode) != FALSE;
}

void CleanupProcess(HANDLE process) {
    const DWORD state = WaitForSingleObject(process, 0);
    if (state == WAIT_OBJECT_0) return;
    if (TerminateProcess(process, 2)) {
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
    window = WaitForStableSingleWindow(
        process.dwProcessId, visibleTopLevelCount, topLevelEnumerationFailed);
    if (!window) {
        if (topLevelEnumerationFailed) {
            failure = L"EnumWindows failed while locating the editor top-level window";
        } else {
            failure = L"expected one stable visible process-owned top-level window, observed "
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
            const HWND inspector = FindControlByText(controls, L"Static", kSceneRootInspectorText);
            if (pendingStateOk) {
                for (const wchar_t* expectedText : kExpectedShellStaticTexts) {
                    const HWND staticControl = FindControlByText(controls, L"Static", expectedText);
                    if (!staticControl || !IsWindowVisible(staticControl)) {
                        failure = L"required shell static is missing, hidden, or mislabeled: "
                            + std::wstring(expectedText);
                        pendingStateOk = false;
                        break;
                    }
                }
            }
            if (pendingStateOk
                && (!DirectVisibleControlOwnedByProcessAndParent(
                        outliner, window, process.dwProcessId, kOutlinerId, L"ListBox")
                    || !DirectVisibleControlOwnedByProcessAndParent(
                        assets, window, process.dwProcessId, kAssetListId, L"ListBox")
                    || !DirectVisibleChildOwnedByProcessAndParent(
                        inspector, window, process.dwProcessId, L"Static"))) {
                failure = L"required Outliner/assets/Inspector surface identity, ownership, parent, class, or visibility is invalid";
                pendingStateOk = false;
            }

            LRESULT outlinerCount = LB_ERR;
            LRESULT assetCount = LB_ERR;
            LRESULT selection = LB_ERR;
            std::wstring sceneRootItem;
            std::wstring cubeItem;
            bool assetItemsMatch = true;
            if (pendingStateOk) {
                for (std::size_t index = 0; index < kExpectedAssetItems.size(); ++index) {
                    std::wstring assetItem;
                    if (!ReadListboxText(assets, static_cast<int>(index), assetItem)
                        || assetItem != kExpectedAssetItems[index]) {
                        assetItemsMatch = false;
                        break;
                    }
                }
            }
            if (pendingStateOk
                && (!ReadListboxValue(outliner, LB_GETCOUNT, 0, outlinerCount)
                    || !ReadListboxValue(assets, LB_GETCOUNT, 0, assetCount)
                    || !ReadListboxValue(outliner, LB_GETCURSEL, 0, selection)
                    || !ReadListboxText(outliner, 0, sceneRootItem)
                    || !ReadListboxText(outliner, 3, cubeItem)
                    || outlinerCount != 5 || assetCount != 4 || !assetItemsMatch || selection != 0
                    || sceneRootItem != L"Scene Root" || cubeItem != L"Cube"
                    || WindowText(inspector) != kSceneRootInspectorText)) {
                failure = L"unexpected initial Outliner/assets/Inspector fixture state";
                pendingStateOk = false;
            }

            if (pendingStateOk) {
                LRESULT newSelection = LB_ERR;
                LRESULT commandResult = 0;
                const bool selectionTargetValid = DirectVisibleControlOwnedByProcessAndParent(
                    outliner, window, process.dwProcessId, kOutlinerId, L"ListBox");
                const bool selected = selectionTargetValid
                    && SendMessageBounded(outliner, LB_SETCURSEL, 3, 0, newSelection)
                    && newSelection != LB_ERR;
                const bool notificationTargetValid = selected
                    && DirectVisibleControlOwnedByProcessAndParent(
                        outliner, window, process.dwProcessId, kOutlinerId, L"ListBox");
                const bool notified = notificationTargetValid
                    && SendMessageBounded(window, WM_COMMAND,
                        MAKEWPARAM(kOutlinerId, LBN_SELCHANGE),
                        reinterpret_cast<LPARAM>(outliner), commandResult);
                LRESULT confirmedSelection = LB_ERR;
                std::wstring confirmedItem;
                const bool synchronized = notified
                    && DirectVisibleControlOwnedByProcessAndParent(
                        outliner, window, process.dwProcessId, kOutlinerId, L"ListBox")
                    && DirectVisibleChildOwnedByProcessAndParent(
                        inspector, window, process.dwProcessId, L"Static")
                    && ReadListboxValue(outliner, LB_GETCURSEL, 0, confirmedSelection)
                    && confirmedSelection == 3
                    && ReadListboxText(outliner, static_cast<int>(confirmedSelection), confirmedItem)
                    && confirmedItem == L"Cube"
                    && WindowText(inspector) == kCubeInspectorText;
                if (!synchronized) {
                    failure = L"Outliner Cube selection and Inspector fixture did not remain synchronized after notification";
                    pendingStateOk = false;
                }
            }

            if (pendingStateOk
                && !ResizeAndCheck(window, process.dwProcessId, 800, 600, failure)) {
                pendingStateOk = false;
            }
            if (pendingStateOk
                && !ResizeAndCheck(window, process.dwProcessId, 420, 260, failure)) {
                pendingStateOk = false;
            }

            int finalVisibleTopLevelCount = 0;
            bool finalTopLevelEnumerationFailed = false;
            if (pendingStateOk
                && !RevalidateStableSingleWindow(process.dwProcessId, window,
                    finalVisibleTopLevelCount, finalTopLevelEnumerationFailed)) {
                if (finalTopLevelEnumerationFailed) {
                    failure = L"EnumWindows failed during final editor-window revalidation";
                } else {
                    failure = L"editor did not remain one stable visible process-owned top-level "
                        L"window; observed " + std::to_wstring(finalVisibleTopLevelCount);
                }
                pendingStateOk = false;
            }

            if (pendingStateOk
                && CloseEditor(window, process.dwProcessId, process.hProcess, exitCode)
                && exitCode == 0) {
                passed = true;
            } else if (pendingStateOk) {
                failure = L"editor did not close cleanly with exit code 0";
            }
        }
    }

    if (!passed) CleanupProcess(process.hProcess);
    if (!passed && GetExitCodeProcess(process.hProcess, &exitCode) == FALSE) exitCode = 1;

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    if (!passed) {
        std::wcerr << L"EDITOR AUTOMATED NATIVE RUNTIME SMOKE: FAIL\nReason: " << failure
            << L"\nExit: " << exitCode << L"\n";
        return 1;
    }

    std::wcout << L"EDITOR AUTOMATED NATIVE RUNTIME SMOKE: PASS\n"
        << L"Observed one stable visible process-owned top-level editor window before and after "
        << L"interaction, 12 required controls, disabled pending tools, exact shell labels/status, "
        << L"exact visible process-owned Outliner/assets surfaces, exact item identities and complete "
        << L"Inspector fixture text with post-notification selection ownership+synchronization, "
        << L"ownership-checked bounded asynchronous normal+narrow resizes, and clean exit.\n";
    return 0;
}
