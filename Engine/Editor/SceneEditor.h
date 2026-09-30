#pragma once

// Scene editor core: the platform-independent model behind AstralEditor (the
// role of Unity's Scene view, Hierarchy and Inspector, and UE's level editor).
//
// The authored scene JSON is the document. Every edit is applied to a copy,
// validated with SceneSerializer::Parse and only then committed, so a bad edit
// never corrupts the scene (the error names the JSON path). Saving writes the
// document back as authored: prefab references, glTF "model" entries and key
// order survive, which re-serialising a live world would flatten.
//
// A preview GameWorld is instantiated from each committed document with its
// behaviours stripped (edit mode runs no scripts, like Unity without
// ExecuteInEditMode) and rendered from an orbiting editor camera. Selection is
// by authored node path; entities that prefabs or models add select their
// authored ancestor. Play() runs the current, possibly unsaved, document in a
// GameHost with the project's input map; Stop() returns to the edit state.

#include "Engine/Assets/AssetManager.h"
#include "Engine/Core/JobSystem.h"
#include "Engine/Core/Json.h"
#include "Engine/Core/Reflection.h"
#include "Engine/Editor/EditorCamera.h"
#include "Engine/Framework/GameHost.h"
#include "Engine/Framework/GameWorld.h"
#include "Engine/Framework/Scene.h"
#include "Engine/Graphics/RenderTarget.h"
#include "Engine/Graphics/SceneRenderer.h"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Astral::Editor {

// Index path of an authored entity: entities[p0].children[p1]...
using NodePath = std::vector<std::size_t>;

enum class SelectionKind { None, Scene, Entity };

struct OutlinerRow {
    SelectionKind kind{SelectionKind::Entity};
    NodePath path;
    int depth{};
    std::string label;
};

enum class InspectorGroupKind { Entity, Transform, Component, Behaviour, Settings, Environment, Material };

struct InspectorField {
    std::string name;
    Core::FieldKind kind{Core::FieldKind::Float};
    std::vector<std::string> enumNames;
    double minimum{};
    double maximum{};
    std::string tooltip;
    Core::JsonValue value; // effective value (authored, inherited from a prefab, or the default)
    bool authored{};       // written on this node
};

struct InspectorGroup {
    InspectorGroupKind kind{InspectorGroupKind::Component};
    std::string name;      // component or behaviour type, material name
    std::size_t index{};   // behaviour index on the node
    bool removable{};
    std::vector<InspectorField> fields;
};

enum class GizmoTool { Select, Move, Rotate, Scale };
enum class GizmoAxis { None, X, Y, Z, All };

// Scene files are saved with this many decimals (keeps diffs readable).
constexpr double kSavedPrecision = 1.0e-4;

class SceneEditor {
public:
    // `contentRoot` is the folder scene, project and asset paths are relative to.
    explicit SceneEditor(std::string contentRoot = ".", int workerThreads = -1);
    ~SceneEditor();
    SceneEditor(const SceneEditor&) = delete;
    SceneEditor& operator=(const SceneEditor&) = delete;

    // ------------------------------------------------------------ documents
    // Opens a project file (its input map is used by Play) and its startup scene.
    bool OpenProject(const std::string& path, std::string& error);
    bool OpenScene(const std::string& path, std::string& error);
    // An unsaved scene with a sun, a ground plane and a camera.
    void NewScene();
    bool Save(std::string& error);
    bool SaveAs(const std::string& path, std::string& error);
    const std::string& ContentRoot() const { return contentRoot_; }
    const std::string& ScenePath() const { return scenePath_; }
    const std::string& ProjectPath() const { return projectPath_; }
    bool Dirty() const { return dirty_; }
    const Core::JsonValue& Document() const { return json_; }
    // Re-reads the scene file when it changed on disk and the document has no
    // unsaved edits (hot reload for scenes edited in a text editor).
    bool ReloadIfChangedOnDisk();

    // ------------------------------------------------------------ hierarchy
    std::vector<OutlinerRow> Outliner() const;
    SelectionKind Selected() const { return selectionKind_; }
    const NodePath& Selection() const { return selection_; }
    bool Select(const NodePath& path);
    void SelectScene();
    void ClearSelection();
    bool FindNode(const std::string& name, NodePath& out) const;
    std::string NodeName(const NodePath& path) const;
    // Preview entity of an authored node (null when there is none).
    Framework::Entity PreviewEntity(const NodePath& path) const;
    const Framework::GameWorld& Preview() const { return *preview_; }

    // ------------------------------------------------------------ edits (undoable)
    bool Rename(const std::string& name, std::string& error);
    Math::Vec3 Position() const;
    Math::Vec3 EulerDegrees() const;
    Math::Vec3 Scale() const;
    bool SetTransform(Math::Vec3 position, Math::Vec3 eulerDegrees, Math::Vec3 scale, std::string& error);
    bool SetField(InspectorGroupKind kind, const std::string& group, std::size_t index, const std::string& field,
        const Core::JsonValue& value, std::string& error);
    // Removes an authored field so the prefab's or default value applies again.
    bool ResetField(InspectorGroupKind kind, const std::string& group, std::size_t index, const std::string& field,
        std::string& error);
    bool AddComponent(const std::string& type, std::string& error);
    bool RemoveComponent(const std::string& type, std::string& error);
    bool AddBehaviour(const std::string& type, std::string& error);
    bool RemoveBehaviour(std::size_t index, std::string& error);
    bool AddMaterial(const std::string& name, std::string& error);
    // Creates a root entity at the camera focus and selects it. Kinds: "empty",
    // "primitive:cube|sphere|plane|capsule|cylinder", "light:point",
    // "light:directional", "camera", "prefab:<name>", "model:<path.glb>".
    bool CreateEntity(const std::string& kind, std::string& error);
    bool Duplicate(std::string& error);
    bool Delete(std::string& error);
    // Moves the selection under `parent` (empty = to the roots), keeping its path valid.
    bool Reparent(const NodePath& parent, std::string& error);
    bool CanUndo() const { return !undo_.empty(); }
    bool CanRedo() const { return !redo_.empty(); }
    bool Undo();
    bool Redo();

    std::vector<InspectorGroup> Inspector() const;
    // Component types the selection does not have yet.
    std::vector<std::string> AddableComponents() const;
    std::vector<std::string> BehaviourTypes() const;
    // Things CreateEntity accepts: primitives, lights, camera, the scene's
    // prefabs and the glTF models under the content root.
    std::vector<std::string> CreatableKinds() const;

    // ------------------------------------------------------------ viewport
    EditorCamera& Camera() { return camera_; }
    const EditorCamera& Camera() const { return camera_; }
    GizmoTool tool{GizmoTool::Move};
    bool snap{false};   // move 0.5 m, rotate 15 degrees, scale 0.1
    bool showGrid{true};
    // Renders the preview (or the running game while playing) with the grid,
    // light and camera icons, the selection box and the gizmo on top.
    const Graphics::ImageRgba8& Render(int width, int height);
    // Selects what is under the pixel (or the scene when nothing is). True if the selection changed.
    bool Pick(float x, float y, int width, int height);
    GizmoAxis HitGizmo(float x, float y, int width, int height) const;
    void SetHoverAxis(GizmoAxis axis) { hoverAxis_ = axis; }
    // One undo step per drag; false when no handle is under the pixel.
    bool BeginDrag(float x, float y, int width, int height);
    bool Drag(float x, float y, int width, int height);
    void EndDrag();
    bool Dragging() const { return drag_.active; }
    void FrameSelection();
    Math::AABB SelectionBounds() const;
    const Graphics::RenderStats& RenderStats() const { return renderer_.Stats(); }

    // ------------------------------------------------------------ play in editor
    bool Play(std::string& error);
    void Stop();
    bool Playing() const { return game_ != nullptr; }
    Framework::GameHost* Game() { return game_.get(); }

    Assets::AssetManager& Assets() { return *assets_; }
    // Last status or error message for the status bar.
    const std::string& Status() const { return status_; }

private:
    struct Snapshot {
        Core::JsonValue json;
        SelectionKind kind{};
        NodePath selection;
    };
    struct DragState {
        bool active{};
        bool changed{};
        GizmoAxis axis{GizmoAxis::None};
        Math::Vec2 start{};
        Math::Vec2 screenAxis{}; // unit direction of the handle on screen
        float pixelsPerUnit{1.0f};
        Math::Vec3 position{};
        Math::Vec3 euler{};
        Math::Vec3 scale{};
    };

    bool LoadDocument(Core::JsonValue json, const std::string& path, std::string& error);
    // Parses `next`; on success records an undo step and makes it current.
    bool Commit(Core::JsonValue next, SelectionKind kind, NodePath selection, std::string& error);
    void PushUndo();
    void Rebuild();
    void MapNodes(const Core::JsonValue& node, Framework::Entity entity, NodePath& path);
    Core::JsonValue* Node(Core::JsonValue& json, const NodePath& path) const;
    const Core::JsonValue* Node(const Core::JsonValue& json, const NodePath& path) const;
    const Framework::EntitySpec* Spec(const NodePath& path) const;
    Core::JsonValue* GroupObject(Core::JsonValue& json, InspectorGroupKind kind, const std::string& group,
        std::size_t index, bool create) const;
    void WriteTransform(Core::JsonValue& node, Math::Vec3 position, Math::Vec3 euler, Math::Vec3 scale) const;
    void ApplyPreviewTransform(Math::Vec3 position, Math::Vec3 euler, Math::Vec3 scale);
    Math::AABB EntityBounds(Framework::Entity entity) const;
    void GizmoFrame(Math::Vec3& pivot, Math::Vec3 axes[3]) const;
    float GizmoLength(Math::Vec3 pivot, int height) const;
    void DrawOverlay(Graphics::ImageRgba8& image, int width, int height) const;
    void DrawGrid(Graphics::ImageRgba8& image, int width, int height) const;
    void DrawLine3D(Graphics::ImageRgba8& image, Math::Vec3 a, Math::Vec3 b, Graphics::Rgba8 color, int width,
        int height) const;
    std::string UniqueName(const std::string& base) const;

    std::string contentRoot_;
    std::unique_ptr<Core::JobSystem> jobs_;
    std::unique_ptr<Assets::AssetManager> assets_;
    Graphics::SceneRenderer renderer_;
    Graphics::RenderTarget target_;

    Core::JsonValue json_;
    std::string scenePath_;
    std::string projectPath_;
    Framework::ProjectDesc project_;
    bool hasProject_{};
    bool dirty_{};
    std::uint32_t diskVersion_{};
    std::vector<Snapshot> undo_;
    std::vector<Snapshot> redo_;

    Framework::SceneDocument parsed_;
    std::unique_ptr<Framework::GameWorld> preview_;
    std::map<NodePath, Framework::Entity> nodeEntities_;
    std::map<std::uint64_t, NodePath> entityNodes_; // packed entity -> authored node
    Framework::Entity editorCamera_{};

    SelectionKind selectionKind_{SelectionKind::None};
    NodePath selection_;
    EditorCamera camera_;
    GizmoAxis hoverAxis_{GizmoAxis::None};
    DragState drag_;

    std::unique_ptr<Framework::GameHost> game_;
    std::string status_;
};

} // namespace Astral::Editor
