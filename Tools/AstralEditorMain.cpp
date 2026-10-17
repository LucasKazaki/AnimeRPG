// Astral Editor: the Win32 front end of the scene editor (Engine/Editor/SceneEditor).
//
//   AstralEditor [content-root [project-or-scene]]
//
// Opens Content/Samples/Playground/project.json when started from the
// repository root with no arguments, otherwise an untitled scene.
//
// Layout (EditorLayout): toolbar, Outliner (authored hierarchy), viewport,
// Inspector (reflected property grid), Assets (things to create), status bar.
// Viewport: left click selects or drags a gizmo handle, right drag orbits,
// middle drag (or Shift + right drag) pans, the wheel zooms. Q/W/E/R pick the
// select/move/rotate/scale tool, F frames the selection, Ctrl snaps while
// dragging. F5 plays the current document in place; Esc or F5 stops.

#include "Engine/Editor/EditorLayout.h"
#include "Engine/Editor/SceneEditor.h"
#include "Game/Samples/Playground/PlaygroundBehaviours.h"

#include <windows.h>
#include <commdlg.h>
#include <windowsx.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace Astral;
using Editor::GizmoTool;
using Editor::InspectorGroupKind;
using Editor::SelectionKind;

constexpr wchar_t kEditorWindowClass[] = L"AstralEditorWindow";
constexpr wchar_t kViewportClass[] = L"AstralEditorViewport";
constexpr wchar_t kInspectorClass[] = L"AstralEditorInspector";
constexpr int kOutlinerId = 1001;
constexpr int kAssetListId = 1002;
constexpr int kToolButtonIds[5] = {1101, 1102, 1103, 1104, 1105};
constexpr int kFirstInspectorId = 2000;
constexpr int kMessageLoopErrorExitCode = 3;
constexpr UINT_PTR kDiskPollTimer = 1;

enum Command : int {
    kFileNew = 3001, kFileOpenProject, kFileOpenScene, kFileSave, kFileSaveAs, kFileExit,
    kEditUndo = 3101, kEditRedo, kEditDuplicate, kEditDelete, kEditSetParent, kEditFrame,
    kCreateEmpty = 3201, kCreateCube, kCreateSphere, kCreatePlane, kCreateCapsule, kCreateCylinder,
    kCreatePointLight, kCreateDirectionalLight, kCreateCamera,
    kViewGrid = 3301, kViewSnap, kViewHalfResolution,
    kPlayToggle = 3401,
    kHelpControls = 3501,
};

std::wstring Wide(const std::string& text) {
    if (text.empty()) return {};
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring out(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), length);
    return out;
}

std::string Utf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), length, nullptr, nullptr);
    return out;
}

std::string WindowText(HWND control) {
    const int length = GetWindowTextLengthW(control);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(control, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(length));
    return Utf8(text);
}

std::string FormatNumber(double value) {
    char text[64];
    if (std::fabs(value - std::round(value)) < 1.0e-9 && std::fabs(value) < 1.0e9) {
        std::snprintf(text, sizeof(text), "%.0f", value);
    } else {
        std::snprintf(text, sizeof(text), "%.4f", value);
        std::string trimmed = text;
        while (!trimmed.empty() && trimmed.back() == '0') trimmed.pop_back();
        if (!trimmed.empty() && trimmed.back() == '.') trimmed.pop_back();
        return trimmed == "-0" ? "0" : trimmed;
    }
    return std::string(text) == "-0" ? "0" : text;
}

bool ParseNumber(const std::string& text, double& out) {
    if (text.empty()) return false;
    char* end = nullptr;
    out = std::strtod(text.c_str(), &end);
    while (end && (*end == ' ' || *end == '\t')) ++end;
    return end && *end == '\0' && std::isfinite(out);
}

int ComponentCount(Core::FieldKind kind) {
    switch (kind) {
    case Core::FieldKind::Vec2: return 2;
    case Core::FieldKind::Vec3: return 3;
    case Core::FieldKind::Vec4:
    case Core::FieldKind::Quat: return 4;
    default: return 1;
    }
}

// One inspector row and the controls that edit it.
struct FieldBinding {
    InspectorGroupKind group{};
    std::string groupName;
    std::size_t index{};
    Editor::InspectorField field;
    std::vector<HWND> controls;
};

enum class ButtonAction { RemoveGroup, ResetField, AddMaterial };

struct ButtonBinding {
    ButtonAction action{};
    InspectorGroupKind group{};
    std::string groupName;
    std::size_t index{};
    std::string field;
};

struct EditorApp {
    std::unique_ptr<Editor::SceneEditor> editor;
    HWND window{};
    std::array<HWND, 5> toolButtons{};
    HWND outlinerLabel{};
    HWND outliner{};
    HWND inspectorLabel{};
    HWND inspector{};
    HWND assetsLabel{};
    HWND assets{};
    HWND status{};
    HWND viewport{};
    HFONT font{};
    HFONT boldFont{};
    Editor::EditorLayout layout{};
    std::vector<Editor::OutlinerRow> rows;
    std::vector<std::string> assetKinds;

    // Inspector state.
    std::vector<HWND> inspectorControls;
    std::vector<FieldBinding> fields;
    std::map<int, std::size_t> controlField;
    std::map<int, ButtonBinding> buttons;
    std::vector<HWND> inheritedLabels;
    int addComponentId{};
    int addBehaviourId{};
    HWND materialNameEdit{};
    std::string inspectorSignature;
    int inspectorContentHeight{};
    int inspectorScroll{};
    bool updatingInspector{};

    // Viewport state.
    int renderScale{1};
    bool autoHalfResolution{true};
    std::vector<std::uint32_t> bgra;
    POINT lastMouse{};
    bool orbiting{};
    bool panning{};
    bool reparenting{};

    // Play state.
    Framework::HostInput input;
    std::chrono::steady_clock::time_point lastTick{};
    std::chrono::steady_clock::time_point fpsStart{};
    int framesSinceFps{};
    double fps{};
    std::string lastError;
};

EditorApp g_app;
WNDPROC g_editProc = nullptr; // the system EDIT procedure the inspector boxes subclass

LRESULT CALLBACK InspectorEditProc(HWND edit, UINT message, WPARAM wParam, LPARAM lParam);

Editor::SceneEditor& Scene() { return *g_app.editor; }

// ---------------------------------------------------------------- helpers

HWND MakeControl(HWND parent, const wchar_t* className, const wchar_t* text, DWORD style, int id = 0, DWORD exStyle = 0) {
    HWND control = CreateWindowExW(exStyle, className, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 10, 10, parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
    if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.font), FALSE);
    return control;
}

void MoveControl(HWND control, int x, int y, int width, int height) {
    if (!control) return;
    MoveWindow(control, x, y, width > 0 ? width : 0, height > 0 ? height : 0, TRUE);
}

void SetStatus(const std::string& text) {
    if (g_app.status) SetWindowTextW(g_app.status, Wide(text).c_str());
}

void ReportError(const std::string& error) {
    g_app.lastError = error;
    SetStatus("Error: " + error);
    MessageBeep(MB_ICONWARNING);
}

std::string NodeLabel() {
    switch (Scene().Selected()) {
    case SelectionKind::Scene: return "Scene";
    case SelectionKind::Entity: return Scene().NodeName(Scene().Selection());
    case SelectionKind::None: break;
    }
    return "nothing selected";
}

void RefreshTitle() {
    std::string title = "Astral Editor";
    if (!Scene().ProjectPath().empty()) title += " - " + Scene().ProjectPath();
    title += " - " + (Scene().ScenePath().empty() ? std::string("untitled scene") : Scene().ScenePath());
    if (Scene().Dirty()) title += " *";
    if (Scene().Playing()) title += "  [PLAYING]";
    SetWindowTextW(g_app.window, Wide(title).c_str());
}

void RefreshStatus() {
    std::string text = Scene().Status();
    if (Scene().Playing()) {
        char fps[64];
        std::snprintf(fps, sizeof(fps), "Playing | %.0f fps | Esc or F5 stops", g_app.fps);
        text = fps;
    } else if (g_app.reparenting) {
        text = "Set parent: click the new parent in the Outliner (the Scene row moves it to the top level; Esc cancels)";
    } else if (text.empty()) {
        text = "Selected: " + NodeLabel() + " | root " + Scene().ContentRoot();
    }
    SetStatus(text);
}

void RefreshToolbar() {
    const int active = static_cast<int>(Scene().tool);
    for (int i = 0; i < 4; ++i) SendMessageW(g_app.toolButtons[i], BM_SETCHECK, i == active ? BST_CHECKED : BST_UNCHECKED, 0);
    SetWindowTextW(g_app.toolButtons[4], Scene().Playing() ? L"Stop (F5)" : L"Play (F5)");
    for (int i = 0; i < 4; ++i) EnableWindow(g_app.toolButtons[i], !Scene().Playing());
}

void RefreshOutliner() {
    g_app.rows = Scene().Outliner();
    SendMessageW(g_app.outliner, WM_SETREDRAW, FALSE, 0);
    SendMessageW(g_app.outliner, LB_RESETCONTENT, 0, 0);
    int selected = -1;
    for (std::size_t i = 0; i < g_app.rows.size(); ++i) {
        const Editor::OutlinerRow& row = g_app.rows[i];
        const std::string label = std::string(static_cast<std::size_t>(row.depth) * 3, ' ') + row.label;
        SendMessageW(g_app.outliner, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Wide(label).c_str()));
        const bool isSelected = (row.kind == SelectionKind::Scene && Scene().Selected() == SelectionKind::Scene)
            || (row.kind == SelectionKind::Entity && Scene().Selected() == SelectionKind::Entity && row.path == Scene().Selection());
        if (isSelected) selected = static_cast<int>(i);
    }
    SendMessageW(g_app.outliner, LB_SETCURSEL, static_cast<WPARAM>(selected), 0);
    SendMessageW(g_app.outliner, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_app.outliner, nullptr, TRUE);
}

std::wstring AssetLabel(const std::string& kind) {
    if (kind == "empty") return L"Empty entity";
    if (kind == "light:point") return L"Point light";
    if (kind == "light:directional") return L"Directional light";
    if (kind == "camera") return L"Camera (from the view)";
    if (kind.rfind("primitive:", 0) == 0) return L"Primitive: " + Wide(kind.substr(10));
    if (kind.rfind("prefab:", 0) == 0) return L"Prefab: " + Wide(kind.substr(7));
    if (kind.rfind("model:", 0) == 0) return L"Model: " + Wide(kind.substr(6));
    return Wide(kind);
}

void RefreshAssets() {
    g_app.assetKinds = Scene().CreatableKinds();
    SendMessageW(g_app.assets, LB_RESETCONTENT, 0, 0);
    for (const std::string& kind : g_app.assetKinds) {
        SendMessageW(g_app.assets, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(AssetLabel(kind).c_str()));
    }
}

void InvalidateViewport() {
    if (g_app.viewport) InvalidateRect(g_app.viewport, nullptr, FALSE);
}

// ---------------------------------------------------------------- inspector

std::string GroupTitle(const Editor::InspectorGroup& group) {
    switch (group.kind) {
    case InspectorGroupKind::Behaviour: return "Behaviour: " + group.name;
    case InspectorGroupKind::Material: return "Material: " + group.name;
    default: return group.name;
    }
}

std::string Signature(const std::vector<Editor::InspectorGroup>& groups) {
    std::string signature = std::to_string(static_cast<int>(Scene().Selected()));
    for (const auto& group : groups) {
        signature += "|" + std::to_string(static_cast<int>(group.kind)) + group.name + std::to_string(group.index)
            + (group.removable ? "r" : "");
        for (const auto& field : group.fields) signature += "," + field.name + (field.authored ? "*" : "");
    }
    signature += Scene().Selected() == SelectionKind::Entity ? std::to_string(Scene().AddableComponents().size()) : "";
    return signature;
}

void ShowFieldValue(const FieldBinding& binding) {
    const Editor::InspectorField& field = binding.field;
    switch (field.kind) {
    case Core::FieldKind::Bool:
        SendMessageW(binding.controls[0], BM_SETCHECK, field.value.AsBool() ? BST_CHECKED : BST_UNCHECKED, 0);
        break;
    case Core::FieldKind::Enum: {
        int selected = -1;
        for (std::size_t i = 0; i < field.enumNames.size(); ++i) {
            if (field.value.IsString() && field.value.AsString() == field.enumNames[i]) selected = static_cast<int>(i);
        }
        SendMessageW(binding.controls[0], CB_SETCURSEL, static_cast<WPARAM>(selected), 0);
        break;
    }
    case Core::FieldKind::String: SetWindowTextW(binding.controls[0], Wide(field.value.AsString("")).c_str()); break;
    case Core::FieldKind::Int:
    case Core::FieldKind::Float:
        SetWindowTextW(binding.controls[0], Wide(FormatNumber(field.value.AsNumber())).c_str());
        break;
    default:
        for (std::size_t i = 0; i < binding.controls.size(); ++i) {
            const double value = field.value.IsArray() && i < field.value.Size() ? field.value[i].AsNumber() : 0.0;
            SetWindowTextW(binding.controls[i], Wide(FormatNumber(value)).c_str());
        }
        break;
    }
}

void ClearInspector() {
    g_app.updatingInspector = true;
    for (HWND control : g_app.inspectorControls) DestroyWindow(control);
    g_app.updatingInspector = false;
    g_app.inspectorControls.clear();
    g_app.fields.clear();
    g_app.controlField.clear();
    g_app.buttons.clear();
    g_app.inheritedLabels.clear();
    g_app.addComponentId = 0;
    g_app.addBehaviourId = 0;
    g_app.materialNameEdit = nullptr;
}

void UpdateInspectorScrollbar() {
    RECT client{};
    GetClientRect(g_app.inspector, &client);
    const int page = std::max(1, static_cast<int>(client.bottom));
    g_app.inspectorScroll = std::clamp(g_app.inspectorScroll, 0, std::max(0, g_app.inspectorContentHeight - page));
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    info.nMin = 0;
    info.nMax = std::max(0, g_app.inspectorContentHeight - 1);
    info.nPage = static_cast<UINT>(page);
    info.nPos = g_app.inspectorScroll;
    SetScrollInfo(g_app.inspector, SB_VERT, &info, TRUE);
}

void BuildInspector() {
    ClearInspector();
    const std::vector<Editor::InspectorGroup> groups = Scene().Inspector();
    g_app.inspectorSignature = Signature(groups);
    RECT client{};
    GetClientRect(g_app.inspector, &client);
    const int width = std::max(120, static_cast<int>(client.right));
    const int labelWidth = std::min(96, width / 3);
    const int valueX = labelWidth + 8;
    const int valueWidth = std::max(40, width - valueX - 6);
    constexpr int kRow = 24;
    int y = 6 - g_app.inspectorScroll;
    int nextId = kFirstInspectorId;
    auto add = [&](HWND control) {
        g_app.inspectorControls.push_back(control);
        return control;
    };

    if (Scene().Selected() == SelectionKind::None) {
        HWND hint = add(MakeControl(g_app.inspector, L"STATIC",
            L"Select an entity in the viewport or the Outliner, or the Scene row for settings, environment and materials.",
            SS_LEFT));
        MoveControl(hint, 6, y, width - 12, 80);
        g_app.inspectorContentHeight = 90;
        UpdateInspectorScrollbar();
        return;
    }
    const bool playing = Scene().Playing();
    for (const Editor::InspectorGroup& group : groups) {
        HWND header = add(MakeControl(g_app.inspector, L"STATIC", Wide(GroupTitle(group)).c_str(), SS_LEFT | SS_ENDELLIPSIS));
        SendMessageW(header, WM_SETFONT, reinterpret_cast<WPARAM>(g_app.boldFont), FALSE);
        MoveControl(header, 6, y + 4, width - (group.removable ? 76 : 12), 18);
        if (group.removable) {
            const int id = nextId++;
            HWND remove = add(MakeControl(g_app.inspector, L"BUTTON", L"Remove", BS_PUSHBUTTON, id));
            MoveControl(remove, width - 66, y + 2, 60, 20);
            g_app.buttons[id] = {ButtonAction::RemoveGroup, group.kind, group.name, group.index, {}};
            EnableWindow(remove, !playing);
        }
        y += kRow;
        for (const Editor::InspectorField& field : group.fields) {
            FieldBinding binding;
            binding.group = group.kind;
            binding.groupName = group.name;
            binding.index = group.index;
            binding.field = field;
            const bool resettable = field.authored && group.kind != InspectorGroupKind::Entity
                && group.kind != InspectorGroupKind::Transform;
            HWND label = add(MakeControl(g_app.inspector, L"STATIC", Wide(field.name).c_str(), SS_LEFT | SS_ENDELLIPSIS | SS_NOTIFY));
            MoveControl(label, 10, y + 3, labelWidth - (resettable ? 18 : 4), 18);
            if (!field.authored && group.kind != InspectorGroupKind::Entity && group.kind != InspectorGroupKind::Transform) {
                g_app.inheritedLabels.push_back(label); // drawn grey: inherited or default
            }
            if (resettable) {
                const int id = nextId++;
                HWND reset = add(MakeControl(g_app.inspector, L"BUTTON", L"x", BS_PUSHBUTTON, id));
                MoveControl(reset, labelWidth - 12, y + 2, 16, 18);
                g_app.buttons[id] = {ButtonAction::ResetField, group.kind, group.name, group.index, field.name};
                EnableWindow(reset, !playing);
            }
            const std::size_t fieldIndex = g_app.fields.size();
            switch (field.kind) {
            case Core::FieldKind::Bool: {
                const int id = nextId++;
                binding.controls.push_back(add(MakeControl(g_app.inspector, L"BUTTON", L"", BS_AUTOCHECKBOX, id)));
                MoveControl(binding.controls.back(), valueX, y + 2, 20, 20);
                g_app.controlField[id] = fieldIndex;
                break;
            }
            case Core::FieldKind::Enum: {
                const int id = nextId++;
                HWND combo = add(MakeControl(g_app.inspector, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, id));
                for (const std::string& name : field.enumNames) {
                    SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Wide(name).c_str()));
                }
                MoveControl(combo, valueX, y, valueWidth, 200);
                binding.controls.push_back(combo);
                g_app.controlField[id] = fieldIndex;
                break;
            }
            default: {
                const int count = ComponentCount(field.kind);
                const int gap = 3;
                const int each = std::max(24, (valueWidth - gap * (count - 1)) / count);
                for (int c = 0; c < count; ++c) {
                    const int id = nextId++;
                    HWND edit = add(MakeControl(g_app.inspector, L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, id,
                        WS_EX_CLIENTEDGE));
                    if (g_editProc) SetWindowLongPtrW(edit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&InspectorEditProc));
                    MoveControl(edit, valueX + c * (each + gap), y, each, 21);
                    binding.controls.push_back(edit);
                    g_app.controlField[id] = fieldIndex;
                }
                break;
            }
            }
            for (HWND control : binding.controls) EnableWindow(control, !playing);
            g_app.fields.push_back(std::move(binding));
            ShowFieldValue(g_app.fields.back());
            y += kRow;
        }
        y += 6;
    }

    if (Scene().Selected() == SelectionKind::Entity && !playing) {
        g_app.addComponentId = nextId++;
        HWND components = add(MakeControl(g_app.inspector, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, g_app.addComponentId));
        SendMessageW(components, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Add component..."));
        for (const std::string& type : Scene().AddableComponents()) {
            SendMessageW(components, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Wide(type).c_str()));
        }
        SendMessageW(components, CB_SETCURSEL, 0, 0);
        MoveControl(components, 6, y, width - 12, 240);
        y += kRow + 4;
        g_app.addBehaviourId = nextId++;
        HWND behaviours = add(MakeControl(g_app.inspector, L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL, g_app.addBehaviourId));
        SendMessageW(behaviours, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Add behaviour..."));
        for (const std::string& type : Scene().BehaviourTypes()) {
            SendMessageW(behaviours, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Wide(type).c_str()));
        }
        SendMessageW(behaviours, CB_SETCURSEL, 0, 0);
        MoveControl(behaviours, 6, y, width - 12, 240);
        y += kRow + 4;
    } else if (Scene().Selected() == SelectionKind::Scene && !playing) {
        g_app.materialNameEdit = add(MakeControl(g_app.inspector, L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, nextId++,
            WS_EX_CLIENTEDGE));
        MoveControl(g_app.materialNameEdit, 6, y, width - 110, 21);
        const int id = nextId++;
        HWND addMaterial = add(MakeControl(g_app.inspector, L"BUTTON", L"Add material", BS_PUSHBUTTON, id));
        MoveControl(addMaterial, width - 100, y, 94, 22);
        g_app.buttons[id] = {ButtonAction::AddMaterial, InspectorGroupKind::Material, {}, 0, {}};
        y += kRow + 4;
    }
    g_app.inspectorContentHeight = y + g_app.inspectorScroll + 6;
    UpdateInspectorScrollbar();
}

// Rebuilds when the rows changed, otherwise only refreshes the values (keeps focus).
void RefreshInspector() {
    const std::vector<Editor::InspectorGroup> groups = Scene().Inspector();
    if (Signature(groups) != g_app.inspectorSignature || g_app.fields.empty()) {
        BuildInspector();
        return;
    }
    std::size_t index = 0;
    g_app.updatingInspector = true;
    for (const auto& group : groups) {
        for (const auto& field : group.fields) {
            if (index < g_app.fields.size()) {
                FieldBinding& binding = g_app.fields[index];
                binding.field = field;
                const auto& controls = binding.controls;
                if (std::find(controls.begin(), controls.end(), GetFocus()) == controls.end()) ShowFieldValue(binding);
            }
            ++index;
        }
    }
    g_app.updatingInspector = false;
}

void RefreshAll() {
    RefreshTitle();
    RefreshToolbar();
    RefreshOutliner();
    RefreshInspector();
    RefreshStatus();
    InvalidateViewport();
}

void AfterEdit(bool ok, const std::string& error) {
    if (!ok) {
        ReportError(error.empty() ? Scene().Status() : error);
        RefreshInspector();
        return;
    }
    g_app.lastError.clear();
    RefreshAll();
}

// Equal at the precision the inspector shows (0.7 typed back over 0.699999988 is no edit).
bool SameAsShown(const Core::JsonValue& a, const Core::JsonValue& b) {
    if (a.IsNumber() && b.IsNumber()) return FormatNumber(a.AsNumber()) == FormatNumber(b.AsNumber());
    if (a.IsArray() && b.IsArray() && a.Size() == b.Size()) {
        for (std::size_t i = 0; i < a.Size(); ++i) {
            if (!SameAsShown(a[i], b[i])) return false;
        }
        return true;
    }
    return a == b;
}

bool ReadField(const FieldBinding& binding, Core::JsonValue& value, std::string& error) {
    const Editor::InspectorField& field = binding.field;
    switch (field.kind) {
    case Core::FieldKind::Bool:
        value = SendMessageW(binding.controls[0], BM_GETCHECK, 0, 0) == BST_CHECKED;
        return true;
    case Core::FieldKind::Enum: {
        const LRESULT selected = SendMessageW(binding.controls[0], CB_GETCURSEL, 0, 0);
        if (selected < 0 || static_cast<std::size_t>(selected) >= field.enumNames.size()) return false;
        value = field.enumNames[static_cast<std::size_t>(selected)];
        return true;
    }
    case Core::FieldKind::String: value = WindowText(binding.controls[0]); return true;
    case Core::FieldKind::Int:
    case Core::FieldKind::Float: {
        double number = 0.0;
        if (!ParseNumber(WindowText(binding.controls[0]), number)) {
            error = field.name + " must be a number";
            return false;
        }
        value = field.kind == Core::FieldKind::Int ? std::round(number) : number;
        return true;
    }
    default: {
        value = Core::JsonValue::MakeArray();
        for (HWND control : binding.controls) {
            double number = 0.0;
            if (!ParseNumber(WindowText(control), number)) {
                error = field.name + " needs a number in every box";
                return false;
            }
            value.Append(number);
        }
        return true;
    }
    }
}

Math::Vec3 ReadVec3Field(const std::string& name, Math::Vec3 fallback) {
    for (const FieldBinding& binding : g_app.fields) {
        if (binding.group != InspectorGroupKind::Transform || binding.field.name != name) continue;
        Core::JsonValue value;
        std::string ignored;
        if (!ReadField(binding, value, ignored)) return fallback;
        return {value[0].AsFloat(), value[1].AsFloat(), value[2].AsFloat()};
    }
    return fallback;
}

void CommitField(std::size_t index) {
    if (g_app.updatingInspector || index >= g_app.fields.size() || Scene().Playing()) return;
    const FieldBinding binding = g_app.fields[index];
    Core::JsonValue value;
    std::string error;
    if (!ReadField(binding, value, error)) {
        if (!error.empty()) ReportError(error);
        ShowFieldValue(binding);
        return;
    }
    if (SameAsShown(value, binding.field.value)) return; // unchanged: no undo step
    bool ok = false;
    if (binding.group == InspectorGroupKind::Transform) {
        ok = Scene().SetTransform(ReadVec3Field("position", Scene().Position()), ReadVec3Field("rotation", Scene().EulerDegrees()),
            ReadVec3Field("scale", Scene().Scale()), error);
    } else if (binding.group == InspectorGroupKind::Entity && binding.field.name == "name") {
        ok = Scene().Rename(value.AsString(), error);
    } else {
        ok = Scene().SetField(binding.group, binding.groupName, binding.index, binding.field.name, value, error);
    }
    AfterEdit(ok, error);
}

void OnInspectorCommand(int id, int code, HWND control) {
    if (g_app.updatingInspector || Scene().Playing()) return;
    std::string error;
    if (id == g_app.addComponentId && code == CBN_SELCHANGE) {
        const LRESULT selected = SendMessageW(control, CB_GETCURSEL, 0, 0);
        const std::vector<std::string> types = Scene().AddableComponents();
        if (selected > 0 && static_cast<std::size_t>(selected) <= types.size()) {
            AfterEdit(Scene().AddComponent(types[static_cast<std::size_t>(selected) - 1], error), error);
        }
        return;
    }
    if (id == g_app.addBehaviourId && code == CBN_SELCHANGE) {
        const LRESULT selected = SendMessageW(control, CB_GETCURSEL, 0, 0);
        const std::vector<std::string> types = Scene().BehaviourTypes();
        if (selected > 0 && static_cast<std::size_t>(selected) <= types.size()) {
            AfterEdit(Scene().AddBehaviour(types[static_cast<std::size_t>(selected) - 1], error), error);
        }
        return;
    }
    if (const auto button = g_app.buttons.find(id); button != g_app.buttons.end() && code == BN_CLICKED) {
        const ButtonBinding binding = button->second;
        bool ok = false;
        switch (binding.action) {
        case ButtonAction::RemoveGroup:
            ok = binding.group == InspectorGroupKind::Behaviour ? Scene().RemoveBehaviour(binding.index, error)
                                                                 : Scene().RemoveComponent(binding.groupName, error);
            break;
        case ButtonAction::ResetField:
            ok = Scene().ResetField(binding.group, binding.groupName, binding.index, binding.field, error);
            break;
        case ButtonAction::AddMaterial:
            ok = Scene().AddMaterial(WindowText(g_app.materialNameEdit), error);
            break;
        }
        AfterEdit(ok, error);
        return;
    }
    const auto found = g_app.controlField.find(id);
    if (found == g_app.controlField.end()) return;
    const Core::FieldKind kind = g_app.fields[found->second].field.kind;
    if ((kind == Core::FieldKind::Bool && code == BN_CLICKED) || (kind == Core::FieldKind::Enum && code == CBN_SELCHANGE)
        || code == EN_KILLFOCUS) {
        CommitField(found->second);
    }
}

// Enter commits an inspector edit box, Escape restores its value.
LRESULT CALLBACK InspectorEditProc(HWND edit, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_KEYDOWN && (wParam == VK_RETURN || wParam == VK_ESCAPE)) {
        const auto found = g_app.controlField.find(GetDlgCtrlID(edit));
        if (found != g_app.controlField.end()) {
            if (wParam == VK_RETURN) {
                CommitField(found->second);
            } else if (found->second < g_app.fields.size()) {
                ShowFieldValue(g_app.fields[found->second]);
            }
        }
        return 0;
    }
    if (message == WM_CHAR && (wParam == VK_RETURN || wParam == VK_ESCAPE)) return 0; // no beep
    return CallWindowProcW(g_editProc, edit, message, wParam, lParam);
}

LRESULT CALLBACK InspectorProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        OnInspectorCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
        return 0;
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        const HWND control = reinterpret_cast<HWND>(lParam);
        const bool inherited = std::find(g_app.inheritedLabels.begin(), g_app.inheritedLabels.end(), control)
            != g_app.inheritedLabels.end();
        SetTextColor(dc, inherited ? RGB(120, 124, 132) : GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
        return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
    }
    case WM_VSCROLL:
    case WM_MOUSEWHEEL: {
        const int before = g_app.inspectorScroll;
        if (message == WM_MOUSEWHEEL) {
            g_app.inspectorScroll -= GET_WHEEL_DELTA_WPARAM(wParam) / 2;
        } else {
            SCROLLINFO info{};
            info.cbSize = sizeof(info);
            info.fMask = SIF_ALL;
            GetScrollInfo(window, SB_VERT, &info);
            switch (LOWORD(wParam)) {
            case SB_LINEUP: g_app.inspectorScroll -= 24; break;
            case SB_LINEDOWN: g_app.inspectorScroll += 24; break;
            case SB_PAGEUP: g_app.inspectorScroll -= static_cast<int>(info.nPage); break;
            case SB_PAGEDOWN: g_app.inspectorScroll += static_cast<int>(info.nPage); break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION: g_app.inspectorScroll = info.nTrackPos; break;
            default: break;
            }
        }
        UpdateInspectorScrollbar();
        if (g_app.inspectorScroll != before) {
            ScrollWindowEx(window, 0, before - g_app.inspectorScroll, nullptr, nullptr, nullptr, nullptr,
                SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
        }
        return 0;
    }
    default: return DefWindowProcW(window, message, wParam, lParam);
    }
}

// ---------------------------------------------------------------- viewport

int RenderScale() {
    RECT client{};
    GetClientRect(g_app.viewport, &client);
    const int area = static_cast<int>(client.right) * static_cast<int>(client.bottom);
    if (g_app.renderScale == 2) return 2;
    return g_app.autoHalfResolution && area > 1400 * 800 ? 2 : 1; // the CPU renderer stays interactive
}

void PresentViewport(HDC dc) {
    RECT client{};
    GetClientRect(g_app.viewport, &client);
    const int scale = RenderScale();
    const int width = std::max(1, static_cast<int>(client.right) / scale);
    const int height = std::max(1, static_cast<int>(client.bottom) / scale);
    const Graphics::ImageRgba8& image = Scene().Render(width, height);
    const std::size_t count = static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height);
    g_app.bgra.resize(count);
    const std::uint8_t* source = image.pixels.data();
    for (std::size_t index = 0; index < count; ++index, source += 4) {
        g_app.bgra[index] = 0xFF000000u | (static_cast<std::uint32_t>(source[0]) << 16)
            | (static_cast<std::uint32_t>(source[1]) << 8) | static_cast<std::uint32_t>(source[2]);
    }
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = image.width;
    info.bmiHeader.biHeight = -image.height; // top-down rows
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(dc, 0, 0, client.right, client.bottom, 0, 0, image.width, image.height, g_app.bgra.data(), &info,
        DIB_RGB_COLORS, SRCCOPY);
}

// Viewport coordinates in render pixels (the editor's gizmo sizes are in those).
void RenderPoint(LPARAM lParam, float& x, float& y, int& width, int& height) {
    RECT client{};
    GetClientRect(g_app.viewport, &client);
    const int scale = RenderScale();
    width = std::max(1, static_cast<int>(client.right) / scale);
    height = std::max(1, static_cast<int>(client.bottom) / scale);
    x = static_cast<float>(GET_X_LPARAM(lParam)) / static_cast<float>(scale);
    y = static_cast<float>(GET_Y_LPARAM(lParam)) / static_cast<float>(scale);
}

void StopPlaying() {
    if (!Scene().Playing()) return;
    Scene().Stop();
    g_app.input = {};
    RefreshAll();
    BuildInspector();
}

void StartPlaying() {
    std::string error;
    if (!Scene().Play(error)) {
        ReportError("Play failed: " + error);
        return;
    }
    g_app.input = {};
    g_app.lastTick = std::chrono::steady_clock::now();
    g_app.fpsStart = g_app.lastTick;
    g_app.framesSinceFps = 0;
    SetFocus(g_app.viewport);
    RefreshAll();
    BuildInspector();
}

void TogglePlay() {
    if (Scene().Playing()) {
        StopPlaying();
    } else {
        StartPlaying();
    }
}

void TickPlay() {
    Framework::GameHost* game = Scene().Game();
    if (!game) return;
    const auto now = std::chrono::steady_clock::now();
    const float dt = std::min(0.1f, std::chrono::duration<float>(now - g_app.lastTick).count());
    g_app.lastTick = now;
    game->Update(dt, g_app.input);
    g_app.input.ui.keysPressed.clear();
    g_app.input.ui.text.clear();
    g_app.input.keys.mouseDelta = {};
    if (game->QuitRequested()) {
        StopPlaying();
        return;
    }
    HDC dc = GetDC(g_app.viewport);
    PresentViewport(dc);
    ReleaseDC(g_app.viewport, dc);
    ++g_app.framesSinceFps;
    const double elapsed = std::chrono::duration<double>(now - g_app.fpsStart).count();
    if (elapsed >= 0.5) {
        g_app.fps = g_app.framesSinceFps / elapsed;
        g_app.framesSinceFps = 0;
        g_app.fpsStart = now;
        RefreshStatus();
    }
}

void AppendUtf8(std::string& out, wchar_t c) {
    const wchar_t pair[2] = {c, L'\0'};
    out += Utf8(pair);
}

// Keyboard and mouse while playing go to the game, like AstralPlayerWin32.
bool PlayInput(UINT message, WPARAM wParam, LPARAM lParam) {
    Framework::HostInput& input = g_app.input;
    switch (message) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        const auto key = static_cast<std::uint16_t>(wParam & 0xFF);
        const bool repeat = (lParam & (1 << 30)) != 0;
        if (!repeat && (key == VK_F5 || (key == VK_ESCAPE && !Scene().Game()->Console().IsOpen()))) {
            StopPlaying();
            return true;
        }
        input.keys.keys.set(key);
        if (!repeat) input.ui.keysPressed.push_back(key);
        if (!repeat && key == VK_F3) Scene().Game()->Commands().Execute("stat");
        input.ui.shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        return true;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP: input.keys.keys.reset(static_cast<std::size_t>(wParam & 0xFF)); return true;
    case WM_CHAR:
        if (wParam >= 32 && wParam != 127) AppendUtf8(input.ui.text, static_cast<wchar_t>(wParam));
        return true;
    case WM_MOUSEMOVE: {
        const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        input.keys.mouseDelta.x += static_cast<float>(point.x - g_app.lastMouse.x);
        input.keys.mouseDelta.y += static_cast<float>(point.y - g_app.lastMouse.y);
        g_app.lastMouse = point;
        const int scale = RenderScale();
        input.ui.pointer = {static_cast<float>(point.x) / static_cast<float>(scale), static_cast<float>(point.y) / static_cast<float>(scale)};
        return true;
    }
    case WM_LBUTTONDOWN:
        input.keys.keys.set(Input::Keys::MouseLeft);
        input.ui.pointerDown = true;
        SetCapture(g_app.viewport);
        return true;
    case WM_LBUTTONUP:
        input.keys.keys.reset(Input::Keys::MouseLeft);
        input.ui.pointerDown = false;
        ReleaseCapture();
        return true;
    case WM_RBUTTONDOWN: input.keys.keys.set(Input::Keys::MouseRight); return true;
    case WM_RBUTTONUP: input.keys.keys.reset(Input::Keys::MouseRight); return true;
    default: return false;
    }
}

LRESULT CALLBACK ViewportProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (!g_app.editor) return DefWindowProcW(window, message, wParam, lParam);
    if (message == WM_KILLFOCUS) {
        g_app.input.keys.keys.reset(); // no stuck keys after alt-tab
        return 0;
    }
    if (message == WM_PAINT) {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(window, &paint);
        PresentViewport(dc);
        EndPaint(window, &paint);
        return 0;
    }
    if (message == WM_ERASEBKGND) return 1;
    if (message == WM_SIZE) {
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    }
    if (Scene().Playing()) {
        if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN) SetFocus(window);
        if (PlayInput(message, wParam, lParam)) return 0;
        return DefWindowProcW(window, message, wParam, lParam);
    }
    float x = 0.0f;
    float y = 0.0f;
    int width = 1;
    int height = 1;
    switch (message) {
    case WM_LBUTTONDOWN: {
        SetFocus(window);
        RenderPoint(lParam, x, y, width, height);
        g_app.lastMouse = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if ((GetKeyState(VK_MENU) & 0x8000) != 0) {
            g_app.orbiting = true; // Alt + left drag orbits, as in Maya and Unreal
        } else if (Scene().BeginDrag(x, y, width, height)) {
            Scene().snap = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        } else if (Scene().Pick(x, y, width, height)) {
            RefreshOutliner();
            BuildInspector();
            RefreshStatus();
        }
        SetCapture(window);
        InvalidateViewport();
        return 0;
    }
    case WM_LBUTTONUP:
        if (Scene().Dragging()) {
            Scene().EndDrag();
            RefreshAll();
        }
        g_app.orbiting = false;
        ReleaseCapture();
        return 0;
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        SetFocus(window);
        g_app.lastMouse = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if (message == WM_MBUTTONDOWN || (GetKeyState(VK_SHIFT) & 0x8000) != 0) {
            g_app.panning = true;
        } else {
            g_app.orbiting = true;
        }
        SetCapture(window);
        return 0;
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        g_app.orbiting = false;
        g_app.panning = false;
        ReleaseCapture();
        return 0;
    case WM_MOUSEMOVE: {
        const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        const float dx = static_cast<float>(point.x - g_app.lastMouse.x);
        const float dy = static_cast<float>(point.y - g_app.lastMouse.y);
        g_app.lastMouse = point;
        RenderPoint(lParam, x, y, width, height);
        if (Scene().Dragging()) {
            Scene().snap = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            if (Scene().Drag(x, y, width, height)) {
                RefreshInspector();
                InvalidateViewport();
            }
        } else if (g_app.orbiting) {
            Scene().Camera().Orbit(dx, dy);
            InvalidateViewport();
        } else if (g_app.panning) {
            RECT client{};
            GetClientRect(window, &client);
            Scene().Camera().Pan(dx, dy, std::max(1, static_cast<int>(client.bottom)));
            InvalidateViewport();
        } else {
            const Editor::GizmoAxis axis = Scene().HitGizmo(x, y, width, height);
            static Editor::GizmoAxis lastAxis = Editor::GizmoAxis::None;
            if (axis != lastAxis) {
                lastAxis = axis;
                Scene().SetHoverAxis(axis);
                InvalidateViewport();
            }
        }
        return 0;
    }
    case WM_MOUSEWHEEL:
        Scene().Camera().Dolly(static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA);
        InvalidateViewport();
        return 0;
    default: return DefWindowProcW(window, message, wParam, lParam);
    }
}

// ---------------------------------------------------------------- commands

bool ConfirmDiscard() {
    if (!Scene().Dirty()) return true;
    const int answer = MessageBoxW(g_app.window, L"The scene has unsaved changes. Save them first?", L"Astral Editor",
        MB_YESNOCANCEL | MB_ICONQUESTION);
    if (answer == IDCANCEL) return false;
    if (answer == IDNO) return true;
    std::string error;
    if (Scene().Save(error)) return true;
    ReportError(error);
    return false;
}

std::wstring AbsoluteContentRoot() {
    std::error_code error;
    return std::filesystem::absolute(std::filesystem::path(Wide(Scene().ContentRoot())), error).wstring();
}

bool BrowseFile(bool save, const wchar_t* title, std::wstring& path) {
    wchar_t buffer[MAX_PATH]{};
    if (!path.empty()) wcsncpy_s(buffer, path.c_str(), _TRUNCATE);
    const std::wstring directory = AbsoluteContentRoot();
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_app.window;
    dialog.lpstrFilter = L"Astral JSON (*.json)\0*.json\0All files\0*.*\0";
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrInitialDir = directory.c_str();
    dialog.lpstrTitle = title;
    dialog.lpstrDefExt = L"json";
    dialog.Flags = OFN_NOCHANGEDIR | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (!(save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog))) return false;
    path = buffer;
    return true;
}

// Path relative to the content root, or empty when the file is outside it.
std::string RelativeToContentRoot(const std::wstring& file) {
    std::error_code error;
    const std::filesystem::path root = std::filesystem::weakly_canonical(std::filesystem::path(AbsoluteContentRoot()), error);
    const std::filesystem::path target = std::filesystem::weakly_canonical(std::filesystem::path(file), error);
    const std::filesystem::path relative = target.lexically_relative(root);
    if (relative.empty() || relative.native().rfind(L"..", 0) == 0) return {};
    return Utf8(relative.generic_wstring());
}

void ReplaceEditor(const std::string& contentRoot) {
    Scene().Stop();
    g_app.editor = std::make_unique<Editor::SceneEditor>(contentRoot);
}

void OpenFile(bool project) {
    if (!ConfirmDiscard()) return;
    std::wstring file;
    if (!BrowseFile(false, project ? L"Open project" : L"Open scene", file)) return;
    std::string relative = RelativeToContentRoot(file);
    if (project || relative.empty()) {
        // A project (or a scene elsewhere) brings its own content root: the file's folder.
        const std::filesystem::path path(file);
        ReplaceEditor(Utf8(path.parent_path().wstring()));
        relative = Utf8(path.filename().wstring());
    }
    std::string error;
    const bool ok = project ? Scene().OpenProject(relative, error) : Scene().OpenScene(relative, error);
    if (!ok) ReportError(error);
    g_app.inspectorScroll = 0;
    RefreshAssets();
    RefreshAll();
    BuildInspector();
}

void SaveAs() {
    std::wstring file = Wide(Scene().ScenePath());
    if (!BrowseFile(true, L"Save scene as", file)) return;
    const std::string relative = RelativeToContentRoot(file);
    std::string error;
    if (relative.empty()) {
        ReportError("save inside the content root (" + Utf8(AbsoluteContentRoot()) + ")");
        return;
    }
    AfterEdit(Scene().SaveAs(relative, error), error);
}

void Create(const std::string& kind) {
    std::string error;
    AfterEdit(Scene().CreateEntity(kind, error), error);
    BuildInspector();
}

void ShowControls() {
    MessageBoxW(g_app.window,
        L"Viewport\n"
        L"  Left click: select (or drag a gizmo handle)\n"
        L"  Right drag or Alt + left drag: orbit\n"
        L"  Middle drag or Shift + right drag: pan\n"
        L"  Wheel: zoom\n"
        L"  Q / W / E / R: select, move, rotate, scale\n"
        L"  Ctrl while dragging: snap (0.5 m, 15 degrees, 0.1)\n"
        L"  F: frame the selection    G: grid    F6: half resolution\n\n"
        L"Editing\n"
        L"  Ctrl+Z / Ctrl+Y: undo / redo    Ctrl+D: duplicate    Delete: delete\n"
        L"  Ctrl+P: set parent (then click the parent in the Outliner)\n"
        L"  Inspector: Enter or leaving a box applies it, Esc restores it,\n"
        L"  x resets a field to the prefab's or the default value (grey)\n"
        L"  Assets: double-click to create at the view centre\n\n"
        L"Files\n"
        L"  Ctrl+S save, Ctrl+O open scene, Ctrl+N new scene\n"
        L"  Scenes edited in a text editor reload when the document has no unsaved edits\n\n"
        L"Play\n"
        L"  F5 plays the current document with the project's input map; Esc or F5 stops.\n"
        L"  Changes made while playing are not kept.",
        L"Astral Editor controls", MB_OK | MB_ICONINFORMATION);
}

void OnCommand(int id) {
    std::string error;
    switch (id) {
    case kFileNew:
        if (!ConfirmDiscard()) return;
        Scene().NewScene();
        RefreshAssets();
        RefreshAll();
        BuildInspector();
        return;
    case kFileOpenProject: OpenFile(true); return;
    case kFileOpenScene: OpenFile(false); return;
    case kFileSave:
        if (Scene().ScenePath().empty()) {
            SaveAs();
        } else {
            AfterEdit(Scene().Save(error), error);
        }
        return;
    case kFileSaveAs: SaveAs(); return;
    case kFileExit: PostMessageW(g_app.window, WM_CLOSE, 0, 0); return;
    case kEditUndo:
    case kEditRedo:
        if (id == kEditUndo ? Scene().Undo() : Scene().Redo()) {
            RefreshAll();
            BuildInspector();
        } else {
            SetStatus(id == kEditUndo ? "Nothing to undo" : "Nothing to redo");
        }
        return;
    case kEditDuplicate: AfterEdit(Scene().Duplicate(error), error); BuildInspector(); return;
    case kEditDelete: AfterEdit(Scene().Delete(error), error); BuildInspector(); return;
    case kEditSetParent:
        if (Scene().Selected() != SelectionKind::Entity) {
            ReportError("select the entity to move first");
            return;
        }
        g_app.reparenting = true;
        RefreshStatus();
        return;
    case kEditFrame: Scene().FrameSelection(); InvalidateViewport(); return;
    case kCreateEmpty: Create("empty"); return;
    case kCreateCube: Create("primitive:cube"); return;
    case kCreateSphere: Create("primitive:sphere"); return;
    case kCreatePlane: Create("primitive:plane"); return;
    case kCreateCapsule: Create("primitive:capsule"); return;
    case kCreateCylinder: Create("primitive:cylinder"); return;
    case kCreatePointLight: Create("light:point"); return;
    case kCreateDirectionalLight: Create("light:directional"); return;
    case kCreateCamera: Create("camera"); return;
    case kViewGrid: Scene().showGrid = !Scene().showGrid; InvalidateViewport(); return;
    case kViewSnap: Scene().snap = !Scene().snap; InvalidateViewport(); return;
    case kViewHalfResolution:
        g_app.renderScale = RenderScale() == 2 ? 1 : 2;
        g_app.autoHalfResolution = false;
        InvalidateViewport();
        return;
    case kPlayToggle: TogglePlay(); return;
    case kHelpControls: ShowControls(); return;
    default: return;
    }
}

bool IsTextInput(HWND focus) {
    wchar_t name[32]{};
    if (!focus || !GetClassNameW(focus, name, 32)) return false;
    return _wcsicmp(name, L"Edit") == 0 || _wcsicmp(name, L"ComboBox") == 0;
}

// Editor shortcuts, checked before normal dispatch.
bool HandleShortcut(const MSG& message) {
    if (message.message != WM_KEYDOWN || !g_app.editor) return false;
    if (Scene().Playing()) return false; // the viewport routes keys to the game
    const bool control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool typing = IsTextInput(GetFocus());
    const auto key = static_cast<int>(message.wParam);
    if (key == VK_F5) {
        OnCommand(kPlayToggle);
        return true;
    }
    if (control) {
        switch (key) {
        case 'S': OnCommand((GetKeyState(VK_SHIFT) & 0x8000) != 0 ? kFileSaveAs : kFileSave); return true;
        case 'O': OnCommand(kFileOpenScene); return true;
        case 'N': OnCommand(kFileNew); return true;
        case 'Z': if (typing) return false; OnCommand(kEditUndo); return true;
        case 'Y': if (typing) return false; OnCommand(kEditRedo); return true;
        case 'D': OnCommand(kEditDuplicate); return true;
        case 'P': OnCommand(kEditSetParent); return true;
        default: return false;
        }
    }
    if (typing) return false;
    switch (key) {
    case VK_DELETE: OnCommand(kEditDelete); return true;
    case VK_ESCAPE:
        if (g_app.reparenting) {
            g_app.reparenting = false;
            RefreshStatus();
            return true;
        }
        return false;
    case VK_F6: OnCommand(kViewHalfResolution); return true;
    default: break;
    }
    // Tool keys only where they cannot mean text (viewport, lists, buttons).
    const GizmoTool tools[4] = {GizmoTool::Select, GizmoTool::Move, GizmoTool::Rotate, GizmoTool::Scale};
    const int toolKeys[4] = {'Q', 'W', 'E', 'R'};
    for (int i = 0; i < 4; ++i) {
        if (key == toolKeys[i]) {
            Scene().tool = tools[i];
            RefreshToolbar();
            InvalidateViewport();
            return true;
        }
    }
    if (key == 'F' || key == 'G') {
        OnCommand(key == 'F' ? kEditFrame : kViewGrid);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------- main window

HMENU BuildMenu() {
    HMENU bar = CreateMenu();
    HMENU file = CreatePopupMenu();
    AppendMenuW(file, MF_STRING, kFileNew, L"&New Scene\tCtrl+N");
    AppendMenuW(file, MF_STRING, kFileOpenProject, L"Open &Project...");
    AppendMenuW(file, MF_STRING, kFileOpenScene, L"&Open Scene...\tCtrl+O");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file, MF_STRING, kFileSave, L"&Save\tCtrl+S");
    AppendMenuW(file, MF_STRING, kFileSaveAs, L"Save &As...\tCtrl+Shift+S");
    AppendMenuW(file, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(file, MF_STRING, kFileExit, L"E&xit");
    HMENU edit = CreatePopupMenu();
    AppendMenuW(edit, MF_STRING, kEditUndo, L"&Undo\tCtrl+Z");
    AppendMenuW(edit, MF_STRING, kEditRedo, L"&Redo\tCtrl+Y");
    AppendMenuW(edit, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(edit, MF_STRING, kEditDuplicate, L"&Duplicate\tCtrl+D");
    AppendMenuW(edit, MF_STRING, kEditDelete, L"De&lete\tDel");
    AppendMenuW(edit, MF_STRING, kEditSetParent, L"Set &Parent...\tCtrl+P");
    AppendMenuW(edit, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(edit, MF_STRING, kEditFrame, L"&Frame Selection\tF");
    HMENU create = CreatePopupMenu();
    AppendMenuW(create, MF_STRING, kCreateEmpty, L"&Empty Entity");
    AppendMenuW(create, MF_STRING, kCreateCube, L"&Cube");
    AppendMenuW(create, MF_STRING, kCreateSphere, L"&Sphere");
    AppendMenuW(create, MF_STRING, kCreatePlane, L"&Plane");
    AppendMenuW(create, MF_STRING, kCreateCapsule, L"C&apsule");
    AppendMenuW(create, MF_STRING, kCreateCylinder, L"C&ylinder");
    AppendMenuW(create, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(create, MF_STRING, kCreatePointLight, L"Point &Light");
    AppendMenuW(create, MF_STRING, kCreateDirectionalLight, L"&Directional Light");
    AppendMenuW(create, MF_STRING, kCreateCamera, L"Ca&mera from View");
    HMENU view = CreatePopupMenu();
    AppendMenuW(view, MF_STRING, kViewGrid, L"&Grid\tG");
    AppendMenuW(view, MF_STRING, kViewSnap, L"&Snap (always)");
    AppendMenuW(view, MF_STRING, kViewHalfResolution, L"&Half Resolution\tF6");
    HMENU play = CreatePopupMenu();
    AppendMenuW(play, MF_STRING, kPlayToggle, L"&Play / Stop\tF5");
    HMENU help = CreatePopupMenu();
    AppendMenuW(help, MF_STRING, kHelpControls, L"&Controls");
    AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(file), L"&File");
    AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(edit), L"&Edit");
    AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(create), L"&Create");
    AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(view), L"&View");
    AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(play), L"&Play");
    AppendMenuW(bar, MF_POPUP, reinterpret_cast<UINT_PTR>(help), L"&Help");
    return bar;
}

void ApplyLayout(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    g_app.layout = Astral::Editor::ComputeEditorLayout(client.right, client.bottom);

    const auto toolbar = Astral::Editor::ComputeEditorToolbarLayout(g_app.layout.toolbar);
    const Editor::EditorRect buttons[5] = {toolbar.select, toolbar.move, toolbar.rotate, toolbar.scale, toolbar.play};
    for (int i = 0; i < 5; ++i) {
        MoveControl(g_app.toolButtons[i], buttons[i].x, buttons[i].y, buttons[i].width, buttons[i].height);
    }

    const auto outliner = Astral::Editor::ComputeEditorPanelContentLayout(g_app.layout.outliner);
    MoveControl(g_app.outlinerLabel, outliner.label.x, outliner.label.y, outliner.label.width, outliner.label.height);
    MoveControl(g_app.outliner, outliner.body.x, outliner.body.y, outliner.body.width, outliner.body.height);

    const auto inspector = Astral::Editor::ComputeEditorPanelContentLayout(g_app.layout.inspector);
    MoveControl(g_app.inspectorLabel, inspector.label.x, inspector.label.y, inspector.label.width, inspector.label.height);
    MoveControl(g_app.inspector, inspector.body.x, inspector.body.y, inspector.body.width, inspector.body.height);

    const auto assets = Astral::Editor::ComputeEditorPanelContentLayout(g_app.layout.assets);
    MoveControl(g_app.assetsLabel, assets.label.x, assets.label.y, assets.label.width, assets.label.height);
    MoveControl(g_app.assets, assets.body.x, assets.body.y, assets.body.width, assets.body.height);

    const auto clip = Astral::Editor::ComputeEditorViewportClipRect(g_app.layout.viewport);
    MoveControl(g_app.viewport, clip.x, clip.y, clip.width, clip.height);

    const auto status = Astral::Editor::ComputeEditorStatusContentLayout(g_app.layout.status);
    MoveControl(g_app.status, status.x, status.y, status.width, status.height);
    if (g_app.editor) BuildInspector();
    InvalidateRect(window, nullptr, TRUE);
}

void OnOutlinerSelection() {
    const LRESULT selected = SendMessageW(g_app.outliner, LB_GETCURSEL, 0, 0);
    if (selected < 0 || static_cast<std::size_t>(selected) >= g_app.rows.size()) return;
    const Editor::OutlinerRow& row = g_app.rows[static_cast<std::size_t>(selected)];
    if (g_app.reparenting) {
        g_app.reparenting = false;
        std::string error;
        AfterEdit(Scene().Reparent(row.kind == SelectionKind::Scene ? Editor::NodePath{} : row.path, error), error);
        BuildInspector();
        return;
    }
    if (row.kind == SelectionKind::Scene) {
        Scene().SelectScene();
    } else {
        Scene().Select(row.path);
    }
    g_app.inspectorScroll = 0;
    BuildInspector();
    RefreshStatus();
    InvalidateViewport();
}

LRESULT CALLBACK EditorWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        g_app.window = window;
        const wchar_t* toolNames[5] = {L"Select (Q)", L"Move (W)", L"Rotate (E)", L"Scale (R)", L"Play (F5)"};
        for (int i = 0; i < 5; ++i) {
            g_app.toolButtons[i] = MakeControl(window, L"BUTTON", toolNames[i],
                i < 4 ? BS_AUTOCHECKBOX | BS_PUSHLIKE : BS_PUSHBUTTON, kToolButtonIds[i]);
        }
        g_app.outlinerLabel = MakeControl(window, L"STATIC", L"OUTLINER", SS_LEFT);
        g_app.outliner = MakeControl(window, L"LISTBOX", L"", LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER | WS_VSCROLL | WS_HSCROLL,
            kOutlinerId);
        g_app.inspectorLabel = MakeControl(window, L"STATIC", L"INSPECTOR", SS_LEFT);
        g_app.inspector = CreateWindowExW(WS_EX_CLIENTEDGE, kInspectorClass, L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN,
            0, 0, 10, 10, window, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_app.assetsLabel = MakeControl(window, L"STATIC", L"ASSETS (double-click to create)", SS_LEFT);
        g_app.assets = MakeControl(window, L"LISTBOX", L"", LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER | WS_VSCROLL, kAssetListId);
        g_app.status = MakeControl(window, L"STATIC", L"", SS_LEFT | SS_ENDELLIPSIS);
        g_app.viewport = CreateWindowExW(0, kViewportClass, L"", WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, window, nullptr,
            GetModuleHandleW(nullptr), nullptr);

        const Astral::Editor::EditorControlCreationState createdControls{{
            g_app.toolButtons[0] != nullptr,
            g_app.toolButtons[1] != nullptr,
            g_app.toolButtons[2] != nullptr,
            g_app.toolButtons[3] != nullptr,
            g_app.toolButtons[4] != nullptr,
            g_app.outlinerLabel != nullptr,
            g_app.outliner != nullptr,
            g_app.inspectorLabel != nullptr,
            g_app.inspector != nullptr,
            g_app.assetsLabel != nullptr,
            g_app.assets != nullptr,
            g_app.status != nullptr,
        }};
        if (!Astral::Editor::AreRequiredEditorControlsCreated(createdControls) || !g_app.viewport) return -1;
        using Astral::Editor::EditorTool;
        const EditorTool tools[5] = {EditorTool::Select, EditorTool::Move, EditorTool::Rotate, EditorTool::Scale, EditorTool::Play};
        for (int i = 0; i < 5; ++i) {
            EnableWindow(g_app.toolButtons[i], Astral::Editor::IsEditorToolAvailable(tools[i]) ? TRUE : FALSE);
        }
        SetTimer(window, kDiskPollTimer, 700, nullptr);
        ApplyLayout(window);
        return 0;
    }
    case WM_SIZE:
        ApplyLayout(window);
        return 0;
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        const int code = HIWORD(wParam);
        if (id == kOutlinerId && code == LBN_SELCHANGE) {
            OnOutlinerSelection();
            return 0;
        }
        if (id == kOutlinerId && code == LBN_DBLCLK) {
            OnCommand(kEditFrame);
            return 0;
        }
        if (id == kAssetListId && code == LBN_DBLCLK) {
            const LRESULT selected = SendMessageW(g_app.assets, LB_GETCURSEL, 0, 0);
            if (selected >= 0 && static_cast<std::size_t>(selected) < g_app.assetKinds.size()) {
                Create(g_app.assetKinds[static_cast<std::size_t>(selected)]);
            }
            return 0;
        }
        for (int i = 0; i < 5; ++i) {
            if (id != kToolButtonIds[i]) continue;
            if (i == 4) {
                TogglePlay();
            } else {
                Scene().tool = static_cast<GizmoTool>(i);
                RefreshToolbar();
                InvalidateViewport();
            }
            SetFocus(g_app.viewport);
            return 0;
        }
        if (code == 0 && lParam == 0) OnCommand(id); // menu
        return 0;
    }
    case WM_TIMER:
        if (wParam == kDiskPollTimer && g_app.editor && !Scene().Playing() && !Scene().Dragging()
            && Scene().ReloadIfChangedOnDisk()) {
            RefreshAssets();
            RefreshAll();
            BuildInspector();
        }
        return 0;
    case WM_CLOSE:
        if (g_app.editor) {
            StopPlaying();
            if (!ConfirmDiscard()) return 0;
        }
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        KillTimer(window, kDiskPollTimer);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

bool RegisterClasses(HINSTANCE instance) {
    WNDCLASSW windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpfnWndProc = EditorWindowProc;
    windowClass.lpszClassName = kEditorWindowClass;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    if (!RegisterClassW(&windowClass)) return false;

    WNDCLASSW viewportClass{};
    viewportClass.hInstance = instance;
    viewportClass.lpfnWndProc = ViewportProc;
    viewportClass.lpszClassName = kViewportClass;
    viewportClass.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    if (!RegisterClassW(&viewportClass)) return false;

    WNDCLASSW inspectorClass{};
    inspectorClass.hInstance = instance;
    inspectorClass.lpfnWndProc = InspectorProc;
    inspectorClass.lpszClassName = kInspectorClass;
    inspectorClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    inspectorClass.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
    return RegisterClassW(&inspectorClass) != 0;
}

// Subclass procedure shared by every inspector edit box.
void InstallEditSubclass() {
    WNDCLASSW edit{};
    if (GetClassInfoW(nullptr, L"EDIT", &edit)) g_editProc = edit.lpfnWndProc;
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    std::string contentRoot = ".";
    std::string startPath;
    if (__argc > 1) contentRoot = Utf8(__wargv[1]);
    if (__argc > 2) startPath = Utf8(__wargv[2]);
    std::error_code fileError;
    if (__argc <= 1 && std::filesystem::exists("Content/Samples/Playground/project.json", fileError)) {
        contentRoot = "Content/Samples/Playground";
        startPath = "project.json";
    }

    Samples::RegisterPlaygroundBehaviours();
    g_app.editor = std::make_unique<Editor::SceneEditor>(contentRoot);
    std::string startError;
    if (!startPath.empty()) {
        const auto probe = Scene().Assets().Load<Core::JsonValue>(startPath);
        const bool project = probe.Ready() && probe->String("format") == "astral-project";
        const bool ok = project ? Scene().OpenProject(startPath, startError) : Scene().OpenScene(startPath, startError);
        if (!ok) startError = "Cannot open " + contentRoot + "/" + startPath + ": " + startError;
    }

    g_app.font = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    g_app.boldFont = CreateFontW(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    InstallEditSubclass();
    if (!RegisterClasses(instance)) return 1;

    HWND window = CreateWindowExW(0, kEditorWindowClass, L"Astral Editor", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1600, 960, nullptr, BuildMenu(), instance, nullptr);
    if (!window) return 2;

    RefreshAssets();
    RefreshAll();
    BuildInspector();
    if (!startError.empty()) ReportError(startError);
    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (true) {
        if (g_app.editor && Scene().Playing()) {
            // Play mode runs a frame loop like AstralPlayerWin32.
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_QUIT) return static_cast<int>(message.wParam);
                if (HandleShortcut(message)) continue;
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            TickPlay();
            continue;
        }
        const BOOL getMessageResult = GetMessageW(&message, nullptr, 0, 0);
        switch (Astral::Editor::ClassifyEditorMessageResult(static_cast<int>(getMessageResult))) {
        case Astral::Editor::EditorMessageLoopAction::Error:
            return kMessageLoopErrorExitCode;
        case Astral::Editor::EditorMessageLoopAction::Quit:
            return static_cast<int>(message.wParam);
        case Astral::Editor::EditorMessageLoopAction::Dispatch:
            if (HandleShortcut(message)) break;
            TranslateMessage(&message);
            DispatchMessageW(&message);
            break;
        }
    }
}
