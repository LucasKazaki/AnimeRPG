// Scene editor core (AstralEditor's model): the orbit camera and its picking
// rays, the authored-document model with transactional edits and undo/redo,
// the reflected inspector, picking and gizmo drags in the viewport, saving,
// disk hot reload, and Play-in-editor through the real game host.

#include "Engine/Editor/EditorCamera.h"
#include "Engine/Editor/SceneEditor.h"
#include "Engine/Framework/Components.h"
#include "Engine/Framework/GameHost.h"
#include "Engine/Input/InputSystem.h"
#include "Game/Samples/Playground/PlaygroundBehaviours.h"
#include "Tests/EngineTestSupport.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#endif

using namespace Astral;
using Editor::GizmoAxis;
using Editor::GizmoTool;
using Editor::InspectorGroupKind;
using Editor::NodePath;
using Editor::SceneEditor;
using Editor::SelectionKind;
using Math::Vec2;
using Math::Vec3;

namespace {

#ifndef ASTRAL_SOURCE_ROOT
#define ASTRAL_SOURCE_ROOT "."
#endif

constexpr int kWidth = 320;
constexpr int kHeight = 180;

struct PreviewProbeCounts {
    int constructed{};
    int created{};
    int destroyed{};
};
PreviewProbeCounts previewProbeCounts;

struct EditorPreviewProbe final : Framework::Behaviour {
    EditorPreviewProbe() { ++previewProbeCounts.constructed; }
    void OnCreate() override { ++previewProbeCounts.created; }
    void OnDestroy() override { ++previewProbeCounts.destroyed; }
};

std::string PlaygroundRoot() { return std::string(ASTRAL_SOURCE_ROOT) + "/Content/Samples/Playground"; }

std::unique_ptr<SceneEditor> OpenPlayground(const std::string& root = PlaygroundRoot()) {
    Samples::RegisterPlaygroundBehaviours();
    auto editor = std::make_unique<SceneEditor>(root, 0); // inline jobs: deterministic, sanitizer-friendly
    std::string error;
    if (!editor->OpenProject("project.json", error)) std::fprintf(stderr, "playground failed to open: %s\n", error.c_str());
    return editor;
}

// A private copy of the Playground so saves and disk edits never touch the source tree.
std::string CopyPlayground(const char* name) {
    const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(path);
    std::filesystem::copy(PlaygroundRoot(), path, std::filesystem::copy_options::recursive);
    return path.string();
}

NodePath Find(const SceneEditor& editor, const char* name) {
    NodePath path;
    ASTRAL_CHECK(editor.FindNode(name, path));
    return path;
}

bool Near(Vec3 a, Vec3 b, float tolerance = 1.0e-3f) {
    return std::fabs(a.x - b.x) <= tolerance && std::fabs(a.y - b.y) <= tolerance && std::fabs(a.z - b.z) <= tolerance;
}

const Editor::InspectorGroup* Group(const std::vector<Editor::InspectorGroup>& groups, InspectorGroupKind kind,
    const std::string& name) {
    for (const auto& group : groups) {
        if (group.kind == kind && group.name == name) return &group;
    }
    return nullptr;
}

const Editor::InspectorField* Field(const Editor::InspectorGroup* group, const std::string& name) {
    if (!group) return nullptr;
    for (const auto& field : group->fields) {
        if (field.name == name) return &field;
    }
    return nullptr;
}

Core::JsonValue Vec3Json(float x, float y, float z) {
    Core::JsonValue array = Core::JsonValue::MakeArray();
    array.Append(x);
    array.Append(y);
    array.Append(z);
    return array;
}

// Centre of the selection on screen.
Vec2 SelectionPixel(const SceneEditor& editor) {
    Vec2 pixel{};
    ASTRAL_CHECK(editor.Camera().Project(editor.SelectionBounds().Center(), kWidth, kHeight, pixel));
    return pixel;
}

struct EditorFiles {
    std::filesystem::path directory;
    std::filesystem::path root;
    std::filesystem::path outside;
    EditorFiles() {
        static unsigned sequence = 0;
        directory = std::filesystem::temp_directory_path() / ("astral-editor-contract-"
            + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
            + "-" + std::to_string(++sequence));
        root = directory / "content";
        outside = directory / "outside";
        std::filesystem::create_directories(root);
        std::filesystem::create_directories(outside);
    }
    ~EditorFiles() { std::filesystem::remove_all(directory); }
};

// Observe document/selection/history and the actual preview without exposing
// implementation-only state. Rejection may update the visible diagnostic.
struct AuthorState {
    Core::JsonValue document;
    SelectionKind kind;
    NodePath selection;
    std::string scenePath, projectPath;
    bool dirty, undo, redo;
    Vec3 position;
    Framework::Entity previewEntity;
    explicit AuthorState(const SceneEditor& editor)
        : document(editor.Document()), kind(editor.Selected()), selection(editor.Selection()),
          scenePath(editor.ScenePath()), projectPath(editor.ProjectPath()), dirty(editor.Dirty()),
          undo(editor.CanUndo()), redo(editor.CanRedo()), position(editor.Position()),
          previewEntity(editor.PreviewEntity(selection)) {}
    void Check(const SceneEditor& editor) const {
        ASTRAL_CHECK(editor.Document() == document && editor.Selected() == kind && editor.Selection() == selection);
        ASTRAL_CHECK(editor.ScenePath() == scenePath && editor.ProjectPath() == projectPath);
        ASTRAL_CHECK(editor.Dirty() == dirty && editor.CanUndo() == undo && editor.CanRedo() == redo);
        ASTRAL_CHECK(Near(editor.Position(), position) && editor.PreviewEntity(selection) == previewEntity);
        if (!previewEntity.IsNull()) {
            ASTRAL_CHECK(Near(editor.Preview().GetLocal(previewEntity).translation, position));
        }
    }
};

bool EditorDirectoryLink(const std::filesystem::path& target, const std::filesystem::path& link) {
#if defined(_WIN32)
    // Local junctions work without the symbolic-link privilege.
    struct JunctionData {
        DWORD tag; WORD length, reserved, substituteOffset, substituteLength, printOffset, printLength;
        wchar_t paths[4096];
    } data{};
    const std::wstring absolute = std::filesystem::absolute(target).make_preferred().wstring();
    const std::wstring substitute = L"\\??\\" + absolute;
    if (substitute.size() + absolute.size() + 2 > 4096) return false;
    data.tag = IO_REPARSE_TAG_MOUNT_POINT;
    data.substituteLength = static_cast<WORD>(substitute.size() * sizeof(wchar_t));
    data.printOffset = static_cast<WORD>((substitute.size() + 1) * sizeof(wchar_t));
    data.printLength = static_cast<WORD>(absolute.size() * sizeof(wchar_t));
    std::memcpy(data.paths, substitute.c_str(), data.substituteLength + sizeof(wchar_t));
    std::memcpy(data.paths + substitute.size() + 1, absolute.c_str(), data.printLength + sizeof(wchar_t));
    data.length = static_cast<WORD>(8 + data.printOffset + data.printLength + sizeof(wchar_t));
    if (!CreateDirectoryW(link.c_str(), nullptr)) return false;
    HANDLE handle = CreateFileW(link.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return false;
    DWORD returned = 0;
    const bool ok = DeviceIoControl(handle, FSCTL_SET_REPARSE_POINT, &data, data.length + 8,
        nullptr, 0, &returned, nullptr) != FALSE;
    CloseHandle(handle);
    return ok;
#else
    std::error_code ec;
    std::filesystem::create_directory_symlink(target, link, ec);
    return !ec;
#endif
}

std::string FileText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

} // namespace

ASTRAL_TEST(EulerAnglesRoundTripThroughQuaternions) {
    const Vec3 cases[] = {{0, 0, 0}, {30, 0, 0}, {0, 45, 0}, {0, 0, 60}, {52, 35, 0}, {-20, 170, 15}, {80, -120, -45},
        {-85, 10, 170}};
    for (const Vec3& euler : cases) {
        const Math::Quat q = Editor::QuatFromEulerDegrees(euler);
        const Vec3 back = Editor::EulerDegreesFromQuat(q);
        ASTRAL_CHECK(Near(back, euler, 0.05f));
        // Same rotation, checked on the rotated basis.
        const Math::Quat again = Editor::QuatFromEulerDegrees(back);
        ASTRAL_CHECK(Near(Math::Rotate(q, {1, 0, 0}), Math::Rotate(again, {1, 0, 0})));
        ASTRAL_CHECK(Near(Math::Rotate(q, {0, 0, 1}), Math::Rotate(again, {0, 0, 1})));
    }
}

ASTRAL_TEST(CameraRaysAndProjectionAgree) {
    Editor::EditorCamera camera;
    camera.target = {2, 1, 3};
    camera.yawDegrees = 40;
    camera.pitchDegrees = 20;
    camera.distance = 12;
    const Math::Ray centre = camera.RayThrough(kWidth * 0.5f, kHeight * 0.5f, kWidth, kHeight);
    ASTRAL_CHECK(Near(centre.direction, camera.Forward()));
    ASTRAL_CHECK(Near(camera.Eye() + camera.Forward() * camera.distance, camera.target));
    for (const Vec2 pixel : {Vec2{10, 20}, Vec2{300, 170}, Vec2{160, 5}}) {
        const Math::Ray ray = camera.RayThrough(pixel.x, pixel.y, kWidth, kHeight);
        Vec2 projected{};
        ASTRAL_CHECK(camera.Project(ray.At(25.0f), kWidth, kHeight, projected));
        ASTRAL_CHECK_NEAR(projected.x, pixel.x, 0.01);
        ASTRAL_CHECK_NEAR(projected.y, pixel.y, 0.01);
    }
    Vec2 ignored{};
    ASTRAL_CHECK(!camera.Project(camera.Eye() - camera.Forward(), kWidth, kHeight, ignored)); // behind the eye
    // Navigation keeps the focus sensible.
    camera.Orbit(1000, 1000);
    ASTRAL_CHECK(camera.pitchDegrees <= 89.0f);
    const float before = camera.distance;
    camera.Dolly(2);
    ASTRAL_CHECK(camera.distance < before);
    camera.Frame(Math::AABB::FromCenterExtents({5, 0, 5}, {1, 1, 1}));
    ASTRAL_CHECK(Near(camera.target, {5, 0, 5}));
}

ASTRAL_TEST(OpensThePlaygroundAndMapsEveryAuthoredNode) {
    auto editor = OpenPlayground();
    ASTRAL_CHECK(editor->ProjectPath() == "project.json" && !editor->ScenePath().empty() && !editor->Dirty());
    const auto rows = editor->Outliner();
    ASTRAL_CHECK(rows.size() > 10 && rows.front().kind == SelectionKind::Scene);
    for (const auto& row : rows) {
        if (row.kind != SelectionKind::Entity) continue;
        const Framework::Entity entity = editor->PreviewEntity(row.path);
        ASTRAL_CHECK(!entity.IsNull());
        const World::Name* name = editor->Preview().Get<World::Name>(entity);
        ASTRAL_CHECK(name && name->value == editor->NodeName(row.path));
    }
    const NodePath body = Find(*editor, "PlayerBody");
    ASTRAL_CHECK(body.size() == 2 && editor->NodeName({body[0]}) == "Player");
    // Behaviours do not run in the editor preview.
    ASTRAL_CHECK(editor->Preview().Stats().behaviours == 0);
}

ASTRAL_TEST(TransformEditsMoveThePreviewAndUndo) {
    auto editor = OpenPlayground();
    ASTRAL_CHECK(editor->Select(Find(*editor, "Player")));
    const Vec3 start = editor->Position();
    std::string error;
    ASTRAL_CHECK(editor->SetTransform(start + Vec3{2, 0, 1}, {0, 90, 0}, {1, 1, 1}, error));
    ASTRAL_CHECK(editor->Dirty() && editor->CanUndo());
    const Framework::Entity player = editor->PreviewEntity(editor->Selection());
    ASTRAL_CHECK(Near(editor->Preview().GetWorld(player).translation, start + Vec3{2, 0, 1}));
    ASTRAL_CHECK(Near(editor->EulerDegrees(), {0, 90, 0}));
    const Core::JsonValue& written = (*editor->Document().Find("entities"))[editor->Selection()[0]]["transform"];
    ASTRAL_CHECK(written["rotation"]["euler"][1].AsNumber() == 90.0);
    ASTRAL_CHECK(!editor->SetTransform(start, {}, {1, 0, 1}, error) && error.find("scale") != std::string::npos);

    ASTRAL_CHECK(editor->Undo());
    ASTRAL_CHECK(Near(editor->Position(), start) && editor->CanRedo());
    ASTRAL_CHECK(!editor->Dirty()); // back to the opened document
    ASTRAL_CHECK(Near(editor->Preview().GetWorld(editor->PreviewEntity(editor->Selection())).translation, start));
    ASTRAL_CHECK(editor->Redo());
    ASTRAL_CHECK(Near(editor->Position(), start + Vec3{2, 0, 1}) && editor->Dirty());
}

ASTRAL_TEST(InvalidEditsAreRejectedAndLeaveTheDocumentAlone) {
    auto editor = OpenPlayground();
    ASTRAL_CHECK(editor->Select(Find(*editor, "Sun")));
    const Core::JsonValue before = editor->Document();
    std::string error;
    ASTRAL_CHECK(!editor->SetField(InspectorGroupKind::Component, "Light", 0, "type", Core::JsonValue("Laser"), error));
    ASTRAL_CHECK(error.find("Light") != std::string::npos && error.find("type") != std::string::npos);
    ASTRAL_CHECK(!editor->AddComponent("Teleporter", error));
    ASTRAL_CHECK(!editor->AddComponent("Light", error)); // already there
    ASTRAL_CHECK(!editor->CreateEntity("primitive:torus", error));
    ASTRAL_CHECK(editor->Document() == before && !editor->CanUndo() && !editor->Dirty());
    ASTRAL_CHECK(editor->SetField(InspectorGroupKind::Component, "Light", 0, "intensity", Core::JsonValue(1.5), error));
    ASTRAL_CHECK(editor->Document() != before && editor->CanUndo());
}

ASTRAL_TEST(CreateDuplicateDeleteAndReparentEntities) {
    auto editor = OpenPlayground();
    const std::size_t rows = editor->Outliner().size();
    std::string error;
    ASTRAL_CHECK(editor->CreateEntity("primitive:cube", error));
    ASTRAL_CHECK(editor->Selected() == SelectionKind::Entity && editor->NodeName(editor->Selection()) == "Cube");
    ASTRAL_CHECK(!editor->PreviewEntity(editor->Selection()).IsNull());
    ASTRAL_CHECK(editor->Preview().Get<Framework::MeshRenderer>(editor->PreviewEntity(editor->Selection())));
    ASTRAL_CHECK(editor->Duplicate(error) && editor->NodeName(editor->Selection()) == "Cube 2");
    ASTRAL_CHECK(editor->Outliner().size() == rows + 2);
    ASTRAL_CHECK(editor->CreateEntity("prefab:Crate", error) && editor->NodeName(editor->Selection()) == "Crate");
    ASTRAL_CHECK(editor->CreateEntity("light:point", error) && editor->CreateEntity("camera", error));
    ASTRAL_CHECK(editor->Delete(error) && editor->Outliner().size() == rows + 4);

    // Put "Cube 2" under the Player, then undo everything back to the opened scene.
    ASTRAL_CHECK(editor->Select(Find(*editor, "Cube 2")));
    const NodePath player = Find(*editor, "Player");
    ASTRAL_CHECK(editor->Reparent(player, error));
    ASTRAL_CHECK(editor->Selection().size() == 2 && editor->Selection()[0] == player[0]);
    ASTRAL_CHECK(editor->NodeName(editor->Selection()) == "Cube 2");
    ASTRAL_CHECK(!editor->Reparent(editor->Selection(), error)); // not under itself
    while (editor->Undo()) {
    }
    ASTRAL_CHECK(editor->Outliner().size() == rows);
}

ASTRAL_TEST(PickingSelectsWhatIsUnderTheCursor) {
    auto editor = OpenPlayground();
    ASTRAL_CHECK(editor->Select(Find(*editor, "Crate1")));
    editor->FrameSelection();
    editor->ClearSelection();
    Vec2 pixel{};
    const Framework::Entity crate = editor->PreviewEntity(Find(*editor, "Crate1"));
    ASTRAL_CHECK(editor->Camera().Project(editor->Preview().GetWorld(crate).translation, kWidth, kHeight, pixel));
    ASTRAL_CHECK(editor->Pick(pixel.x, pixel.y, kWidth, kHeight));
    ASTRAL_CHECK(editor->NodeName(editor->Selection()) == "Crate1");

    // A prefab's own children select the authored instance.
    NodePath lamp;
    for (const auto& row : editor->Outliner()) {
        if (row.label.find("[Lamp]") != std::string::npos) lamp = row.path;
    }
    ASTRAL_CHECK(!lamp.empty() && editor->Select(lamp));
    editor->FrameSelection();
    const Framework::Entity post = World::GetChildren(editor->Preview().Registry(), editor->PreviewEntity(lamp)).front();
    ASTRAL_CHECK(editor->Camera().Project(editor->Preview().GetWorld(post).translation, kWidth, kHeight, pixel));
    editor->ClearSelection();
    editor->Pick(pixel.x, pixel.y, kWidth, kHeight);
    ASTRAL_CHECK(editor->Selected() == SelectionKind::Entity && editor->Selection() == lamp);

    // Up into the empty sky (from above the scene, not from under the ground) selects the scene.
    editor->Camera().target = {0.0f, 200.0f, 0.0f};
    editor->Camera().distance = 20.0f;
    editor->Camera().pitchDegrees = -60.0f;
    editor->Pick(kWidth * 0.5f, 2.0f, kWidth, kHeight);
    ASTRAL_CHECK(editor->Selected() == SelectionKind::Scene);
}

ASTRAL_TEST(GizmoDragsMoveOneAxisAsOneUndoStep) {
    auto editor = OpenPlayground();
    ASTRAL_CHECK(editor->Select(Find(*editor, "Crate1")));
    editor->FrameSelection();
    editor->tool = GizmoTool::Move;
    const Framework::Entity crate = editor->PreviewEntity(editor->Selection());
    const Vec3 pivot = editor->Preview().GetWorld(crate).translation;
    const float length = editor->Camera().WorldPerPixel(pivot, kHeight) * 90.0f;
    Vec2 origin{};
    Vec2 tip{};
    ASTRAL_CHECK(editor->Camera().Project(pivot, kWidth, kHeight, origin));
    ASTRAL_CHECK(editor->Camera().Project(pivot + Vec3{length, 0, 0}, kWidth, kHeight, tip));
    const Vec2 grab{(origin.x + tip.x) * 0.5f, (origin.y + tip.y) * 0.5f};
    ASTRAL_CHECK(editor->HitGizmo(grab.x, grab.y, kWidth, kHeight) == GizmoAxis::X);
    ASTRAL_CHECK(!editor->BeginDrag(5, 5, kWidth, kHeight)); // no handle there

    const Vec3 start = editor->Position();
    ASTRAL_CHECK(editor->BeginDrag(grab.x, grab.y, kWidth, kHeight));
    const Vec2 direction{tip.x - origin.x, tip.y - origin.y};
    for (int step = 1; step <= 4; ++step) {
        ASTRAL_CHECK(editor->Drag(grab.x + direction.x * 0.25f * step, grab.y + direction.y * 0.25f * step, kWidth, kHeight));
    }
    editor->EndDrag();
    const Vec3 moved = editor->Position();
    ASTRAL_CHECK_NEAR(moved.x - start.x, length, length * 0.05);
    ASTRAL_CHECK_NEAR(moved.y, start.y, 1.0e-4);
    ASTRAL_CHECK_NEAR(moved.z, start.z, 1.0e-4);
    ASTRAL_CHECK(editor->Undo() && Near(editor->Position(), start));
    ASTRAL_CHECK(!editor->CanUndo()); // the whole drag was one step

    // Rotate and scale handles edit their own component.
    std::string error;
    editor->tool = GizmoTool::Scale;
    ASTRAL_CHECK(editor->HitGizmo(origin.x, origin.y, kWidth, kHeight) == GizmoAxis::All);
    ASTRAL_CHECK(editor->BeginDrag(origin.x, origin.y, kWidth, kHeight));
    ASTRAL_CHECK(editor->Drag(origin.x + 45, origin.y - 45, kWidth, kHeight));
    editor->EndDrag();
    const Vec3 scale = editor->Scale();
    ASTRAL_CHECK(scale.x > 1.5f && std::fabs(scale.x - scale.y) < 1.0e-4f && std::fabs(scale.y - scale.z) < 1.0e-4f);
}

ASTRAL_TEST(InspectorShowsInheritedAndAuthoredValues) {
    auto editor = OpenPlayground();
    ASTRAL_CHECK(editor->Select(Find(*editor, "Crate1")));
    auto groups = editor->Inspector();
    const auto* body = Field(Group(groups, InspectorGroupKind::Component, "RigidBody"), "mass");
    ASTRAL_CHECK(body && body->value.AsNumber() == 3.0 && !body->authored); // from the Crate prefab
    ASTRAL_CHECK(!Group(groups, InspectorGroupKind::Component, "RigidBody")->removable);
    ASTRAL_CHECK(Group(groups, InspectorGroupKind::Material, "wood")); // the material it renders with
    const auto* shape = Field(Group(groups, InspectorGroupKind::Component, "Collider"), "shape");
    ASTRAL_CHECK(shape && shape->kind == Core::FieldKind::Enum && !shape->enumNames.empty());

    std::string error;
    ASTRAL_CHECK(editor->SetField(InspectorGroupKind::Component, "RigidBody", 0, "mass", Core::JsonValue(8), error));
    groups = editor->Inspector();
    body = Field(Group(groups, InspectorGroupKind::Component, "RigidBody"), "mass");
    ASTRAL_CHECK(body && body->value.AsNumber() == 8.0 && body->authored);
    ASTRAL_CHECK(editor->ResetField(InspectorGroupKind::Component, "RigidBody", 0, "mass", error));
    groups = editor->Inspector();
    body = Field(Group(groups, InspectorGroupKind::Component, "RigidBody"), "mass");
    ASTRAL_CHECK(body && body->value.AsNumber() == 3.0 && !body->authored);
    ASTRAL_CHECK(!editor->RemoveComponent("RigidBody", error) && error.find("prefab") != std::string::npos);

    // Behaviours are listed with their reflected properties.
    ASTRAL_CHECK(editor->Select(Find(*editor, "Player")));
    groups = editor->Inspector();
    const auto* speed = Field(Group(groups, InspectorGroupKind::Behaviour, "PlayerController"), "moveSpeed");
    ASTRAL_CHECK(speed && speed->value.AsNumber() == 6.0 && speed->authored);

    // The scene row edits settings, environment and every material.
    editor->SelectScene();
    groups = editor->Inspector();
    ASTRAL_CHECK(Group(groups, InspectorGroupKind::Settings, "Settings"));
    ASTRAL_CHECK(Field(Group(groups, InspectorGroupKind::Environment, "Environment"), "fogDensity"));
    ASTRAL_CHECK(Group(groups, InspectorGroupKind::Material, "grass") && Group(groups, InspectorGroupKind::Material, "water"));
}

ASTRAL_TEST(SaveKeepsTheAuthoredDocumentAndReopens) {
    const std::string root = CopyPlayground("astral-editor-save");
    auto editor = OpenPlayground(root);
    const std::string path = editor->ScenePath();
    std::string error;
    editor->SelectScene();
    ASTRAL_CHECK(editor->SetField(InspectorGroupKind::Material, "grass", 0, "baseColor", Vec3Json(1, 0, 0), error));
    ASTRAL_CHECK(editor->Save(error) && !editor->Dirty());

    Core::JsonValue saved;
    ASTRAL_CHECK(Core::ReadJsonFile(root + "/" + path, saved, error));
    ASTRAL_CHECK(saved == editor->Document());
    ASTRAL_CHECK(saved["materials"]["grass"]["baseColor"][0].AsNumber() == 1.0);
    bool prefabReferenceKept = false;
    for (const Core::JsonValue& entity : saved["entities"].Items()) prefabReferenceKept |= entity.String("prefab") == "Crate";
    ASTRAL_CHECK(prefabReferenceKept);
    ASTRAL_CHECK(!editor->ReloadIfChangedOnDisk()); // our own save is not a disk change

    auto reopened = OpenPlayground(root);
    ASTRAL_CHECK(reopened->Document() == editor->Document());
    ASTRAL_CHECK(!editor->SaveAs("../outside.scene.json", error)); // stays inside the content root
    ASTRAL_CHECK(editor->SaveAs("scenes/copy.scene.json", error) && editor->ScenePath() == "scenes/copy.scene.json");
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(DiskEditsReloadOnlyWhenTheDocumentIsClean) {
    const std::string root = CopyPlayground("astral-editor-reload");
    auto editor = OpenPlayground(root);
    const std::string file = root + "/" + editor->ScenePath();
    std::string error;
    Core::JsonValue json;
    ASTRAL_CHECK(Core::ReadJsonFile(file, json, error));
    Core::JsonValue* entities = json.Find("entities");
    Core::JsonValue added = Core::JsonValue::MakeObject();
    added.Set("name", "FromDisk");
    entities->Append(added);
    ASTRAL_CHECK(Core::WriteJsonFile(file, json, error));
    std::filesystem::last_write_time(file, std::filesystem::last_write_time(file) + std::chrono::seconds(5));
    ASTRAL_CHECK(editor->ReloadIfChangedOnDisk());
    NodePath fromDisk;
    ASTRAL_CHECK(editor->FindNode("FromDisk", fromDisk) && !editor->Dirty());

    // With unsaved edits the document wins and the user is told.
    ASTRAL_CHECK(editor->CreateEntity("empty", error));
    entities->Items().pop_back();
    ASTRAL_CHECK(Core::WriteJsonFile(file, json, error));
    std::filesystem::last_write_time(file, std::filesystem::last_write_time(file) + std::chrono::seconds(10));
    ASTRAL_CHECK(!editor->ReloadIfChangedOnDisk());
    ASTRAL_CHECK(editor->FindNode("FromDisk", fromDisk) && editor->Status().find("unsaved") != std::string::npos);
    std::filesystem::remove_all(root);
}

ASTRAL_TEST(PlayRunsTheEditedSceneAndRestartsIt) {
    auto editor = OpenPlayground();
    ASTRAL_CHECK(editor->Select(Find(*editor, "Player")));
    std::string error;
    const Vec3 edited = editor->Position() + Vec3{0, 0, 3};
    ASTRAL_CHECK(editor->SetTransform(edited, editor->EulerDegrees(), editor->Scale(), error));
    const Core::JsonValue document = editor->Document();

    ASTRAL_CHECK(editor->Play(error) && editor->Playing());
    Framework::GameHost& game = *editor->Game();
    auto player = [&game] { return game.World().GetWorld(game.World().Find("Player")).translation; };
    ASTRAL_CHECK(Near(player(), edited, 0.05f));
    Framework::HostInput forward;
    forward.keys.keys.set(Input::KeyFromName("W"));
    for (int i = 0; i < 30; ++i) game.Update(1.0f / 60.0f, forward); // the project's input map drives it
    ASTRAL_CHECK(Math::Distance(player(), edited) > 0.5f);
    ASTRAL_CHECK(game.World().Stats().behaviours > 0);

    // An unsaved scene reloads from its document (the Playground's win restart).
    game.World().Events().Queue(Framework::LoadSceneRequest{});
    game.Update(1.0f / 60.0f, {});
    ASTRAL_CHECK(game.Stats().sceneLoads == 2 && game.LastError().empty());
    ASTRAL_CHECK(Near(player(), edited, 0.05f));
    ASTRAL_CHECK(game.Commands().Execute("restart").find("unsaved") != std::string::npos);

    const Graphics::ImageRgba8& frame = editor->Render(kWidth, kHeight);
    ASTRAL_CHECK(frame.width == kWidth && frame.height == kHeight);
    editor->Stop();
    ASTRAL_CHECK(!editor->Playing() && editor->Document() == document);
}

ASTRAL_TEST(ViewportRendersTheSelectionAndGizmo) {
    auto editor = OpenPlayground();
    ASTRAL_CHECK(editor->Select(Find(*editor, "Crate1")));
    editor->FrameSelection();
    auto countColor = [](const Graphics::ImageRgba8& image, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        int count = 0;
        for (std::size_t i = 0; i + 3 < image.pixels.size(); i += 4) {
            count += image.pixels[i] == r && image.pixels[i + 1] == g && image.pixels[i + 2] == b;
        }
        return count;
    };
    editor->tool = GizmoTool::Move;
    const Graphics::ImageRgba8& image = editor->Render(kWidth, kHeight);
    ASTRAL_CHECK(image.width == kWidth && image.height == kHeight);
    ASTRAL_CHECK(countColor(image, 255, 160, 40) > 20); // selection box
    ASTRAL_CHECK(countColor(image, 235, 70, 70) > 20);  // X handle
    const Vec2 centre = SelectionPixel(*editor);
    ASTRAL_CHECK(centre.x > 0 && centre.x < kWidth && centre.y > 0 && centre.y < kHeight);
    editor->ClearSelection();
    ASTRAL_CHECK(countColor(editor->Render(kWidth, kHeight), 255, 160, 40) == 0);
}

ASTRAL_TEST(NewScenesAreValidAndPlayable) {
    Samples::RegisterPlaygroundBehaviours();
    SceneEditor editor(PlaygroundRoot(), 0);
    ASTRAL_CHECK(editor.ScenePath().empty() && !editor.Dirty() && editor.Outliner().size() == 4);
    std::string error;
    ASTRAL_CHECK(!editor.Save(error)); // needs a name first
    ASTRAL_CHECK(editor.Play(error));
    ASTRAL_CHECK(!editor.Game()->World().ActiveCamera().IsNull());
    editor.Stop();
    ASTRAL_CHECK(editor.AddMaterial("red", error) && !editor.AddMaterial("red", error));
    ASTRAL_CHECK(editor.CreateEntity("primitive:sphere", error));
    ASTRAL_CHECK(editor.SetField(InspectorGroupKind::Component, "MeshRenderer", 0, "material", Core::JsonValue("red"), error));
    ASTRAL_CHECK(!editor.SetField(InspectorGroupKind::Component, "MeshRenderer", 0, "material", Core::JsonValue("nope"), error));
    ASTRAL_CHECK(editor.AddBehaviour("Spinner", error) && !editor.AddBehaviour("NoSuchBehaviour", error));
    ASTRAL_CHECK(editor.Preview().Get<Framework::MeshRenderer>(editor.PreviewEntity(editor.Selection())));
}

ASTRAL_TEST(PreviewNeverAttachesOrCreatesAuthoredAndInheritedBehaviours) {
    previewProbeCounts = {};
    Framework::BehaviourRegistry::Global().Register<EditorPreviewProbe>("EditorPreviewProbe");
    const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path root = std::filesystem::temp_directory_path()
        / ("astral-editor-preview-probe-" + std::to_string(suffix));
    std::filesystem::create_directories(root / "prefabs");
    Core::JsonValue prefab, scene;
    std::string error;
    ASTRAL_CHECK(Core::ParseJson(R"({"entity":{"behaviours":[{"type":"EditorPreviewProbe"}],
        "children":[{"name":"File child","behaviours":[{"type":"EditorPreviewProbe"}]}]}})", prefab, error));
    ASTRAL_CHECK(Core::WriteJsonFile((root / "prefabs/probe.json").string(), prefab, error));
    ASTRAL_CHECK(Core::ParseJson(R"({"format":"astral-scene","version":1,
        "prefabs":{"InlineProbe":{"behaviours":[{"type":"EditorPreviewProbe"}],
            "children":[{"name":"Inline child","behaviours":[{"type":"EditorPreviewProbe"}]}]}},
        "entities":[{"name":"Inline instance","prefab":"InlineProbe"},
            {"name":"File instance","prefab":"prefabs/probe.json"},
            {"name":"Direct","behaviours":[{"type":"EditorPreviewProbe"}]}]})", scene, error));
    ASTRAL_CHECK(Core::WriteJsonFile((root / "probe.scene.json").string(), scene, error));
    {
        SceneEditor editor(root.string(), 0);
        ASTRAL_CHECK(editor.OpenScene("probe.scene.json", error));
        const auto checkPreview = [&] {
            ASTRAL_CHECK(editor.Preview().Registry().Count<Framework::Behaviours>() == 0);
            ASTRAL_CHECK(previewProbeCounts.created == 0 && previewProbeCounts.destroyed == 0);
        };
        checkPreview();
        editor.Render(64, 64);
        ASTRAL_CHECK(editor.Select(Find(editor, "Direct")));
        ASTRAL_CHECK(editor.Rename("Edited", error));
        checkPreview();
        ASTRAL_CHECK(editor.Undo());
        checkPreview();
        ASTRAL_CHECK(editor.Redo());
        checkPreview();
        ASTRAL_CHECK(previewProbeCounts.constructed > 0); // parsing invokes validation constructors
        const auto document = editor.Document();
        ASTRAL_CHECK(editor.Play(error));
        ASTRAL_CHECK(previewProbeCounts.created == 5);
        ASTRAL_CHECK(editor.Game()->World().Registry().Count<Framework::Behaviours>() == 5);
        ASTRAL_CHECK(editor.Preview().Registry().Count<Framework::Behaviours>() == 0);
        editor.Stop();
        ASTRAL_CHECK(previewProbeCounts.destroyed == 5 && editor.Document() == document);
    }
    std::filesystem::remove_all(root); // unique directory created by this fixture
}

ASTRAL_TEST(PlayingRejectsAuthoringCommandsWithoutChangingPreviewOrHistory) {
    EditorFiles files;
    Framework::BehaviourRegistry::Global().Register<EditorPreviewProbe>("EditorPreviewProbe");
    SceneEditor editor(files.root.string(), 0);
    std::string error;
    ASTRAL_CHECK(editor.Select(Find(editor, "Ground")));
    ASTRAL_CHECK(editor.AddBehaviour("EditorPreviewProbe", error));
    ASTRAL_CHECK(editor.AddMaterial("Authored", error));
    ASTRAL_CHECK(editor.SaveAs("author.scene.json", error));
    ASTRAL_CHECK(editor.Rename("Authored ground", error));
    ASTRAL_CHECK(editor.Play(error));
    Framework::GameHost* const game = editor.Game();
    const AuthorState before(editor);
    const std::vector<std::function<bool()>> commands = {
        [&] { return editor.CreateEntity("primitive:cube", error); },
        [&] { return editor.Rename("Must not persist", error); },
        [&] { return editor.Duplicate(error); },
        [&] { return editor.Delete(error); },
        [&] { return editor.Reparent(Find(editor, "Sun"), error); },
        [&] { return editor.SetField(InspectorGroupKind::Entity, "", 0, "name", Core::JsonValue("Changed"), error); },
        [&] { return editor.ResetField(InspectorGroupKind::Entity, "", 0, "name", error); },
        [&] { return editor.AddComponent("Light", error); },
        [&] { return editor.RemoveComponent("MeshRenderer", error); },
        [&] { return editor.AddBehaviour("EditorPreviewProbe", error); },
        [&] { return editor.RemoveBehaviour(0, error); },
        [&] { return editor.AddMaterial("DuringPlay", error); }
    };
    for (const auto& command : commands) {
        error.clear();
        ASTRAL_CHECK(!command());
        ASTRAL_CHECK(error.find("playing") != std::string::npos);
        before.Check(editor);
        ASTRAL_CHECK(editor.Playing() && editor.Game() == game);
    }
    // Runtime edits really happen, but Stop cannot copy them into authoring.
    const auto runtimeEntity = game->World().Find("Authored ground");
    Math::TRS moved = game->World().GetLocal(runtimeEntity);
    moved.translation.x += 9;
    game->World().SetLocal(runtimeEntity, moved);
    ASTRAL_CHECK(Near(game->World().GetLocal(runtimeEntity).translation, moved.translation));
    editor.Stop();
    before.Check(editor);
    ASTRAL_CHECK(editor.Rename("Allowed after Stop", error));
}

ASTRAL_TEST(PlayingRejectsDirectTransformWithoutChangingAuthoredState) {
    EditorFiles files;
    SceneEditor editor(files.root.string(), 0);
    std::string error;
    ASTRAL_CHECK(editor.Select(Find(editor, "Ground")) && editor.Play(error));
    const AuthorState before(editor);
    ASTRAL_CHECK(!editor.SetTransform({7, 2, 1}, {0, 45, 0}, {2, 2, 2}, error));
    ASTRAL_CHECK(error.find("playing") != std::string::npos);
    before.Check(editor);
    editor.Stop();
    ASTRAL_CHECK(editor.SetTransform({7, 2, 1}, {0, 45, 0}, {2, 2, 2}, error));
}

ASTRAL_TEST(PlayingPreservesUndoAndRedoUntilStopped) {
    for (const bool redo : {false, true}) {
        EditorFiles files;
        SceneEditor editor(files.root.string(), 0);
        std::string error;
        ASTRAL_CHECK(editor.Select(Find(editor, "Ground")));
        ASTRAL_CHECK(editor.Rename("Edited", error));
        if (redo) ASTRAL_CHECK(editor.Undo());
        ASTRAL_CHECK(editor.Play(error));
        const AuthorState before(editor);
        ASTRAL_CHECK(!(redo ? editor.Redo() : editor.Undo()));
        before.Check(editor);
        editor.Stop();
        ASTRAL_CHECK(redo ? editor.Redo() : editor.Undo());
    }
}

ASTRAL_TEST(SaveAsRejectsCanonicalDirectoryEscapeWithoutWritingOutside) {
    EditorFiles files;
    const auto link = files.root / "linked";
    if (!EditorDirectoryLink(files.outside, link)) {
        std::printf("[ SKIP ] directory link unavailable; canonical SaveAs escape fixture not executed\n");
        return;
    }
    SceneEditor editor(files.root.string(), 0);
    std::string error;
    ASTRAL_CHECK(editor.Select(Find(editor, "Ground")) && editor.Rename("Unsaved marker", error));
    const AuthorState before(editor);
    const bool saved = editor.SaveAs("linked/escape.scene.json", error);
    std::printf("SaveAs returned %d; disposable outside file exists=%d\n", saved,
        std::filesystem::exists(files.outside / "escape.scene.json"));
    ASTRAL_CHECK(!saved && !error.empty());
    ASTRAL_CHECK(!std::filesystem::exists(files.outside / "escape.scene.json"));
    ASTRAL_CHECK(!std::filesystem::exists(files.outside / "escape.scene.json.tmp"));
    before.Check(editor);
    std::filesystem::remove(link); // remove the owned link before recursive fixture cleanup
}

ASTRAL_TEST(SaveAsRejectsCanonicalTemporaryFileEscape) {
    EditorFiles files;
    const auto outside = files.outside / "marker.json";
    std::ofstream(outside) << "original disposable marker";
    std::error_code linkError;
    std::filesystem::create_symlink(outside, files.root / "saved.scene.json.tmp", linkError);
    if (linkError) {
        std::printf("[ SKIP ] file symlink unavailable: %s\n", linkError.message().c_str());
        return;
    }
    SceneEditor editor(files.root.string(), 0);
    const AuthorState before(editor);
    std::string error;
    ASTRAL_CHECK(!editor.SaveAs("saved.scene.json", error) && !error.empty());
    ASTRAL_CHECK(FileText(outside) == "original disposable marker");
    ASTRAL_CHECK(!std::filesystem::exists(files.root / "saved.scene.json"));
    before.Check(editor);
}

ASTRAL_TEST(SaveAsAcceptsInRootNormalizationAndRejectsInvalidPathsTransactionally) {
    EditorFiles files;
    SceneEditor editor(files.root.string(), 0);
    std::string error;
    ASTRAL_CHECK(editor.SaveAs("..hidden.scene.json", error)); // a filename, not a parent segment
    ASTRAL_CHECK(editor.SaveAs("scenes/../copy.scene.json", error));
    ASTRAL_CHECK(editor.ScenePath() == "copy.scene.json");
    Core::JsonValue saved;
    ASTRAL_CHECK(Core::ReadJsonFile((files.root / "copy.scene.json").string(), saved, error));
    ASTRAL_CHECK(saved == editor.Document());
    SceneEditor reopened(files.root.string(), 0);
    ASTRAL_CHECK(reopened.OpenScene(editor.ScenePath(), error) && reopened.Document() == saved);
    ASTRAL_CHECK(!editor.ReloadIfChangedOnDisk());
    ASTRAL_CHECK(editor.SaveAs("nested\\mixed.scene.json", error));
    ASTRAL_CHECK(editor.ScenePath() == "nested/mixed.scene.json");
    ASTRAL_CHECK(editor.Select(Find(editor, "Ground")) && editor.Rename("Still unsaved", error));
    const AuthorState before(editor);
    const std::vector<std::string> invalid = {"", "../outside.json", "..\\outside.json", "/absolute.json",
        "C:relative.json", "C:/absolute.json", "\\\\server\\share\\file.json", "file:scene.json",
        std::string("nul.scene.json") + '\0' + "ignored"};
    for (const auto& path : invalid) {
        error.clear();
        ASTRAL_CHECK(!editor.SaveAs(path, error) && !error.empty());
        before.Check(editor);
    }
#if defined(_WIN32)
    ASTRAL_CHECK(!editor.SaveAs(std::string(1, static_cast<char>(0xff)) + ".json", error));
    before.Check(editor); // malformed UTF-8 is a diagnostic, never a path-conversion exception
#endif
}

ASTRAL_TEST(FailedSavePreservesDocumentSelectionHistoryAndLastSavedBytes) {
    EditorFiles files;
    SceneEditor editor(files.root.string(), 0);
    std::string error;
    ASTRAL_CHECK(editor.SaveAs("author.scene.json", error));
    const auto lastSaved = FileText(files.root / "author.scene.json");
    ASTRAL_CHECK(editor.Select(Find(editor, "Ground")));
    ASTRAL_CHECK(editor.Rename("Unsaved", error));
    ASTRAL_CHECK(editor.SetTransform({4, 0, 0}, {}, {1, 1, 1}, error));
    ASTRAL_CHECK(editor.Undo()); // preserve a real redo entry as well
    const AuthorState before(editor);
    std::filesystem::create_directory(files.root / "author.scene.json.tmp");
    ASTRAL_CHECK(!editor.Save(error) && !error.empty());
    before.Check(editor);
    ASTRAL_CHECK(FileText(files.root / "author.scene.json") == lastSaved);
    std::filesystem::create_directory(files.root / "blocked.scene.json");
    ASTRAL_CHECK(!editor.SaveAs("blocked.scene.json", error)); // rename failure after temp write
    before.Check(editor);
    ASTRAL_CHECK(FileText(files.root / "author.scene.json") == lastSaved);
    ASTRAL_CHECK(!std::filesystem::exists(files.root / "blocked.scene.json.tmp"));
    ASTRAL_CHECK(editor.Redo());
}

ASTRAL_TEST(SaveAsAllowsInRootDirectoryLinksAndKeepsAuthoredReferences) {
    EditorFiles files;
    const auto destination = files.root / "scenes";
    std::filesystem::create_directory(destination);
    const auto link = files.root / "linked";
    if (!EditorDirectoryLink(destination, link)) {
        std::printf("[ SKIP ] directory link unavailable; in-root SaveAs control not executed\n");
        return;
    }
    SceneEditor editor(files.root.string(), 0);
    std::string error;
    ASTRAL_CHECK(editor.Select(Find(editor, "Ground")) && editor.Rename("Authored through link", error));
    const auto document = editor.Document();
    ASTRAL_CHECK(editor.SaveAs("linked/author.scene.json", error));
    ASTRAL_CHECK(editor.Document() == document && !editor.Dirty() && editor.CanUndo());
    Core::JsonValue saved;
    ASTRAL_CHECK(Core::ReadJsonFile((destination / "author.scene.json").string(), saved, error));
    ASTRAL_CHECK(saved == document);
    SceneEditor reopened(files.root.string(), 0);
    ASTRAL_CHECK(reopened.OpenScene("linked/author.scene.json", error) && reopened.Document() == document);
    std::filesystem::remove(link);
    const auto deep = destination / "deep";
    std::filesystem::create_directory(deep);
    ASTRAL_CHECK(EditorDirectoryLink(deep, link));
    ASTRAL_CHECK(editor.SaveAs("linked/../physical-parent.scene.json", error));
    std::printf("physical-parent save: reported=%s expected-dir-file=%d root-file=%d weakly-canonical=%s resolved-link-parent=%s\n",
        editor.ScenePath().c_str(), static_cast<int>(std::filesystem::exists(destination / "physical-parent.scene.json")),
        static_cast<int>(std::filesystem::exists(files.root / "physical-parent.scene.json")),
        std::filesystem::weakly_canonical(link / "../physical-parent.scene.json").generic_string().c_str(),
        std::filesystem::canonical(link).parent_path().generic_string().c_str());
    ASTRAL_CHECK(editor.ScenePath() == "scenes/physical-parent.scene.json");
    ASTRAL_CHECK(std::filesystem::exists(destination / "physical-parent.scene.json"));
    ASTRAL_CHECK(!std::filesystem::exists(files.root / "physical-parent.scene.json"));
    std::filesystem::remove(link);
}

ASTRAL_TEST(FailedSaveRejectsAliasedTemporaryFileAndPreservesSavedBytes) {
    EditorFiles files;
    SceneEditor editor(files.root.string(), 0);
    std::string error;
    ASTRAL_CHECK(editor.SaveAs("author.scene.json", error));
    const auto lastSaved = FileText(files.root / "author.scene.json");
    ASTRAL_CHECK(editor.Select(Find(editor, "Ground")) && editor.Rename("Unsaved hard-link marker", error));
    std::error_code linkError;
    std::filesystem::create_hard_link(files.root / "author.scene.json",
        files.root / "blocked.scene.json.tmp", linkError);
    if (linkError) {
        std::printf("[ SKIP ] hard-link fixture unavailable: %s\n", linkError.message().c_str());
        return;
    }
    ASTRAL_CHECK(std::filesystem::hard_link_count(files.root / "blocked.scene.json.tmp") == 2);
    std::filesystem::create_directory(files.root / "blocked.scene.json"); // force atomic rename failure
    const AuthorState before(editor);
    ASTRAL_CHECK(!editor.SaveAs("blocked.scene.json", error) && !error.empty());
    before.Check(editor);
    ASTRAL_CHECK(FileText(files.root / "author.scene.json") == lastSaved);
    ASTRAL_CHECK(std::filesystem::is_directory(files.root / "blocked.scene.json"));
}

ASTRAL_TEST_MAIN("EngineEditorTests")
