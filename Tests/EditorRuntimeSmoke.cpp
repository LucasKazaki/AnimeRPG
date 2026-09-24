#include <windows.h>

#include <algorithm>
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
constexpr DWORD kSelectionTimeoutMs = 3000;
constexpr DWORD kShowStateTimeoutMs = 3000;
constexpr ULONGLONG kWorkBudgetMs = 135000;
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

ULONGLONG gWorkDeadlineTick = 0;
ULONGLONG gMessageDeadlineTick = 0;
HANDLE gOwnedProcessHandle = nullptr;

DWORD RemainingWorkBudget(DWORD requestedMs) {
    if (gWorkDeadlineTick == 0) return requestedMs;
    const ULONGLONG now = GetTickCount64();
    if (now >= gWorkDeadlineTick) return 0;
    const ULONGLONG remaining = gWorkDeadlineTick - now;
    return remaining < requestedMs ? static_cast<DWORD>(remaining) : requestedMs;
}

DWORD RemainingMessageBudget(DWORD requestedMs) {
    const DWORD workBudget = RemainingWorkBudget(requestedMs);
    if (workBudget == 0 || gMessageDeadlineTick == 0) return workBudget;
    const ULONGLONG now = GetTickCount64();
    if (now >= gMessageDeadlineTick) return 0;
    const ULONGLONG remaining = gMessageDeadlineTick - now;
    return remaining < workBudget ? static_cast<DWORD>(remaining) : workBudget;
}

DWORD RemainingDeadlineBudget(ULONGLONG deadline, DWORD requestedMs) {
    const DWORD workBudget = RemainingWorkBudget(requestedMs);
    if (workBudget == 0) return 0;
    const ULONGLONG now = GetTickCount64();
    if (now >= deadline) return 0;
    const ULONGLONG remaining = deadline - now;
    return remaining < workBudget ? static_cast<DWORD>(remaining) : workBudget;
}

class ScopedMessageDeadline {
public:
    explicit ScopedMessageDeadline(ULONGLONG deadline)
        : previousDeadline_(gMessageDeadlineTick) {
        gMessageDeadlineTick = deadline;
    }

    ~ScopedMessageDeadline() {
        gMessageDeadlineTick = previousDeadline_;
    }

    ScopedMessageDeadline(const ScopedMessageDeadline&) = delete;
    ScopedMessageDeadline& operator=(const ScopedMessageDeadline&) = delete;

private:
    ULONGLONG previousDeadline_{};
};

bool WorkBudgetExpired() {
    return gWorkDeadlineTick != 0 && GetTickCount64() >= gWorkDeadlineTick;
}

bool OwnedProcessStillRunning() {
    if (!gOwnedProcessHandle) return false;
    return WaitForSingleObject(gOwnedProcessHandle, 0) == WAIT_TIMEOUT;
}

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

struct ShellButtonHandles {
    std::array<HWND, 5> tools{};
};

struct ButtonGeometry {
    HWND handle{};
    LONG left{};
    LONG right{};
};

bool WindowOwnedByProcess(HWND window, DWORD processId) {
    if (!window || !OwnedProcessStillRunning()) return false;
    DWORD ownerProcessId = 0;
    if (GetWindowThreadProcessId(window, &ownerProcessId) == 0) return false;
    return ownerProcessId == processId && OwnedProcessStillRunning();
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
    const DWORD timeoutMs = RemainingMessageBudget(kMessageTimeoutMs);
    if (timeoutMs == 0) return false;
    DWORD_PTR rawResult = 0;
    const LRESULT sent = SendMessageTimeoutW(window, message, wParam, lParam,
        SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, timeoutMs, &rawResult);
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
    if (WindowOwnedByProcess(window, collection.processId) && IsWindowVisible(window)) {
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
        if (WorkBudgetExpired()) return nullptr;
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
        const DWORD sleepMs = RemainingWorkBudget(kWindowPollIntervalMs);
        if (sleepMs == 0) return nullptr;
        Sleep(sleepMs);
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

bool CaptureInitialButtonHandles(const std::vector<ChildControl>& controls, HWND parent,
    DWORD processId, ShellButtonHandles& buttons, std::wstring& failure) {
    std::vector<ButtonGeometry> ordered;
    ordered.reserve(kPendingButtons.size());
    for (const auto& control : controls) {
        if (control.className != L"Button") continue;
        if (!DirectVisibleChildOwnedByProcessAndParent(
                control.handle, parent, processId, L"Button")
            || IsWindowEnabled(control.handle)) {
            failure = L"initial toolbar button is hidden, replaced, or unexpectedly enabled";
            return false;
        }
        RECT rect{};
        if (!GetWindowRect(control.handle, &rect)) {
            failure = L"GetWindowRect failed while binding initial toolbar buttons";
            return false;
        }
        POINT points[2]{{rect.left, rect.top}, {rect.right, rect.bottom}};
        MapWindowPoints(HWND_DESKTOP, parent, points, 2);
        ordered.push_back({control.handle, points[0].x, points[1].x});
    }
    if (ordered.size() != kPendingButtons.size()) {
        failure = L"expected five initial toolbar buttons, observed "
            + std::to_wstring(ordered.size());
        return false;
    }
    std::sort(ordered.begin(), ordered.end(), [](const ButtonGeometry& a, const ButtonGeometry& b) {
        return a.left < b.left;
    });
    for (std::size_t index = 0; index < ordered.size(); ++index) {
        if (ordered[index].right <= ordered[index].left
            || (index != 0 && ordered[index - 1].right > ordered[index].left)) {
            failure = L"initial toolbar button slots are empty, overlapping, or not left-to-right";
            return false;
        }
        std::wstring caption;
        if (!ReadValidatedChildText(
                ordered[index].handle, parent, processId, L"Button", caption)
            || caption != kPendingButtons[index]) {
            failure = L"initial toolbar semantic slot mismatch at index "
                + std::to_wstring(index);
            return false;
        }
        buttons.tools[index] = ordered[index].handle;
    }
    return true;
}

bool ValidateBoundButtonSemantics(const ShellButtonHandles& buttons, HWND parent,
    DWORD processId, std::wstring& failure) {
    LONG previousRight = 0;
    for (std::size_t index = 0; index < buttons.tools.size(); ++index) {
        const HWND button = buttons.tools[index];
        if (!DirectVisibleChildOwnedByProcessAndParent(button, parent, processId, L"Button")
            || IsWindowEnabled(button)) {
            failure = L"bound toolbar tool is hidden, replaced, or unexpectedly enabled: "
                + std::wstring(kPendingButtons[index]);
            return false;
        }
        std::wstring caption;
        if (!ReadValidatedChildText(button, parent, processId, L"Button", caption)
            || caption != kPendingButtons[index]) {
            failure = L"bound toolbar tool changed semantic caption: "
                + std::wstring(kPendingButtons[index]);
            return false;
        }
        RECT rect{};
        if (!GetWindowRect(button, &rect)) {
            failure = L"GetWindowRect failed for bound toolbar tool: "
                + std::wstring(kPendingButtons[index]);
            return false;
        }
        POINT points[2]{{rect.left, rect.top}, {rect.right, rect.bottom}};
        MapWindowPoints(HWND_DESKTOP, parent, points, 2);
        if (points[1].x <= points[0].x
            || (index != 0 && previousRight > points[0].x)) {
            failure = L"bound toolbar semantic order/geometry changed: "
                + std::wstring(kPendingButtons[index]);
            return false;
        }
        previousRight = points[1].x;
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
    const ShellButtonHandles& buttons, const wchar_t* expectedInspectorText,
    int expectedSelection, std::wstring& failure) {
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
    if (!ValidateBoundButtonSemantics(buttons, window, processId, failure)) return false;

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
            || points[1].x <= points[0].x || points[1].y <= points[0].y) {
            failure = L"child outside client or empty: " + control.className
                + L" child=" + std::to_wstring(points[0].x) + L"," + std::to_wstring(points[0].y)
                + L".." + std::to_wstring(points[1].x) + L"," + std::to_wstring(points[1].y)
                + L" client=" + std::to_wstring(client.right) + L"x"
                + std::to_wstring(client.bottom);
            return false;
        }
    }
    return true;
}

bool PostResizeWhileOwnerPinned(HWND window, DWORD processId, DWORD threadId, HANDLE thread,
    HANDLE process, int width, int height, std::wstring& failure) {
    if (!WindowOwnedByProcess(window, processId) || !IsWindowVisible(window)
        || !IsWindowEnabled(window) || WaitForSingleObject(thread, 0) != WAIT_TIMEOUT
        || WaitForSingleObject(process, 0) != WAIT_TIMEOUT) {
        failure = L"editor ownership/liveness changed before resize owner pin";
        return false;
    }

    DWORD ownerProcessId = 0;
    const DWORD ownerThreadId = GetWindowThreadProcessId(window, &ownerProcessId);
    if (ownerThreadId == 0 || ownerProcessId != processId || ownerThreadId != threadId) {
        failure = L"editor HWND is not owned by the launched primary thread before resize";
        return false;
    }

    const DWORD previousSuspendCount = SuspendThread(thread);
    if (previousSuspendCount == static_cast<DWORD>(-1)) {
        failure = L"SuspendThread failed before resize post";
        return false;
    }

    bool suspendedContextCaptured = false;
    bool postedResize = false;
    if (previousSuspendCount == 0) {
        CONTEXT context{};
        context.ContextFlags = CONTEXT_CONTROL;
        suspendedContextCaptured = GetThreadContext(thread, &context) != FALSE;
    }
    if (previousSuspendCount == 0 && suspendedContextCaptured
        && WaitForSingleObject(process, 0) == WAIT_TIMEOUT
        && WaitForSingleObject(thread, 0) == WAIT_TIMEOUT) {
        DWORD pinnedProcessId = 0;
        const DWORD pinnedThreadId = GetWindowThreadProcessId(window, &pinnedProcessId);
        if (pinnedThreadId == threadId && pinnedProcessId == processId
            && IsWindowVisible(window) && IsWindowEnabled(window)) {
            postedResize = SetWindowPos(window, nullptr, 0, 0, width, height,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS) != FALSE;
        }
    }

    const DWORD resumePreviousCount = ResumeThread(thread);
    if (resumePreviousCount == static_cast<DWORD>(-1)
        || previousSuspendCount != 0 || !suspendedContextCaptured
        || resumePreviousCount != 1 || !postedResize) {
        failure = L"failed to pin, post, and resume the editor owner thread for resize";
        return false;
    }
    return true;
}

bool PostShowStateWhileOwnerPinned(HWND window, DWORD processId, DWORD threadId, HANDLE thread,
    HANDLE process, int showCommand, std::wstring& failure) {
    if (!WindowOwnedByProcess(window, processId) || !IsWindowVisible(window)
        || !IsWindowEnabled(window) || WaitForSingleObject(thread, 0) != WAIT_TIMEOUT
        || WaitForSingleObject(process, 0) != WAIT_TIMEOUT) {
        failure = L"editor ownership/liveness changed before show-state owner pin";
        return false;
    }

    DWORD ownerProcessId = 0;
    const DWORD ownerThreadId = GetWindowThreadProcessId(window, &ownerProcessId);
    if (ownerThreadId == 0 || ownerProcessId != processId || ownerThreadId != threadId) {
        failure = L"editor HWND is not owned by the launched primary thread before show-state request";
        return false;
    }

    const DWORD previousSuspendCount = SuspendThread(thread);
    if (previousSuspendCount == static_cast<DWORD>(-1)) {
        failure = L"SuspendThread failed before show-state post";
        return false;
    }

    bool suspendedContextCaptured = false;
    bool postedShowState = false;
    if (previousSuspendCount == 0) {
        CONTEXT context{};
        context.ContextFlags = CONTEXT_CONTROL;
        suspendedContextCaptured = GetThreadContext(thread, &context) != FALSE;
    }
    if (previousSuspendCount == 0 && suspendedContextCaptured
        && WaitForSingleObject(process, 0) == WAIT_TIMEOUT
        && WaitForSingleObject(thread, 0) == WAIT_TIMEOUT) {
        DWORD pinnedProcessId = 0;
        const DWORD pinnedThreadId = GetWindowThreadProcessId(window, &pinnedProcessId);
        if (pinnedThreadId == threadId && pinnedProcessId == processId
            && IsWindowVisible(window) && IsWindowEnabled(window)) {
            postedShowState = ShowWindowAsync(window, showCommand) != FALSE;
        }
    }

    const DWORD resumePreviousCount = ResumeThread(thread);
    if (resumePreviousCount == static_cast<DWORD>(-1)
        || previousSuspendCount != 0 || !suspendedContextCaptured
        || resumePreviousCount != 1 || !postedShowState) {
        failure = L"failed to pin, post, and resume the editor owner thread for show-state request";
        return false;
    }
    return true;
}

bool PostSelectionMutationWhileOwnerPinned(HWND window, HWND outliner, DWORD processId,
    DWORD threadId, HANDLE thread, HANDLE process, ULONGLONG deadline, bool notifySelection,
    std::wstring& failure) {
    if (GetTickCount64() >= deadline) {
        failure = L"selection deadline exhausted before selection owner pin";
        return false;
    }
    if (!WindowOwnedByProcess(window, processId)
        || !ValidatedControlHasStyle(outliner, window, processId,
            kOutlinerId, L"ListBox", static_cast<LONG_PTR>(LBS_NOTIFY))
        || WaitForSingleObject(thread, 0) != WAIT_TIMEOUT
        || WaitForSingleObject(process, 0) != WAIT_TIMEOUT) {
        failure = L"editor/Outliner ownership or liveness changed before selection owner pin";
        return false;
    }

    DWORD ownerProcessId = 0;
    const DWORD ownerThreadId = GetWindowThreadProcessId(window, &ownerProcessId);
    DWORD outlinerProcessId = 0;
    const DWORD outlinerThreadId = GetWindowThreadProcessId(outliner, &outlinerProcessId);
    if (ownerThreadId == 0 || ownerProcessId != processId || ownerThreadId != threadId
        || outlinerThreadId == 0 || outlinerProcessId != processId
        || outlinerThreadId != threadId) {
        failure = L"editor or Outliner HWND is not owned by the launched primary thread before selection post";
        return false;
    }

    const DWORD previousSuspendCount = SuspendThread(thread);
    if (previousSuspendCount == static_cast<DWORD>(-1)) {
        failure = L"SuspendThread failed before selection post";
        return false;
    }

    bool suspendedContextCaptured = false;
    bool postedSelectionMutation = false;
    bool deadlineExpiredBeforePost = false;
    if (previousSuspendCount == 0) {
        CONTEXT context{};
        context.ContextFlags = CONTEXT_CONTROL;
        suspendedContextCaptured = GetThreadContext(thread, &context) != FALSE;
    }
    if (previousSuspendCount == 0 && suspendedContextCaptured
        && WaitForSingleObject(process, 0) == WAIT_TIMEOUT
        && WaitForSingleObject(thread, 0) == WAIT_TIMEOUT) {
        DWORD pinnedProcessId = 0;
        const DWORD pinnedThreadId = GetWindowThreadProcessId(window, &pinnedProcessId);
        DWORD pinnedOutlinerProcessId = 0;
        const DWORD pinnedOutlinerThreadId =
            GetWindowThreadProcessId(outliner, &pinnedOutlinerProcessId);
        if (pinnedThreadId == threadId && pinnedProcessId == processId
            && pinnedOutlinerThreadId == threadId && pinnedOutlinerProcessId == processId
            && IsWindowVisible(window) && IsWindowEnabled(window)
            && ValidatedControlHasStyle(outliner, window, processId,
                kOutlinerId, L"ListBox", static_cast<LONG_PTR>(LBS_NOTIFY))) {
            if (GetTickCount64() >= deadline) {
                deadlineExpiredBeforePost = true;
            } else {
                postedSelectionMutation = notifySelection
                    ? PostMessageW(window, WM_COMMAND,
                        MAKEWPARAM(kOutlinerId, LBN_SELCHANGE),
                        reinterpret_cast<LPARAM>(outliner)) != FALSE
                    : PostMessageW(outliner, LB_SETCURSEL, 3, 0) != FALSE;
            }
        }
    }

    const DWORD resumePreviousCount = ResumeThread(thread);
    if (resumePreviousCount == static_cast<DWORD>(-1)
        || previousSuspendCount != 0 || !suspendedContextCaptured
        || resumePreviousCount != 1) {
        failure = L"failed to pin and resume the editor owner thread for selection mutation";
        return false;
    }
    if (deadlineExpiredBeforePost) {
        failure = L"selection deadline exhausted immediately before asynchronous selection post";
        return false;
    }
    if (!postedSelectionMutation) {
        failure = L"failed to post selection mutation while the editor owner thread was pinned";
        return false;
    }
    return true;
}

bool ResizeAndCheck(HWND window, DWORD processId, DWORD threadId, HANDLE thread, HANDLE process,
    const std::vector<ChildControl>& initialControls, const ShellStaticHandles& statics,
    const ShellButtonHandles& buttons, int width, int height,
    const wchar_t* expectedInspectorText, int expectedSelection, std::wstring& failure) {
    if (WorkBudgetExpired()) {
        failure = L"internal runtime work budget exhausted before resize";
        return false;
    }
    if (!WindowOwnedByProcess(window, processId) || !IsWindowVisible(window)
        || !IsWindowEnabled(window)) {
        failure = L"editor HWND ownership, visibility, or enabled state changed before resize";
        return false;
    }
    if (!SameControlHandles(initialControls, DirectChildren(window, processId))) {
        failure = L"direct child HWND inventory changed before resize";
        return false;
    }

    const ULONGLONG deadline = GetTickCount64() + kResizeTimeoutMs;
    if (!PostResizeWhileOwnerPinned(
            window, processId, threadId, thread, process, width, height, failure)) {
        return false;
    }

    ScopedMessageDeadline phaseDeadline(deadline);
    while (true) {
        if (WorkBudgetExpired()) {
            failure = L"internal runtime work budget exhausted while waiting for asynchronous resize";
            return false;
        }
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
            if (GetTickCount64() >= deadline) {
                failure = L"asynchronous resize reached requested geometry after the resize deadline";
                return false;
            }
            if (!DirectChildrenContained(window, processId, initialControls, failure)) return false;
            if (GetTickCount64() >= deadline) {
                failure = L"resized editor containment validation finished after the resize deadline";
                return false;
            }
            if (!ValidateShellState(window, processId, initialControls, statics, buttons,
                    expectedInspectorText, expectedSelection, failure)) {
                return false;
            }
            if (GetTickCount64() >= deadline) {
                failure = L"resized editor shell validation finished after the resize deadline";
                return false;
            }
            return true;
        }
        if (GetTickCount64() >= deadline) {
            failure = L"asynchronous resize did not complete within deadline";
            return false;
        }
        const DWORD sleepMs = RemainingDeadlineBudget(deadline, kResizePollIntervalMs);
        if (sleepMs == 0) {
            failure = L"resize deadline exhausted while waiting for asynchronous resize";
            return false;
        }
        Sleep(sleepMs);
    }
}

bool MaximizeRestoreAndCheck(HWND window, DWORD processId, DWORD threadId, HANDLE thread,
    HANDLE process, const std::vector<ChildControl>& initialControls,
    const ShellStaticHandles& statics, const ShellButtonHandles& buttons,
    const wchar_t* expectedInspectorText, int expectedSelection, std::wstring& failure) {
    if (WorkBudgetExpired()) {
        failure = L"internal runtime work budget exhausted before maximize";
        return false;
    }
    if (!WindowOwnedByProcess(window, processId) || !IsWindowVisible(window)
        || !IsWindowEnabled(window)) {
        failure = L"editor HWND ownership, visibility, or enabled state changed before maximize";
        return false;
    }
    if (IsZoomed(window)) {
        failure = L"editor unexpectedly already maximized before maximize verification";
        return false;
    }
    RECT normalRect{};
    if (!GetWindowRect(window, &normalRect)) {
        failure = L"GetWindowRect failed before maximize verification";
        return false;
    }
    const int normalWidth = normalRect.right - normalRect.left;
    const int normalHeight = normalRect.bottom - normalRect.top;
    if (normalWidth <= 0 || normalHeight <= 0) {
        failure = L"editor normal window rectangle is empty before maximize verification";
        return false;
    }

    const ULONGLONG maximizeDeadline = GetTickCount64() + kShowStateTimeoutMs;
    if (!PostShowStateWhileOwnerPinned(
            window, processId, threadId, thread, process, SW_MAXIMIZE, failure)) {
        return false;
    }

    std::wstring lastMaximizeFailure;
    {
        ScopedMessageDeadline phaseDeadline(maximizeDeadline);
        while (true) {
            if (WorkBudgetExpired()) {
                failure = L"internal runtime work budget exhausted while waiting for maximized state";
                return false;
            }
            if (!WindowOwnedByProcess(window, processId) || !IsWindowVisible(window)
                || !IsWindowEnabled(window)) {
                failure = L"editor HWND ownership, visibility, or enabled state changed while waiting for maximized state";
                return false;
            }
            if (IsZoomed(window)) {
                std::wstring stateFailure;
                if (DirectChildrenContained(window, processId, initialControls, stateFailure)
                    && ValidateShellState(window, processId, initialControls, statics, buttons,
                        expectedInspectorText, expectedSelection, stateFailure)) {
                    if (GetTickCount64() >= maximizeDeadline) {
                        failure = L"maximized editor shell validation finished after the show-state deadline";
                        return false;
                    }
                    break;
                }
                lastMaximizeFailure = std::move(stateFailure);
            }
            if (GetTickCount64() >= maximizeDeadline) {
                failure = IsZoomed(window)
                    ? L"maximized editor shell did not settle before deadline: " + lastMaximizeFailure
                    : L"editor did not enter maximized state before deadline";
                return false;
            }
            const DWORD sleepMs = RemainingDeadlineBudget(
                maximizeDeadline, kResizePollIntervalMs);
            if (sleepMs == 0) {
                failure = L"show-state deadline exhausted while waiting for maximized state";
                return false;
            }
            Sleep(sleepMs);
        }
    }

    const ULONGLONG restoreDeadline = GetTickCount64() + kShowStateTimeoutMs;
    if (!PostShowStateWhileOwnerPinned(
            window, processId, threadId, thread, process, SW_RESTORE, failure)) {
        return false;
    }

    std::wstring lastRestoreFailure;
    ScopedMessageDeadline phaseDeadline(restoreDeadline);
    while (true) {
        if (WorkBudgetExpired()) {
            failure = L"internal runtime work budget exhausted while waiting for restored state";
            return false;
        }
        if (!WindowOwnedByProcess(window, processId) || !IsWindowVisible(window)
            || !IsWindowEnabled(window)) {
            failure = L"editor HWND ownership, visibility, or enabled state changed while waiting for restored state";
            return false;
        }
        RECT restoredRect{};
        if (!GetWindowRect(window, &restoredRect)) {
            failure = L"GetWindowRect failed while waiting for restored state";
            return false;
        }
        const bool restoredGeometry = restoredRect.left == normalRect.left
            && restoredRect.top == normalRect.top
            && restoredRect.right == normalRect.right
            && restoredRect.bottom == normalRect.bottom;
        if (!IsZoomed(window) && restoredGeometry) {
            std::wstring stateFailure;
            if (DirectChildrenContained(window, processId, initialControls, stateFailure)
                && ValidateShellState(window, processId, initialControls, statics, buttons,
                    expectedInspectorText, expectedSelection, stateFailure)) {
                if (GetTickCount64() >= restoreDeadline) {
                    failure = L"restored editor shell validation finished after the show-state deadline";
                    return false;
                }
                return true;
            }
            lastRestoreFailure = std::move(stateFailure);
        }
        if (GetTickCount64() >= restoreDeadline) {
            if (IsZoomed(window)) {
                failure = L"editor did not leave maximized state before restore deadline";
            } else if (!restoredGeometry) {
                failure = L"editor did not restore its pre-maximize outer rectangle before deadline";
            } else {
                failure = L"restored editor shell did not settle before deadline: " + lastRestoreFailure;
            }
            return false;
        }
        const DWORD sleepMs = RemainingDeadlineBudget(restoreDeadline, kResizePollIntervalMs);
        if (sleepMs == 0) {
            failure = L"show-state deadline exhausted while waiting for restored state";
            return false;
        }
        Sleep(sleepMs);
    }
}

bool SelectCubeAndNotify(HWND window, DWORD processId, DWORD threadId, HANDLE thread,
    HANDLE process, const std::vector<ChildControl>& initialControls,
    const ShellStaticHandles& statics, const ShellButtonHandles& buttons,
    std::wstring& failure) {
    if (WorkBudgetExpired()) {
        failure = L"internal runtime work budget exhausted before selection";
        return false;
    }
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

    const ULONGLONG deadline = GetTickCount64() + kSelectionTimeoutMs;
    ScopedMessageDeadline phaseDeadline(deadline);
    if (!PostSelectionMutationWhileOwnerPinned(
            window, outliner, processId, threadId, thread, process, deadline, false, failure)) {
        return false;
    }

    while (true) {
        if (WorkBudgetExpired()) {
            failure = L"internal runtime work budget exhausted while waiting for Cube selection";
            return false;
        }
        if (!ValidatedControlHasStyle(outliner, window, processId,
                kOutlinerId, L"ListBox", static_cast<LONG_PTR>(LBS_NOTIFY))) {
            failure = L"Outliner identity/style changed while waiting for Cube selection";
            return false;
        }
        LRESULT selection = LB_ERR;
        if (!ReadValidatedListboxValue(outliner, window, processId, kOutlinerId,
                LB_GETCURSEL, 0, selection)) {
            failure = L"failed to read Outliner selection after asynchronous Cube selection post";
            return false;
        }
        if (selection == 3) {
            if (GetTickCount64() >= deadline) {
                failure = L"Cube selection was first observed at or after the selection deadline";
                return false;
            }
            break;
        }
        if (GetTickCount64() >= deadline) {
            failure = L"Cube selection did not settle before the selection deadline";
            return false;
        }
        const DWORD sleepMs = RemainingDeadlineBudget(deadline, kResizePollIntervalMs);
        if (sleepMs == 0) {
            failure = L"selection deadline exhausted while waiting for Cube selection";
            return false;
        }
        Sleep(sleepMs);
    }

    if (!PostSelectionMutationWhileOwnerPinned(
            window, outliner, processId, threadId, thread, process, deadline, true, failure)) {
        return false;
    }

    std::wstring lastStateFailure;
    while (true) {
        std::wstring stateFailure;
        if (ValidateShellState(window, processId, initialControls, statics, buttons,
                kCubeInspectorText, 3, stateFailure)) {
            if (GetTickCount64() >= deadline) {
                failure = L"Cube selection/Inspector validation finished after the selection deadline";
                return false;
            }
            return true;
        }
        lastStateFailure = std::move(stateFailure);
        if (GetTickCount64() >= deadline) {
            failure = L"Cube selection notification did not settle the shell before deadline: "
                + lastStateFailure;
            return false;
        }
        const DWORD sleepMs = RemainingDeadlineBudget(deadline, kResizePollIntervalMs);
        if (sleepMs == 0) {
            failure = L"selection deadline exhausted while waiting for Cube Inspector synchronization";
            return false;
        }
        Sleep(sleepMs);
    }
}

bool CloseEditor(HWND window, DWORD processId, DWORD threadId, HANDLE thread,
    HANDLE process, DWORD& exitCode) {
    if (WorkBudgetExpired()) return false;
    if (!WindowOwnedByProcess(window, processId)
        || WaitForSingleObject(thread, 0) != WAIT_TIMEOUT) {
        return false;
    }

    DWORD ownerProcessId = 0;
    const DWORD ownerThreadId = GetWindowThreadProcessId(window, &ownerProcessId);
    if (ownerThreadId == 0 || ownerProcessId != processId || ownerThreadId != threadId) {
        return false;
    }

    const DWORD previousSuspendCount = SuspendThread(thread);
    if (previousSuspendCount == static_cast<DWORD>(-1)) return false;

    bool suspendedContextCaptured = false;
    bool postedClose = false;
    if (previousSuspendCount == 0) {
        CONTEXT context{};
        context.ContextFlags = CONTEXT_CONTROL;
        suspendedContextCaptured = GetThreadContext(thread, &context) != FALSE;
    }
    if (previousSuspendCount == 0 && suspendedContextCaptured
        && WaitForSingleObject(process, 0) == WAIT_TIMEOUT
        && WaitForSingleObject(thread, 0) == WAIT_TIMEOUT) {
        DWORD pinnedProcessId = 0;
        const DWORD pinnedThreadId = GetWindowThreadProcessId(window, &pinnedProcessId);
        if (pinnedThreadId == threadId && pinnedProcessId == processId) {
            postedClose = PostMessageW(window, WM_CLOSE, 0, 0) != FALSE;
        }
    }

    const DWORD resumePreviousCount = ResumeThread(thread);
    if (resumePreviousCount == static_cast<DWORD>(-1)
        || previousSuspendCount != 0 || !suspendedContextCaptured
        || resumePreviousCount != 1 || !postedClose) {
        return false;
    }

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

    gWorkDeadlineTick = GetTickCount64() + kWorkBudgetMs;
    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\"";
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, argv[2],
            &startup, &process)) {
        std::cerr << "EDITOR AUTOMATED NATIVE RUNTIME SMOKE: FAIL (launch)\n";
        return 1;
    }
    gOwnedProcessHandle = process.hProcess;

    HWND window = nullptr;
    bool passed = false;
    std::wstring failure;
    DWORD exitCode = 1;
    std::vector<ChildControl> initialControls;
    ShellStaticHandles statics{};
    ShellButtonHandles buttons{};

    if (WorkBudgetExpired()) {
        failure = L"135-second internal work budget exhausted during editor launch";
    } else {
        const DWORD inputIdleTimeoutMs = RemainingWorkBudget(5000);
        if (inputIdleTimeoutMs == 0) {
            failure = L"internal runtime work budget exhausted before input-idle wait";
        } else {
            WaitForInputIdle(process.hProcess, inputIdleTimeoutMs);
        }
    }
    int visibleTopLevelCount = 0;
    bool topLevelEnumerationFailed = false;
    if (failure.empty()) {
        window = WaitForStableSingleWindow(
            process.dwProcessId, visibleTopLevelCount, topLevelEnumerationFailed);
    }
    if (!failure.empty()) {
        // failure set before top-level discovery.
    } else if (!window) {
        if (WorkBudgetExpired()) {
            failure = L"internal runtime work budget exhausted while locating the editor top-level window";
        } else if (topLevelEnumerationFailed) {
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
    } else if (!CaptureInitialButtonHandles(
                   initialControls, window, process.dwProcessId, buttons, failure)) {
        // failure set by semantic-toolbar capture helper.
    } else if (!ValidateShellState(window, process.dwProcessId, initialControls, statics,
                   buttons, kSceneRootInspectorText, 0, failure)) {
        // failure set by validator.
    } else if (!DirectChildrenContained(
                   window, process.dwProcessId, initialControls, failure)) {
        // failure set by startup containment validator before any resize can normalize layout.
    } else if (!SelectCubeAndNotify(window, process.dwProcessId, process.dwThreadId,
                   process.hThread, process.hProcess, initialControls, statics, buttons, failure)) {
        // failure set by selector/validator.
    } else if (!ResizeAndCheck(window, process.dwProcessId, process.dwThreadId,
                   process.hThread, process.hProcess, initialControls, statics, buttons,
                   800, 600, kCubeInspectorText, 3, failure)) {
        // failure set by resize validator.
    } else if (!ResizeAndCheck(window, process.dwProcessId, process.dwThreadId,
                   process.hThread, process.hProcess, initialControls, statics, buttons,
                   1280, 720, kCubeInspectorText, 3, failure)) {
        // failure set by resize validator.
    } else if (!ResizeAndCheck(window, process.dwProcessId, process.dwThreadId,
                   process.hThread, process.hProcess, initialControls, statics, buttons,
                   1440, 900, kCubeInspectorText, 3, failure)) {
        // failure set by resize validator.
    } else if (!MaximizeRestoreAndCheck(window, process.dwProcessId, process.dwThreadId,
                   process.hThread, process.hProcess, initialControls, statics, buttons,
                   kCubeInspectorText, 3, failure)) {
        // failure set by maximize/restore validator.
    } else if (!ResizeAndCheck(window, process.dwProcessId, process.dwThreadId,
                   process.hThread, process.hProcess, initialControls, statics, buttons,
                   420, 260, kCubeInspectorText, 3, failure)) {
        // failure set by resize validator.
    } else {
        int finalVisibleTopLevelCount = 0;
        bool finalTopLevelEnumerationFailed = false;
        if (!RevalidateStableSingleWindow(process.dwProcessId, window,
                finalVisibleTopLevelCount, finalTopLevelEnumerationFailed)) {
            if (WorkBudgetExpired()) {
                failure = L"internal runtime work budget exhausted during final editor-window revalidation";
            } else if (finalTopLevelEnumerationFailed) {
                failure = L"EnumWindows failed during final editor-window revalidation";
            } else {
                failure = L"editor did not remain one stable visible process-owned top-level window; observed "
                    + std::to_wstring(finalVisibleTopLevelCount);
            }
        } else if (!ValidateShellState(window, process.dwProcessId, initialControls, statics,
                       buttons, kCubeInspectorText, 3, failure)) {
            // failure set by validator.
        } else if (CloseEditor(window, process.dwProcessId, process.dwThreadId,
                       process.hThread, process.hProcess, exitCode)
            && exitCode == 0) {
            passed = true;
        } else {
            failure = WorkBudgetExpired()
                ? L"internal runtime work budget exhausted before clean editor shutdown"
                : L"editor did not close cleanly with exit code 0";
        }
    }

    if (!passed) {
        if (WorkBudgetExpired()) {
            AppendCleanupFailure(failure,
                L"135-second internal launch+work budget exhausted; cleanup margin reserved before CTest's 180-second timeout");
        }
        CleanupProcess(process.hProcess, exitCode, failure);
    }
    if (!passed && GetExitCodeProcess(process.hProcess, &exitCode) == FALSE) exitCode = 1;

    CloseHandle(process.hThread);
    gOwnedProcessHandle = nullptr;
    CloseHandle(process.hProcess);

    if (!passed) {
        std::wcerr << L"EDITOR AUTOMATED NATIVE RUNTIME SMOKE: FAIL\nReason: " << failure
            << L"\nExit: " << exitCode << L"\n";
        return 1;
    }

    std::wcout << L"EDITOR AUTOMATED NATIVE RUNTIME SMOKE: PASS\n"
        << L"Observed one stable visible, enabled, process-owned top-level editor window before and after "
        << L"interaction; the original 12 process-owned child HWND identities, bound semantic Static HWNDs and "
        << L"left-to-right semantic toolbar Button HWNDs, disabled pending tools, enabled Outliner/assets surfaces, "
        << L"required Outliner LBS_NOTIFY style, exact row identities, and Inspector state were revalidated around "
        << L"every bounded cross-process read and after 800x600, 1280x720, 1440x900, maximized+restored, and 420x260 states; "
        << L"Cube selection and its WM_COMMAND notification were admitted before the same three-second selection deadline and posted only while the exact launched editor/Outliner owner thread was suspended behind a valid thread-context barrier, then resumed before bounded polling confirmed selection and Inspector synchronization; "
        << L"each asynchronous resize post was issued while the exact launched window-owner thread was suspended behind a valid thread-context barrier, then resumed before polling, "
        << L"and resize containment/shell validation stayed inside its 1.5-second phase deadline; "
        << L"maximize/restore show-state posts were also issued while that exact owner thread was suspended behind a valid thread-context barrier, then resumed before show-state polling, with nested shell-message waits capped to each show-state deadline; "
        << L"Cube selection stayed synchronized, all direct children remained contained from startup through every size/show-state transition, "
        << L"the retained CreateProcess handle remained nonsignaled around PID-based HWND ownership checks, "
        << L"the original window-owning launch thread was suspended and a valid suspended thread context was captured "
        << L"before the final asynchronous WM_CLOSE enqueue, then the thread was resumed before any wait and shutdown "
        << L"exited cleanly.\n";
    return 0;
}
