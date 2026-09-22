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
constexpr std::array<const wchar_t*, 5> kExpectedOutlinerItems{{
    L"Scene Root", L"Camera", L"Directional Light", L"Cube", L"Floor"}};
constexpr std::array<const wchar_t*, 4> kExpectedAssetItems{{
    L"Primitive/Cube", L"Primitive/Plane", L"Camera", L"DirectionalLight"}};
constexpr std::array<const wchar_t*, 5> kPendingButtons{{
    L"Select (pending)", L"Move (pending)", L"Rotate (pending)", L"Scale (pending)",
    L"Play (pending)"}};

struct ProcessWindowCollection {
    DWORD processId{};
    std::vector<HWND> windows;
};

struct ChildControl {
    HWND handle{};
    std::wstring className;
};

struct ChildCollection {
    HWND parent{};
    DWORD processId{};
    std::vector<ChildControl> controls;
};

struct ShellStaticHandles {
    HWND outlinerLabel{};
    HWND inspectorLabel{};
    HWND inspector{};
    HWND assetsLabel{};
    HWND status{};
};

bool WindowOwnedByProcess(HWND window, DWORD processId) {
    if (!window) return false;
    DWORD ownerProcessId = 0;
    if (GetWindowThreadProcessId(window, &ownerProcessId) == 0) return false;
    return ownerProcessId == processId;
}

std::wstring ClassName(HWND window) {
    wchar_t buffer[128]{};
    if (GetClassNameW(window, buffer, 128) <= 0) return {};
    return buffer;
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
        && GetDlgItem(parent, expectedControlId) == control
        && IsWindowEnabled(control);
}

bool ValidatedControlHasStyle(HWND control, HWND parent, DWORD processId,
    int expectedControlId, const wchar_t* expectedClassName, LONG_PTR requiredStyle) {
    if (!DirectVisibleControlOwnedByProcessAndParent(
            control, parent, processId, expectedControlId, expectedClassName)) {
        return false;
    }
    SetLastError(ERROR_SUCCESS);
    const LONG_PTR style = GetWindowLongPtrW(control, GWL_STYLE);
    if (style == 0 && GetLastError() != ERROR_SUCCESS) return false;
    if (!DirectVisibleControlOwnedByProcessAndParent(
            control, parent, processId, expectedControlId, expectedClassName)) {
        return false;
    }
    return (style & requiredStyle) == requiredStyle;
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

bool ReadOwnedWindowText(HWND window, DWORD processId, std::wstring& text) {
    if (!WindowOwnedByProcess(window, processId)) return false;
    LRESULT textLength = 0;
    if (!SendMessageBounded(window, WM_GETTEXTLENGTH, 0, 0, textLength)
        || textLength < 0 || textLength > 8192) {
        return false;
    }

    std::wstring buffer(static_cast<std::size_t>(textLength) + 1, L'\0');
    if (!WindowOwnedByProcess(window, processId)) return false;
    LRESULT copied = 0;
    if (!SendMessageBounded(window, WM_GETTEXT, buffer.size(),
            reinterpret_cast<LPARAM>(buffer.data()), copied)
        || copied < 0 || copied > textLength) {
        return false;
    }
    buffer.resize(static_cast<std::size_t>(copied));
    text = std::move(buffer);
    return true;
}

bool ReadValidatedChildText(HWND control, HWND parent, DWORD processId,
    const wchar_t* expectedClassName, std::wstring& text) {
    if (!DirectVisibleChildOwnedByProcessAndParent(
            control, parent, processId, expectedClassName)) {
        return false;
    }
    LRESULT textLength = 0;
    if (!SendMessageBounded(control, WM_GETTEXTLENGTH, 0, 0, textLength)
        || textLength < 0 || textLength > 8192) {
        return false;
    }

    std::wstring buffer(static_cast<std::size_t>(textLength) + 1, L'\0');
    if (!DirectVisibleChildOwnedByProcessAndParent(
            control, parent, processId, expectedClassName)) {
        return false;
    }
    LRESULT copied = 0;
    if (!SendMessageBounded(control, WM_GETTEXT, buffer.size(),
            reinterpret_cast<LPARAM>(buffer.data()), copied)
        || copied < 0 || copied > textLength) {
        return false;
    }
    buffer.resize(static_cast<std::size_t>(copied));
    text = std::move(buffer);
    return true;
}

bool ReadValidatedListboxValue(HWND listbox, HWND parent, DWORD processId, int controlId,
    UINT message, WPARAM wParam, LRESULT& value) {
    if (!DirectVisibleControlOwnedByProcessAndParent(
            listbox, parent, processId, controlId, L"ListBox")) {
        return false;
    }
    return SendMessageBounded(listbox, message, wParam, 0, value) && value != LB_ERR;
}

bool ReadValidatedListboxText(HWND listbox, HWND parent, DWORD processId, int controlId,
    int index, std::wstring& text) {
    if (!DirectVisibleControlOwnedByProcessAndParent(
            listbox, parent, processId, controlId, L"ListBox")) {
        return false;
    }
    LRESULT textLength = LB_ERR;
    if (!SendMessageBounded(listbox, LB_GETTEXTLEN, static_cast<WPARAM>(index), 0, textLength)
        || textLength == LB_ERR || textLength < 0 || textLength > 8192) {
        return false;
    }

    std::wstring buffer(static_cast<std::size_t>(textLength) + 1, L'\0');
    if (!DirectVisibleControlOwnedByProcessAndParent(
            listbox, parent, processId, controlId, L"ListBox")) {
        return false;
    }
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

BOOL CALLBACK CollectDirectChild(HWND child, LPARAM parameter) {
    auto& collection = *reinterpret_cast<ChildCollection*>(parameter);
    if (GetParent(child) == collection.parent
        && WindowOwnedByProcess(child, collection.processId)) {
        collection.controls.push_back({child, ClassName(child)});
    }
    return TRUE;
}

std::vector<ChildControl> DirectChildren(HWND window, DWORD processId) {
    ChildCollection collection{window, processId, {}};
    EnumChildWindows(window, CollectDirectChild, reinterpret_cast<LPARAM>(&collection));
    return collection.controls;
}

bool SameControlHandles(const std::vector<ChildControl>& expected,
    const std::vector<ChildControl>& observed) {
    if (expected.size() != observed.size()) return false;
    std::vector<bool> matched(observed.size(), false);
    for (const auto& expectedControl : expected) {
        bool found = false;
        for (std::size_t index = 0; index < observed.size(); ++index) {
            if (!matched[index]
                && observed[index].handle == expectedControl.handle
                && observed[index].className == expectedControl.className) {
                matched[index] = true;
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}

bool CaptureInitialControlInventory(HWND window, DWORD processId,
    std::vector<ChildControl>& controls, std::wstring& failure) {
    if (!WindowOwnedByProcess(window, processId)) {
        failure = L"editor top-level ownership changed before initial child inventory capture";
        return false;
    }
    controls = DirectChildren(window, processId);
    if (controls.size() != 12) {
        failure = L"expected 12 direct process-owned controls at initial capture, observed "
            + std::to_wstring(controls.size());
        return false;
    }
    return true;
}

int CountClass(const std::vector<ChildControl>& controls, const wchar_t* className) {
    int count = 0;
    for (const auto& control : controls) {
        if (control.className == className) ++count;
    }
    return count;
}

HWND FindDirectVisibleChildByText(const std::vector<ChildControl>& controls, HWND parent,
    DWORD processId, const wchar_t* className, const std::wstring& exactText) {
    for (const auto& control : controls) {
        if (control.className != className) continue;
        std::wstring text;
        if (ReadValidatedChildText(control.handle, parent, processId, className, text)
            && text == exactText) {
            return control.handle;
        }
    }
    return nullptr;
}

bool CaptureInitialStaticHandles(const std::vector<ChildControl>& controls, HWND parent,
    DWORD processId, ShellStaticHandles& statics, std::wstring& failure) {
    statics.outlinerLabel = FindDirectVisibleChildByText(
        controls, parent, processId, L"Static", kOutlinerLabelText);
    statics.inspectorLabel = FindDirectVisibleChildByText(
        controls, parent, processId, L"Static", kInspectorLabelText);
    statics.inspector = FindDirectVisibleChildByText(
        controls, parent, processId, L"Static", kSceneRootInspectorText);
    statics.assetsLabel = FindDirectVisibleChildByText(
        controls, parent, processId, L"Static", kAssetsLabelText);
    statics.status = FindDirectVisibleChildByText(
        controls, parent, processId, L"Static", kStatusText);

    const std::array<HWND, 5> handles{{
        statics.outlinerLabel, statics.inspectorLabel, statics.inspector,
        statics.assetsLabel, statics.status}};
    for (std::size_t i = 0; i < handles.size(); ++i) {
        if (!handles[i]) {
            failure = L"failed to bind every initial semantic Static control";
            return false;
        }
        for (std::size_t j = i + 1; j < handles.size(); ++j) {
            if (handles[i] == handles[j]) {
                failure = L"initial semantic Static controls are not unique HWNDs";
                return false;
            }
        }
    }
    return true;
}

bool ValidateBoundStaticText(HWND control, HWND parent, DWORD processId,
    const wchar_t* expectedText, const wchar_t* semanticName, std::wstring& failure) {
    std::wstring observed;
    if (!ReadValidatedChildText(control, parent, processId, L"Static", observed)
        || observed != expectedText) {
        failure = L"semantic Static changed identity/text: " + std::wstring(semanticName);
        return false;
    }
    return true;
}

bool ValidateShellState(HWND window, DWORD processId,
    const std::vector<ChildControl>& initialControls, const ShellStaticHandles& statics,
    const wchar_t* expectedInspectorText, int expectedSelection, std::wstring& failure) {
    if (!WindowOwnedByProcess(window, processId) || !IsWindowVisible(window)
        || !IsWindowEnabled(window)) {
        failure = L"editor top-level ownership, visibility, or enabled state changed";
        return false;
    }
    if (ClassName(window) != L"AstralEditorWindow") {
        failure = L"unexpected editor window class";
        return false;
    }
    std::wstring title;
    if (!ReadOwnedWindowText(window, processId, title) || title != L"Astral Editor 0.1") {
        failure = L"unexpected editor title";
        return false;
    }

    const auto controls = DirectChildren(window, processId);
    if (controls.size() != 12) {
        failure = L"expected 12 direct process-owned controls, observed "
            + std::to_wstring(controls.size());
        return false;
    }
    if (!SameControlHandles(initialControls, controls)) {
        failure = L"direct child HWND inventory no longer matches the initial shell inventory";
        return false;
    }
    if (CountClass(controls, L"Button") != 5
        || CountClass(controls, L"ListBox") != 2
        || CountClass(controls, L"Static") != 5) {
        failure = L"unexpected child-control class counts";
        return false;
    }

    for (const wchar_t* caption : kPendingButtons) {
        const HWND button = FindDirectVisibleChildByText(
            controls, window, processId, L"Button", caption);
        if (!button || !DirectVisibleChildOwnedByProcessAndParent(
                button, window, processId, L"Button")
            || IsWindowEnabled(button)) {
            failure = L"pending tool is missing, hidden, replaced, relabeled, or enabled: "
                + std::wstring(caption);
            return false;
        }
    }

    if (!ValidateBoundStaticText(statics.outlinerLabel, window, processId,
            kOutlinerLabelText, L"Outliner label", failure)
        || !ValidateBoundStaticText(statics.inspectorLabel, window, processId,
            kInspectorLabelText, L"Inspector label", failure)
        || !ValidateBoundStaticText(statics.assetsLabel, window, processId,
            kAssetsLabelText, L"Assets label", failure)
        || !ValidateBoundStaticText(statics.status, window, processId,
            kStatusText, L"Status", failure)
        || !ValidateBoundStaticText(statics.inspector, window, processId,
            expectedInspectorText, L"Inspector body", failure)) {
        return false;
    }

    const HWND outliner = GetDlgItem(window, kOutlinerId);
    const HWND assets = GetDlgItem(window, kAssetListId);
    if (!DirectVisibleControlOwnedByProcessAndParent(
            outliner, window, processId, kOutlinerId, L"ListBox")
        || !DirectVisibleControlOwnedByProcessAndParent(
            assets, window, processId, kAssetListId, L"ListBox")) {
        failure = L"required Outliner/assets surface identity, ownership, parent, class, ID, visibility, or enabled state is invalid";
        return false;
    }
    if (!ValidatedControlHasStyle(outliner, window, processId,
            kOutlinerId, L"ListBox", static_cast<LONG_PTR>(LBS_NOTIFY))) {
        failure = L"Outliner is missing LBS_NOTIFY required for user-driven selection notifications";
        return false;
    }

    LRESULT outlinerCount = LB_ERR;
    LRESULT assetCount = LB_ERR;
    LRESULT selection = LB_ERR;
    if (!ReadValidatedListboxValue(outliner, window, processId, kOutlinerId,
            LB_GETCOUNT, 0, outlinerCount)
        || !ReadValidatedListboxValue(assets, window, processId, kAssetListId,
            LB_GETCOUNT, 0, assetCount)
        || !ReadValidatedListboxValue(outliner, window, processId, kOutlinerId,
            LB_GETCURSEL, 0, selection)
        || outlinerCount != static_cast<LRESULT>(kExpectedOutlinerItems.size())
        || assetCount != static_cast<LRESULT>(kExpectedAssetItems.size())
        || selection != expectedSelection) {
        failure = L"unexpected Outliner/assets count or selection state";
        return false;
    }

    for (std::size_t index = 0; index < kExpectedOutlinerItems.size(); ++index) {
        std::wstring outlinerItem;
        if (!ReadValidatedListboxText(outliner, window, processId, kOutlinerId,
                static_cast<int>(index), outlinerItem)
            || outlinerItem != kExpectedOutlinerItems[index]) {
            failure = L"unexpected Outliner item identity at row " + std::to_wstring(index);
            return false;
        }
    }

    for (std::size_t index = 0; index < kExpectedAssetItems.size(); ++index) {
        std::wstring assetItem;
        if (!ReadValidatedListboxText(assets, window, processId, kAssetListId,
                static_cast<int>(index), assetItem)
            || assetItem != kExpectedAssetItems[index]) {
            failure = L"unexpected asset item identity at row " + std::to_wstring(index);
            return false;
        }
    }

    const auto finalControls = DirectChildren(window, processId);
    if (!SameControlHandles(initialControls, finalControls)) {
        failure = L"direct child HWND inventory changed during shell-state validation";
        return false;
    }
    return true;
}

bool DirectChildrenContained(HWND window, DWORD processId,
    const std::vector<ChildControl>& initialControls, std::wstring& failure) {
    if (!WindowOwnedByProcess(window, processId)) {
        failure = L"editor top-level ownership changed before containment check";
        return false;
    }
    RECT client{};
    if (!GetClientRect(window, &client)) {
        failure = L"GetClientRect failed";
        return false;
    }

    const auto controls = DirectChildren(window, processId);
    if (controls.size() != 12) {
        failure = L"expected 12 direct process-owned controls during containment, observed "
            + std::to_wstring(controls.size());
        return false;
    }
    if (!SameControlHandles(initialControls, controls)) {
        failure = L"direct child HWND inventory changed before containment validation";
        return false;
    }

    for (const auto& control : controls) {
        if (!DirectVisibleChildOwnedByProcessAndParent(
                control.handle, window, processId, control.className.c_str())) {
            failure = L"child identity, ownership, parent, class, or visibility changed during containment";
            return false;
        }
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
            failure = L"child outside client: " + control.className
                + L" child=" + std::to_wstring(points[0].x) + L"," + std::to_wstring(points[0].y)
                + L".." + std::to_wstring(points[1].x) + L"," + std::to_wstring(points[1].y)
                + L" client=" + std::to_wstring(client.right) + L"x"
                + std::to_wstring(client.bottom);
            return false;
        }
    }
    return true;
}

bool ResizeAndCheck(HWND window, DWORD processId,
    const std::vector<ChildControl>& initialControls, const ShellStaticHandles& statics,
    int width, int height, const wchar_t* expectedInspectorText,
    int expectedSelection, std::wstring& failure) {
    if (!WindowOwnedByProcess(window, processId) || !IsWindowVisible(window)
        || !IsWindowEnabled(window)) {
        failure = L"editor HWND ownership, visibility, or enabled state changed before resize";
        return false;
    }
    if (!SameControlHandles(initialControls, DirectChildren(window, processId))) {
        failure = L"direct child HWND inventory changed before resize";
        return false;
    }
    if (!SetWindowPos(window, nullptr, 0, 0, width, height,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS)) {
        failure = L"SetWindowPos failed";
        return false;
    }

    const ULONGLONG deadline = GetTickCount64() + kResizeTimeoutMs;
    while (true) {
        if (!WindowOwnedByProcess(window, processId) || !IsWindowVisible(window)
            || !IsWindowEnabled(window)) {
            failure = L"editor HWND ownership, visibility, or enabled state changed while waiting for asynchronous resize";
            return false;
        }
        RECT rect{};
        if (!GetWindowRect(window, &rect)) {
            failure = L"GetWindowRect failed while waiting for asynchronous resize";
            return false;
        }
        if (rect.right - rect.left == width && rect.bottom - rect.top == height) {
            if (!DirectChildrenContained(window, processId, initialControls, failure)) return false;
            return ValidateShellState(window, processId, initialControls, statics,
                expectedInspectorText, expectedSelection, failure);
        }
        if (GetTickCount64() >= deadline) {
            failure = L"asynchronous resize did not complete within deadline";
            return false;
        }
        Sleep(kResizePollIntervalMs);
    }
}

bool SelectCubeAndNotify(HWND window, DWORD processId,
    const std::vector<ChildControl>& initialControls, const ShellStaticHandles& statics,
    std::wstring& failure) {
    if (!WindowOwnedByProcess(window, processId) || !IsWindowVisible(window)
        || !IsWindowEnabled(window)) {
        failure = L"editor top-level ownership, visibility, or enabled state changed before selection";
        return false;
    }
    if (!SameControlHandles(initialControls, DirectChildren(window, processId))) {
        failure = L"direct child HWND inventory changed before selection";
        return false;
    }

    const HWND outliner = GetDlgItem(window, kOutlinerId);
    if (!ValidatedControlHasStyle(outliner, window, processId,
            kOutlinerId, L"ListBox", static_cast<LONG_PTR>(LBS_NOTIFY))) {
        failure = L"Outliner target is invalid, disabled, or missing LBS_NOTIFY before selection";
        return false;
    }

    LRESULT newSelection = LB_ERR;
    if (!SendMessageBounded(outliner, LB_SETCURSEL, 3, 0, newSelection)
        || newSelection == LB_ERR) {
        failure = L"failed to select Cube in Outliner";
        return false;
    }

    if (!SameControlHandles(initialControls, DirectChildren(window, processId))
        || !ValidatedControlHasStyle(outliner, window, processId,
            kOutlinerId, L"ListBox", static_cast<LONG_PTR>(LBS_NOTIFY))
        || !WindowOwnedByProcess(window, processId)
        || !IsWindowVisible(window)
        || !IsWindowEnabled(window)) {
        failure = L"Outliner/editor identity, enabled state, or notification style changed before selection notification";
        return false;
    }
    LRESULT commandResult = 0;
    if (!SendMessageBounded(window, WM_COMMAND,
            MAKEWPARAM(kOutlinerId, LBN_SELCHANGE),
            reinterpret_cast<LPARAM>(outliner), commandResult)) {
        failure = L"bounded Outliner selection notification failed";
        return false;
    }

    return ValidateShellState(
        window, processId, initialControls, statics, kCubeInspectorText, 3, failure);
}

bool CloseEditor(HWND window, DWORD processId, HANDLE process, DWORD& exitCode) {
    if (!WindowOwnedByProcess(window, processId)) return false;
    if (!PostMessageW(window, WM_CLOSE, 0, 0)) return false;
    if (WaitForSingleObject(process, kProcessExitTimeoutMs) != WAIT_OBJECT_0) return false;
    return GetExitCodeProcess(process, &exitCode) != FALSE;
}

void AppendCleanupFailure(std::wstring& failure, const wchar_t* detail) {
    if (!failure.empty()) failure += L"; ";
    failure += detail;
}

bool CleanupProcess(HANDLE process, DWORD& exitCode, std::wstring& failure) {
    const DWORD initialState = WaitForSingleObject(process, 0);
    if (initialState == WAIT_OBJECT_0) {
        if (!GetExitCodeProcess(process, &exitCode) || exitCode == STILL_ACTIVE) {
            AppendCleanupFailure(failure,
                L"owned-process cleanup observed a signaled process but could not verify a terminal exit code");
            return false;
        }
        return true;
    }
    if (initialState == WAIT_FAILED) {
        AppendCleanupFailure(failure, L"owned-process cleanup precheck wait failed");
    } else if (initialState != WAIT_TIMEOUT) {
        AppendCleanupFailure(failure, L"owned-process cleanup precheck returned an unexpected wait state");
    }

    if (!TerminateProcess(process, 2)) {
        AppendCleanupFailure(failure, L"TerminateProcess failed for the owned editor process");
        return false;
    }

    const DWORD terminatedState = WaitForSingleObject(process, kCleanupTimeoutMs);
    if (terminatedState != WAIT_OBJECT_0) {
        AppendCleanupFailure(failure,
            terminatedState == WAIT_TIMEOUT
                ? L"owned editor process did not signal termination before the cleanup deadline"
                : L"owned editor process cleanup wait failed");
        return false;
    }
    if (!GetExitCodeProcess(process, &exitCode) || exitCode == STILL_ACTIVE) {
        AppendCleanupFailure(failure,
            L"owned editor process signaled after TerminateProcess but terminal exit code verification failed");
        return false;
    }
    return true;
}
} // namespace

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
    std::vector<ChildControl> initialControls;
    ShellStaticHandles statics{};

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
    } else if (!CaptureInitialControlInventory(
                   window, process.dwProcessId, initialControls, failure)) {
        // failure set by capture helper.
    } else if (!CaptureInitialStaticHandles(
                   initialControls, window, process.dwProcessId, statics, failure)) {
        // failure set by semantic-control capture helper.
    } else if (!ValidateShellState(window, process.dwProcessId, initialControls, statics,
                   kSceneRootInspectorText, 0, failure)) {
        // failure set by validator.
    } else if (!SelectCubeAndNotify(
                   window, process.dwProcessId, initialControls, statics, failure)) {
        // failure set by selector/validator.
    } else if (!ResizeAndCheck(window, process.dwProcessId, initialControls, statics,
                   800, 600, kCubeInspectorText, 3, failure)) {
        // failure set by resize validator.
    } else if (!ResizeAndCheck(window, process.dwProcessId, initialControls, statics,
                   420, 260, kCubeInspectorText, 3, failure)) {
        // failure set by resize validator.
    } else {
        int finalVisibleTopLevelCount = 0;
        bool finalTopLevelEnumerationFailed = false;
        if (!RevalidateStableSingleWindow(process.dwProcessId, window,
                finalVisibleTopLevelCount, finalTopLevelEnumerationFailed)) {
            if (finalTopLevelEnumerationFailed) {
                failure = L"EnumWindows failed during final editor-window revalidation";
            } else {
                failure = L"editor did not remain one stable visible process-owned top-level window; observed "
                    + std::to_wstring(finalVisibleTopLevelCount);
            }
        } else if (!ValidateShellState(window, process.dwProcessId, initialControls, statics,
                       kCubeInspectorText, 3, failure)) {
            // failure set by validator.
        } else if (CloseEditor(window, process.dwProcessId, process.hProcess, exitCode)
            && exitCode == 0) {
            passed = true;
        } else {
            failure = L"editor did not close cleanly with exit code 0";
        }
    }

    if (!passed) {
        CleanupProcess(process.hProcess, exitCode, failure);
    }
    if (!passed && GetExitCodeProcess(process.hProcess, &exitCode) == FALSE) exitCode = 1;

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);

    if (!passed) {
        std::wcerr << L"EDITOR AUTOMATED NATIVE RUNTIME SMOKE: FAIL\nReason: " << failure
            << L"\nExit: " << exitCode << L"\n";
        return 1;
    }

    std::wcout << L"EDITOR AUTOMATED NATIVE RUNTIME SMOKE: PASS\n"
        << L"Observed one stable visible, enabled, process-owned top-level editor window before and after "
        << L"interaction; the original 12 process-owned child HWND identities, bound semantic Static HWNDs, "
        << L"disabled pending tools, enabled Outliner/assets surfaces, required Outliner LBS_NOTIFY style, "
        << L"exact row identities, and Inspector state were revalidated around every bounded cross-process "
        << L"read and after both normal+narrow resizes; Cube selection stayed synchronized, all direct children "
        << L"remained contained, and shutdown exited cleanly.\n";
    return 0;
}
