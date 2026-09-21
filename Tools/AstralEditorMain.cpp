#include "Engine/Editor/EditorLayout.h"

#include <windows.h>

#include <array>
#include <cwchar>
#include <string>

namespace {
constexpr wchar_t kEditorWindowClass[] = L"AstralEditorWindow";
constexpr int kOutlinerId = 1001;
constexpr int kAssetListId = 1002;

HWND g_outlinerLabel = nullptr;
HWND g_outliner = nullptr;
HWND g_inspectorLabel = nullptr;
HWND g_inspector = nullptr;
HWND g_assetsLabel = nullptr;
HWND g_assets = nullptr;
HWND g_status = nullptr;
HWND g_selectButton = nullptr;
HWND g_moveButton = nullptr;
HWND g_rotateButton = nullptr;
HWND g_scaleButton = nullptr;
HWND g_playButton = nullptr;
Astral::Editor::EditorLayout g_layout{};

constexpr std::array<const wchar_t*, 5> kFixtureNames{
    L"Scene Root", L"Camera", L"Directional Light", L"Cube", L"Floor"};
constexpr std::array<const wchar_t*, 5> kFixtureDetails{
    L"Name: Scene Root\r\nType: Scene\r\n\r\nTransform: n/a\r\n\r\nE11.0 editor fixture",
    L"Name: Camera\r\nType: Camera\r\n\r\nPosition: 0, 4, -8\r\nRotation: 18, 0, 0\r\nScale: 1, 1, 1",
    L"Name: Directional Light\r\nType: Light placeholder\r\n\r\nRotation: 45, -30, 0\r\n\r\nLighting runtime is not implemented.",
    L"Name: Cube\r\nType: Primitive placeholder\r\n\r\nPosition: 0, 0.5, 0\r\nRotation: 0, 0, 0\r\nScale: 1, 1, 1",
    L"Name: Floor\r\nType: Primitive placeholder\r\n\r\nPosition: 0, 0, 0\r\nRotation: 0, 0, 0\r\nScale: 10, 1, 10"};

void MoveControl(HWND control, int x, int y, int width, int height) {
    if (!control) return;
    MoveWindow(control, x, y, width > 0 ? width : 0, height > 0 ? height : 0, TRUE);
}

void UpdateInspector() {
    int selection = static_cast<int>(SendMessageW(g_outliner, LB_GETCURSEL, 0, 0));
    if (selection < 0 || selection >= static_cast<int>(kFixtureDetails.size())) selection = 0;
    SetWindowTextW(g_inspector, kFixtureDetails[static_cast<std::size_t>(selection)]);
}

void ApplyLayout(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    g_layout = Astral::Editor::ComputeEditorLayout(client.right, client.bottom);

    const auto toolbar = Astral::Editor::ComputeEditorToolbarLayout(g_layout.toolbar);
    MoveControl(g_selectButton, toolbar.select.x, toolbar.select.y,
        toolbar.select.width, toolbar.select.height);
    MoveControl(g_moveButton, toolbar.move.x, toolbar.move.y,
        toolbar.move.width, toolbar.move.height);
    MoveControl(g_rotateButton, toolbar.rotate.x, toolbar.rotate.y,
        toolbar.rotate.width, toolbar.rotate.height);
    MoveControl(g_scaleButton, toolbar.scale.x, toolbar.scale.y,
        toolbar.scale.width, toolbar.scale.height);
    MoveControl(g_playButton, toolbar.play.x, toolbar.play.y,
        toolbar.play.width, toolbar.play.height);

    const auto outliner = Astral::Editor::ComputeEditorPanelContentLayout(g_layout.outliner);
    MoveControl(g_outlinerLabel, outliner.label.x, outliner.label.y,
        outliner.label.width, outliner.label.height);
    MoveControl(g_outliner, outliner.body.x, outliner.body.y,
        outliner.body.width, outliner.body.height);

    const auto inspector = Astral::Editor::ComputeEditorPanelContentLayout(g_layout.inspector);
    MoveControl(g_inspectorLabel, inspector.label.x, inspector.label.y,
        inspector.label.width, inspector.label.height);
    MoveControl(g_inspector, inspector.body.x, inspector.body.y,
        inspector.body.width, inspector.body.height);

    const auto assets = Astral::Editor::ComputeEditorPanelContentLayout(g_layout.assets);
    MoveControl(g_assetsLabel, assets.label.x, assets.label.y,
        assets.label.width, assets.label.height);
    MoveControl(g_assets, assets.body.x, assets.body.y,
        assets.body.width, assets.body.height);

    const auto status = Astral::Editor::ComputeEditorStatusContentLayout(g_layout.status);
    MoveControl(g_status, status.x, status.y, status.width, status.height);
    InvalidateRect(window, nullptr, TRUE);
}

void DrawViewport(HDC dc) {
    const auto& v = g_layout.viewport;
    RECT viewport{v.x, v.y, v.x + v.width, v.y + v.height};
    HBRUSH background = CreateSolidBrush(RGB(24, 27, 34));
    FillRect(dc, &viewport, background);
    DeleteObject(background);

    if (v.width <= 0 || v.height <= 0) return;

    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(48, 53, 64));
    HGDIOBJ priorPen = SelectObject(dc, gridPen);
    for (int x = v.x + 20; x < v.x + v.width; x += 20) {
        MoveToEx(dc, x, v.y, nullptr);
        LineTo(dc, x, v.y + v.height);
    }
    for (int y = v.y + 20; y < v.y + v.height; y += 20) {
        MoveToEx(dc, v.x, y, nullptr);
        LineTo(dc, v.x + v.width, y);
    }
    SelectObject(dc, priorPen);
    DeleteObject(gridPen);

    const int centerX = v.x + v.width / 2;
    const int centerY = v.y + v.height / 2;
    HPEN xPen = CreatePen(PS_SOLID, 2, RGB(220, 80, 80));
    priorPen = SelectObject(dc, xPen);
    MoveToEx(dc, v.x + 10, centerY, nullptr);
    LineTo(dc, v.x + v.width - 10, centerY);
    SelectObject(dc, priorPen);
    DeleteObject(xPen);

    HPEN yPen = CreatePen(PS_SOLID, 2, RGB(80, 200, 120));
    priorPen = SelectObject(dc, yPen);
    MoveToEx(dc, centerX, v.y + 10, nullptr);
    LineTo(dc, centerX, v.y + v.height - 10);
    SelectObject(dc, priorPen);
    DeleteObject(yPen);

    HPEN cubePen = CreatePen(PS_SOLID, 2, RGB(105, 165, 255));
    priorPen = SelectObject(dc, cubePen);
    const int half = (v.width < v.height ? v.width : v.height) / 8;
    Rectangle(dc, centerX - half, centerY - half, centerX + half, centerY + half);
    Rectangle(dc, centerX - half / 2, centerY - half - half / 2,
        centerX + half + half / 2, centerY + half - half / 2);
    MoveToEx(dc, centerX - half, centerY - half, nullptr);
    LineTo(dc, centerX - half / 2, centerY - half - half / 2);
    MoveToEx(dc, centerX + half, centerY - half, nullptr);
    LineTo(dc, centerX + half + half / 2, centerY - half - half / 2);
    MoveToEx(dc, centerX + half, centerY + half, nullptr);
    LineTo(dc, centerX + half + half / 2, centerY + half - half / 2);
    SelectObject(dc, priorPen);
    DeleteObject(cubePen);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(225, 230, 240));
    const wchar_t* title = L"Astral Editor Viewport | procedural fixture | GDI preview";
    TextOutW(dc, v.x + 12, v.y + 10, title, static_cast<int>(std::wcslen(title)));

    int selection = static_cast<int>(SendMessageW(g_outliner, LB_GETCURSEL, 0, 0));
    if (selection >= 0 && selection < static_cast<int>(kFixtureNames.size())) {
        std::wstring selected = L"Selected: ";
        selected += kFixtureNames[static_cast<std::size_t>(selection)];
        TextOutW(dc, v.x + 12, v.y + 30, selected.c_str(), static_cast<int>(selected.size()));
    }
}

HWND MakeControl(HWND parent, const wchar_t* className, const wchar_t* text,
    DWORD style, int id = 0) {
    return CreateWindowExW(0, className, text, WS_CHILD | WS_VISIBLE | style,
        0, 0, 10, 10, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
}

void ApplyToolAvailability() {
    using Astral::Editor::EditorTool;
    using Astral::Editor::IsEditorToolAvailable;
    EnableWindow(g_selectButton, IsEditorToolAvailable(EditorTool::Select) ? TRUE : FALSE);
    EnableWindow(g_moveButton, IsEditorToolAvailable(EditorTool::Move) ? TRUE : FALSE);
    EnableWindow(g_rotateButton, IsEditorToolAvailable(EditorTool::Rotate) ? TRUE : FALSE);
    EnableWindow(g_scaleButton, IsEditorToolAvailable(EditorTool::Scale) ? TRUE : FALSE);
    EnableWindow(g_playButton, IsEditorToolAvailable(EditorTool::Play) ? TRUE : FALSE);
}

LRESULT CALLBACK EditorWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        g_selectButton = MakeControl(window, L"BUTTON", L"Select (pending)", BS_PUSHBUTTON);
        g_moveButton = MakeControl(window, L"BUTTON", L"Move (pending)", BS_PUSHBUTTON);
        g_rotateButton = MakeControl(window, L"BUTTON", L"Rotate (pending)", BS_PUSHBUTTON);
        g_scaleButton = MakeControl(window, L"BUTTON", L"Scale (pending)", BS_PUSHBUTTON);
        g_playButton = MakeControl(window, L"BUTTON", L"Play (pending)", BS_PUSHBUTTON);
        ApplyToolAvailability();

        g_outlinerLabel = MakeControl(window, L"STATIC", L"OUTLINER", SS_LEFT);
        g_outliner = MakeControl(window, L"LISTBOX", L"",
            LBS_NOTIFY | WS_BORDER | WS_VSCROLL, kOutlinerId);
        for (const wchar_t* name : kFixtureNames) SendMessageW(g_outliner, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
        SendMessageW(g_outliner, LB_SETCURSEL, 0, 0);

        g_inspectorLabel = MakeControl(window, L"STATIC", L"INSPECTOR", SS_LEFT);
        g_inspector = MakeControl(window, L"STATIC", L"", SS_LEFT | WS_BORDER);

        g_assetsLabel = MakeControl(window, L"STATIC", L"ASSETS / DEFAULT PRIMITIVES", SS_LEFT);
        g_assets = MakeControl(window, L"LISTBOX", L"", WS_BORDER | WS_VSCROLL, kAssetListId);
        for (const wchar_t* asset : {L"Primitive/Cube", L"Primitive/Plane", L"Camera", L"DirectionalLight"}) {
            SendMessageW(g_assets, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(asset));
        }

        g_status = MakeControl(window, L"STATIC",
            L"E11.0 editor shell | Outliner selection works | viewport transform tools, undo/redo, save/reopen, Play and real asset import pending",
            SS_LEFT);
        UpdateInspector();
        ApplyLayout(window);
        return 0;
    }
    case WM_SIZE:
        ApplyLayout(window);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == kOutlinerId && HIWORD(wParam) == LBN_SELCHANGE) {
            UpdateInspector();
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        }
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        DrawViewport(dc);
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
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    WNDCLASSW windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = EditorWindowProc;
    windowClass.lpszClassName = kEditorWindowClass;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH));
    if (!RegisterClassW(&windowClass)) return 1;

    HWND window = CreateWindowExW(0, kEditorWindowClass, L"Astral Editor 0.1",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1440, 900,
        nullptr, nullptr, instance, nullptr);
    if (!window) return 2;

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
