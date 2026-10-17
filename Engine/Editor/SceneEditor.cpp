#include "Engine/Editor/SceneEditor.h"

#include "Engine/Graphics/Canvas.h"
#include "Engine/World/Components.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <functional>
#include <limits>
#include <set>
#include <system_error>
#include <utility>

namespace Astral::Editor {

using Core::FieldKind;
using Core::JsonValue;
using Framework::Entity;
using Framework::EntitySpec;
using Math::Vec2;
using Math::Vec3;

namespace {

constexpr std::size_t kMaxUndo = 200;
constexpr const char* kEditorCameraName = "__AstralEditorCamera";
constexpr const char* kComponentTypes[] = {"MeshRenderer", "Light", "Camera", "Collider", "RigidBody",
    "CharacterMover", "AudioSource", "AudioListener", "ParticleSystem", "Animator"};

double Rounded(float value) {
    const double rounded = std::round(static_cast<double>(value) / kSavedPrecision) * kSavedPrecision;
    return rounded == 0.0 ? 0.0 : rounded; // no "-0"
}

JsonValue Vec3Json(Vec3 v) {
    JsonValue array = JsonValue::MakeArray();
    array.Append(Rounded(v.x));
    array.Append(Rounded(v.y));
    array.Append(Rounded(v.z));
    return array;
}

bool ReadVec3(const JsonValue& json, Vec3& out) {
    if (!json.IsArray() || json.Size() != 3) return false;
    for (std::size_t i = 0; i < 3; ++i) {
        if (!json[i].IsNumber()) return false;
    }
    out = {json[0].AsFloat(), json[1].AsFloat(), json[2].AsFloat()};
    return true;
}

bool Fail(std::string& error, std::string message) {
    error = std::move(message);
    return false;
}

bool NearlyEqual(Vec3 a, Vec3 b) {
    return std::fabs(a.x - b.x) < 1.0e-5f && std::fabs(a.y - b.y) < 1.0e-5f && std::fabs(a.z - b.z) < 1.0e-5f;
}

std::uint64_t Pack(Entity entity) {
    return (static_cast<std::uint64_t>(entity.generation) << 32) | entity.index;
}

bool IsComponentType(const std::string& type) {
    for (const char* known : kComponentTypes) {
        if (type == known) return true;
    }
    return false;
}

void StripBehaviours(EntitySpec& spec) {
    spec.behaviours.clear();
    for (EntitySpec& child : spec.children) StripBehaviours(child);
}

// Children array of an entity node or the scene's entities array (created when missing).
JsonValue& ChildArray(JsonValue& owner, const char* key) {
    if (JsonValue* array = owner.Find(key); array && array->IsArray()) return *array;
    return owner.Set(key, JsonValue::MakeArray());
}

// Effective values of a reflected type: defaults overlaid with `authored`.
std::vector<InspectorField> ReflectedFields(const Core::TypeInfo& info, const JsonValue& effective,
    const JsonValue* authored) {
    void* object = info.create();
    std::string ignored;
    if (effective.IsObject()) info.FromJson(object, effective, ignored, false);
    std::vector<InspectorField> fields;
    for (const Core::FieldInfo& field : info.fields) {
        InspectorField out;
        out.name = field.name;
        out.kind = field.kind;
        out.enumNames = field.enumNames;
        out.minimum = field.minimum;
        out.maximum = field.maximum;
        out.tooltip = field.tooltip;
        out.value = field.read(object);
        out.authored = authored && authored->IsObject() && authored->Has(field.name);
        fields.push_back(std::move(out));
    }
    info.destroy(object);
    return fields;
}

InspectorField MakeField(const std::string& name, FieldKind kind, JsonValue value, bool authored,
    double minimum = -std::numeric_limits<double>::infinity(),
    double maximum = std::numeric_limits<double>::infinity()) {
    InspectorField field;
    field.name = name;
    field.kind = kind;
    field.value = std::move(value);
    field.authored = authored;
    field.minimum = minimum;
    field.maximum = maximum;
    return field;
}

// Liang-Barsky clip of a segment to a rectangle; false when nothing is left.
// On success a and b are the clipped ends and [t0, t1] their parameters on the input.
bool ClipSegment(Vec2& a, Vec2& b, float minX, float minY, float maxX, float maxY, float* clippedT0 = nullptr,
    float* clippedT1 = nullptr) {
    float t0 = 0.0f;
    float t1 = 1.0f;
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float p[4] = {-dx, dx, -dy, dy};
    const float q[4] = {a.x - minX, maxX - a.x, a.y - minY, maxY - a.y};
    for (int i = 0; i < 4; ++i) {
        if (std::fabs(p[i]) < 1.0e-9f) {
            if (q[i] < 0.0f) return false;
            continue;
        }
        const float t = q[i] / p[i];
        if (p[i] < 0.0f) {
            if (t > t1) return false;
            t0 = std::max(t0, t);
        } else {
            if (t < t0) return false;
            t1 = std::min(t1, t);
        }
    }
    const Vec2 start = a;
    a = {start.x + dx * t0, start.y + dy * t0};
    b = {start.x + dx * t1, start.y + dy * t1};
    if (clippedT0) *clippedT0 = t0;
    if (clippedT1) *clippedT1 = t1;
    return true;
}

float DistanceToSegment(Vec2 p, Vec2 a, Vec2 b) {
    const Vec2 ab{b.x - a.x, b.y - a.y};
    const float lengthSquared = ab.x * ab.x + ab.y * ab.y;
    float t = lengthSquared > 1.0e-6f ? ((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lengthSquared : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);
    const float dx = p.x - (a.x + ab.x * t);
    const float dy = p.y - (a.y + ab.y * t);
    return std::sqrt(dx * dx + dy * dy);
}

// Points of a circle of `radius` around `center` in the plane perpendicular to `axis`.
std::vector<Vec3> Ring(Vec3 center, Vec3 axis, float radius, int segments = 48) {
    const Vec3 helper = std::fabs(axis.y) < 0.9f ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
    const Vec3 u = Math::Normalize(Math::Cross(axis, helper), {1.0f, 0.0f, 0.0f});
    const Vec3 v = Math::Cross(axis, u);
    std::vector<Vec3> points;
    for (int i = 0; i <= segments; ++i) {
        const float angle = Math::kTwoPi * static_cast<float>(i) / static_cast<float>(segments);
        points.push_back(center + u * (std::cos(angle) * radius) + v * (std::sin(angle) * radius));
    }
    return points;
}

const Graphics::Rgba8 kAxisColors[3] = {{235, 70, 70, 255}, {90, 215, 90, 255}, {80, 140, 255, 255}};
const Graphics::Rgba8 kHighlight{255, 225, 60, 255};
const Graphics::Rgba8 kSelectionColor{255, 160, 40, 255};

} // namespace

// ================================================================ lifetime

SceneEditor::SceneEditor(std::string contentRoot, int workerThreads)
    : contentRoot_(std::move(contentRoot)),
      jobs_(workerThreads != 0 ? std::make_unique<Core::JobSystem>(workerThreads) : nullptr),
      assets_(std::make_unique<Assets::AssetManager>(contentRoot_, jobs_.get())),
      renderer_(jobs_.get()) {
    assets_->RegisterDefaultLoaders();
    NewScene();
}

SceneEditor::~SceneEditor() {
    game_.reset();
    preview_.reset();
}

// ================================================================ documents

void SceneEditor::NewScene() {
    std::string error;
    JsonValue json;
    Core::ParseJson(R"({
  "format": "astral-scene",
  "version": 1,
  "environment": {},
  "materials": {"ground": {"shading": "Toon", "baseColor": [0.42, 0.6, 0.4]}},
  "entities": [
    {"name": "Sun", "transform": {"rotation": {"euler": [50, 30, 0]}},
     "components": {"Light": {"type": "Directional", "intensity": 2.5}}},
    {"name": "Ground", "transform": {"scale": [40, 1, 40]},
     "components": {"MeshRenderer": {"mesh": "primitive:plane", "material": "ground"}, "Collider": {"shape": "Mesh"}}},
    {"name": "Main Camera", "transform": {"position": [0, 3, -10], "rotation": {"euler": [12, 0, 0]}},
     "components": {"Camera": {}, "AudioListener": {}}}
  ]
})", json, error);
    LoadDocument(std::move(json), {}, error);
    projectPath_.clear();
    hasProject_ = false;
    status_ = "New scene";
}

bool SceneEditor::LoadDocument(JsonValue json, const std::string& path, std::string& error) {
    Framework::SceneDocument document;
    if (!Framework::SceneSerializer(assets_.get()).Parse(json, document, error)) {
        if (!path.empty()) error = path + ": " + error;
        return false;
    }
    Stop();
    json_ = std::move(json);
    parsed_ = std::move(document);
    scenePath_ = path;
    savedJson_ = json_;
    undo_.clear();
    redo_.clear();
    selectionKind_ = SelectionKind::None;
    selection_.clear();
    drag_ = {};
    Rebuild();
    FrameSelection();
    status_ = path.empty() ? "Untitled scene" : "Opened " + path;
    return true;
}

bool SceneEditor::OpenScene(const std::string& path, std::string& error) {
    const auto handle = assets_->Load<JsonValue>(path);
    if (!handle.Ready()) {
        error = path + ": " + handle.Error();
        return false;
    }
    if (!LoadDocument(*handle.Get(), path, error)) return false;
    diskVersion_ = handle.Version();
    return true;
}

bool SceneEditor::OpenProject(const std::string& path, std::string& error) {
    const auto handle = assets_->Load<JsonValue>(path);
    if (!handle.Ready()) {
        error = path + ": " + handle.Error();
        return false;
    }
    Framework::ProjectDesc project;
    std::string parseError;
    if (!Framework::ParseProject(*handle.Get(), project, parseError)) {
        error = path + ": " + parseError;
        return false;
    }
    if (!OpenScene(project.startupScene, error)) return false;
    project_ = std::move(project);
    projectPath_ = path;
    hasProject_ = true;
    status_ = "Opened project " + (project_.name.empty() ? path : project_.name);
    return true;
}

bool SceneEditor::Save(std::string& error) {
    if (scenePath_.empty()) {
        error = "the scene has no file yet; use Save As";
        return false;
    }
    return SaveAs(scenePath_, error);
}

bool SceneEditor::SaveAs(const std::string& path, std::string& error) {
    const std::filesystem::path relative(path);
    if (path.empty() || relative.is_absolute() || relative.lexically_normal().string().rfind("..", 0) == 0) {
        error = "scene paths are relative to the content root (" + contentRoot_ + ")";
        return false;
    }
    const std::filesystem::path full = std::filesystem::path(contentRoot_) / relative;
    std::error_code directoryError;
    if (full.has_parent_path()) std::filesystem::create_directories(full.parent_path(), directoryError);
    if (!Core::WriteJsonFile(full.string(), json_, error)) return false;
    scenePath_ = path;
    savedJson_ = json_;
    // The asset cache now matches the file, so hot reload does not see our own save.
    assets_->Reload(path);
    diskVersion_ = assets_->Load<JsonValue>(path).Version();
    status_ = "Saved " + path;
    return true;
}

bool SceneEditor::ReloadIfChangedOnDisk() {
    if (scenePath_.empty() || Playing()) return false;
    assets_->PollChanges();
    const auto handle = assets_->Load<JsonValue>(scenePath_);
    if (!handle.Ready() || handle.Version() == diskVersion_) return false;
    diskVersion_ = handle.Version();
    if (*handle.Get() == json_) return false;
    if (Dirty()) {
        status_ = scenePath_ + " changed on disk; your unsaved edits are kept";
        return false;
    }
    Framework::SceneDocument document;
    std::string error;
    if (!Framework::SceneSerializer(assets_.get()).Parse(*handle.Get(), document, error)) {
        status_ = "Not reloaded, " + scenePath_ + " has an error: " + error;
        return false;
    }
    const SelectionKind kind = selectionKind_;
    const NodePath selection = selection_;
    PushUndo(); // the reload itself can be undone
    json_ = *handle.Get();
    savedJson_ = json_;
    parsed_ = std::move(document);
    Rebuild();
    if (kind != SelectionKind::Entity || !Select(selection)) {
        selectionKind_ = kind == SelectionKind::Scene ? SelectionKind::Scene : SelectionKind::None;
        selection_.clear();
    }
    status_ = "Reloaded " + scenePath_ + " (changed on disk)";
    return true;
}

// ================================================================ document plumbing

JsonValue* SceneEditor::Node(JsonValue& json, const NodePath& path) const {
    if (path.empty()) return nullptr;
    JsonValue* array = json.Find("entities");
    JsonValue* node = nullptr;
    for (std::size_t index : path) {
        if (!array || !array->IsArray() || index >= array->Size()) return nullptr;
        node = &array->Items()[index];
        array = node->Find("children");
    }
    return node;
}

const JsonValue* SceneEditor::Node(const JsonValue& json, const NodePath& path) const {
    return Node(const_cast<JsonValue&>(json), path);
}

const EntitySpec* SceneEditor::Spec(const NodePath& path) const {
    const JsonValue* node = Node(json_, path);
    if (!node || path[0] >= parsed_.entities.size()) return nullptr;
    const EntitySpec* spec = &parsed_.entities[path[0]];
    const JsonValue* current = &json_["entities"][path[0]];
    for (std::size_t depth = 1; depth < path.size(); ++depth) {
        // Authored children come after the prefab's and the model's.
        const JsonValue* children = current->Find("children");
        if (!children) return nullptr;
        const std::size_t authored = children->Size();
        if (spec->children.size() < authored) return nullptr;
        spec = &spec->children[spec->children.size() - authored + path[depth]];
        current = &(*children)[path[depth]];
    }
    return spec;
}

void SceneEditor::PushUndo() {
    undo_.push_back({json_, selectionKind_, selection_});
    if (undo_.size() > kMaxUndo) undo_.erase(undo_.begin());
    redo_.clear();
}

bool SceneEditor::Commit(JsonValue next, SelectionKind kind, NodePath selection, std::string& error) {
    Framework::SceneDocument document;
    if (!Framework::SceneSerializer(assets_.get()).Parse(next, document, error)) {
        status_ = "Edit rejected: " + error;
        return false;
    }
    PushUndo();
    json_ = std::move(next);
    parsed_ = std::move(document);
    selectionKind_ = kind;
    selection_ = std::move(selection);
    Rebuild();
    status_.clear();
    return true;
}

void SceneEditor::Rebuild() {
    preview_.reset();
    auto world = std::make_unique<Framework::GameWorld>(Framework::GameWorldSettings{}, nullptr, assets_.get());
    Framework::SceneDocument document = parsed_;
    for (EntitySpec& spec : document.entities) StripBehaviours(spec);
    const std::vector<Entity> roots = Framework::SceneSerializer(assets_.get()).Instantiate(document, *world);
    preview_ = std::move(world);
    nodeEntities_.clear();
    entityNodes_.clear();
    if (const JsonValue* entities = json_.Find("entities")) {
        for (std::size_t i = 0; i < entities->Size() && i < roots.size(); ++i) {
            NodePath path{i};
            MapNodes((*entities)[i], roots[i], path);
        }
    }
    editorCamera_ = preview_->CreateEntity(kEditorCameraName);
    preview_->Add<Framework::Camera>(editorCamera_).priority = std::numeric_limits<int>::max();
    preview_->UpdateTransforms();
}

void SceneEditor::MapNodes(const JsonValue& node, Entity entity, NodePath& path) {
    nodeEntities_[path] = entity;
    entityNodes_[Pack(entity)] = path;
    const JsonValue* children = node.Find("children");
    if (!children || !children->IsArray()) return;
    const std::vector<Entity> worldChildren = World::GetChildren(preview_->Registry(), entity);
    const std::size_t authored = children->Size();
    if (worldChildren.size() < authored) return;
    for (std::size_t j = 0; j < authored; ++j) {
        path.push_back(j);
        MapNodes((*children)[j], worldChildren[worldChildren.size() - authored + j], path);
        path.pop_back();
    }
}

// ================================================================ hierarchy

std::vector<OutlinerRow> SceneEditor::Outliner() const {
    std::vector<OutlinerRow> rows;
    OutlinerRow scene;
    scene.kind = SelectionKind::Scene;
    scene.label = "Scene: " + (scenePath_.empty() ? std::string("(untitled)") : scenePath_) + (Dirty() ? " *" : "");
    rows.push_back(scene);
    NodePath path;
    const std::function<void(const JsonValue&, int)> visit = [&](const JsonValue& array, int depth) {
        for (std::size_t i = 0; i < array.Size(); ++i) {
            const JsonValue& node = array[i];
            path.push_back(i);
            OutlinerRow row;
            row.path = path;
            row.depth = depth;
            row.label = node.String("name");
            if (row.label.empty()) row.label = node.Has("model") ? node.String("model") : "(unnamed)";
            if (node.Has("prefab")) row.label += "  [" + node.String("prefab") + "]";
            if (node.Has("model")) row.label += "  [model]";
            rows.push_back(row);
            if (const JsonValue* children = node.Find("children")) visit(*children, depth + 1);
            path.pop_back();
        }
    };
    if (const JsonValue* entities = json_.Find("entities")) visit(*entities, 1);
    return rows;
}

bool SceneEditor::Select(const NodePath& path) {
    if (!Node(json_, path)) return false;
    selectionKind_ = SelectionKind::Entity;
    selection_ = path;
    return true;
}

void SceneEditor::SelectScene() {
    selectionKind_ = SelectionKind::Scene;
    selection_.clear();
}

void SceneEditor::ClearSelection() {
    selectionKind_ = SelectionKind::None;
    selection_.clear();
}

bool SceneEditor::FindNode(const std::string& name, NodePath& out) const {
    for (const OutlinerRow& row : Outliner()) {
        if (row.kind == SelectionKind::Entity && NodeName(row.path) == name) {
            out = row.path;
            return true;
        }
    }
    return false;
}

std::string SceneEditor::NodeName(const NodePath& path) const {
    const JsonValue* node = Node(json_, path);
    if (!node) return {};
    if (node->Has("name")) return node->String("name");
    const EntitySpec* spec = Spec(path);
    return spec ? spec->name : std::string();
}

Entity SceneEditor::PreviewEntity(const NodePath& path) const {
    const auto found = nodeEntities_.find(path);
    return found == nodeEntities_.end() ? Entity{} : found->second;
}

std::string SceneEditor::UniqueName(const std::string& base) const {
    std::set<std::string> used;
    for (const OutlinerRow& row : Outliner()) {
        if (row.kind == SelectionKind::Entity) used.insert(NodeName(row.path));
    }
    if (!used.count(base)) return base;
    for (int n = 2;; ++n) {
        const std::string candidate = base + " " + std::to_string(n);
        if (!used.count(candidate)) return candidate;
    }
}

// ================================================================ edits

bool SceneEditor::Rename(const std::string& name, std::string& error) {
    if (selectionKind_ != SelectionKind::Entity) return Fail(error, "select an entity first");
    JsonValue next = json_;
    Node(next, selection_)->Set("name", name);
    return Commit(std::move(next), selectionKind_, selection_, error);
}

Vec3 SceneEditor::Position() const {
    const EntitySpec* spec = selectionKind_ == SelectionKind::Entity ? Spec(selection_) : nullptr;
    return spec ? spec->transform.position : Vec3{};
}

Vec3 SceneEditor::EulerDegrees() const {
    const EntitySpec* spec = selectionKind_ == SelectionKind::Entity ? Spec(selection_) : nullptr;
    if (!spec) return {};
    // Keep the angles as authored when they are ({"euler": [...]}), so 200 degrees stays 200.
    const JsonValue* node = Node(json_, selection_);
    if (const JsonValue* transform = node->Find("transform")) {
        if (const JsonValue* rotation = transform->Find("rotation"); rotation && rotation->IsObject()) {
            Vec3 euler;
            if (ReadVec3((*rotation)["euler"], euler)) return euler;
        }
    }
    return EulerDegreesFromQuat(spec->transform.rotation);
}

Vec3 SceneEditor::Scale() const {
    const EntitySpec* spec = selectionKind_ == SelectionKind::Entity ? Spec(selection_) : nullptr;
    return spec ? spec->transform.scale : Vec3{1.0f, 1.0f, 1.0f};
}

void SceneEditor::WriteTransform(JsonValue& node, Vec3 position, Vec3 euler, Vec3 scale) const {
    JsonValue transform = JsonValue::MakeObject();
    transform.Set("position", Vec3Json(position));
    if (!NearlyEqual(euler, {})) {
        JsonValue rotation = JsonValue::MakeObject();
        rotation.Set("euler", Vec3Json(euler));
        transform.Set("rotation", rotation);
    }
    if (!NearlyEqual(scale, {1.0f, 1.0f, 1.0f})) transform.Set("scale", Vec3Json(scale));
    node.Set("transform", transform);
}

void SceneEditor::ApplyPreviewTransform(Vec3 position, Vec3 euler, Vec3 scale) {
    // Transform edits cannot make a scene invalid, so they skip the re-parse
    // (and keep gizmo drags cheap); the parsed spec and preview follow directly.
    if (EntitySpec* spec = const_cast<EntitySpec*>(Spec(selection_))) {
        spec->transform.position = position;
        spec->transform.rotation = QuatFromEulerDegrees(euler);
        spec->transform.scale = scale;
    }
    const Entity entity = PreviewEntity(selection_);
    if (!entity.IsNull()) {
        Math::TRS local;
        local.translation = position;
        local.rotation = QuatFromEulerDegrees(euler);
        local.scale = scale;
        preview_->SetLocal(entity, local);
        preview_->UpdateTransforms();
    }
}

bool SceneEditor::SetTransform(Vec3 position, Vec3 euler, Vec3 scale, std::string& error) {
    if (selectionKind_ != SelectionKind::Entity) return Fail(error, "select an entity first");
    if (!Math::IsFinite(position) || !Math::IsFinite(euler) || !Math::IsFinite(scale)) return Fail(error, "values must be finite numbers");
    if (std::fabs(scale.x) < 1.0e-4f || std::fabs(scale.y) < 1.0e-4f || std::fabs(scale.z) < 1.0e-4f) {
        return Fail(error, "scale components must not be zero");
    }
    PushUndo();
    WriteTransform(*Node(json_, selection_), position, euler, scale);
    ApplyPreviewTransform(position, euler, scale);
    return true;
}

JsonValue* SceneEditor::GroupObject(JsonValue& json, InspectorGroupKind kind, const std::string& group,
    std::size_t index, bool create) const {
    auto child = [create](JsonValue& owner, const std::string& key) -> JsonValue* {
        if (JsonValue* found = owner.Find(key); found && found->IsObject()) return found;
        return create ? &owner.Set(key, JsonValue::MakeObject()) : nullptr;
    };
    switch (kind) {
    case InspectorGroupKind::Settings: return child(json, "settings");
    case InspectorGroupKind::Environment: return child(json, "environment");
    case InspectorGroupKind::Material: {
        JsonValue* materials = child(json, "materials");
        return materials ? child(*materials, group) : nullptr;
    }
    case InspectorGroupKind::Entity: return selectionKind_ == SelectionKind::Entity ? Node(json, selection_) : nullptr;
    case InspectorGroupKind::Component: {
        JsonValue* node = selectionKind_ == SelectionKind::Entity ? Node(json, selection_) : nullptr;
        JsonValue* components = node ? child(*node, "components") : nullptr;
        return components ? child(*components, group) : nullptr;
    }
    case InspectorGroupKind::Behaviour: {
        JsonValue* node = selectionKind_ == SelectionKind::Entity ? Node(json, selection_) : nullptr;
        JsonValue* behaviours = node ? node->Find("behaviours") : nullptr;
        if (!behaviours || !behaviours->IsArray() || index >= behaviours->Size()) return nullptr;
        return &behaviours->Items()[index];
    }
    case InspectorGroupKind::Transform: return nullptr;
    }
    return nullptr;
}

bool SceneEditor::SetField(InspectorGroupKind kind, const std::string& group, std::size_t index,
    const std::string& field, const JsonValue& value, std::string& error) {
    if (kind == InspectorGroupKind::Behaviour && field == "type") {
        return Fail(error, "a behaviour's type cannot change; remove it and add another");
    }
    JsonValue next = json_;
    JsonValue* object = GroupObject(next, kind, group, index, true);
    if (!object) return Fail(error, "nothing to edit");
    object->Set(field, value);
    return Commit(std::move(next), selectionKind_, selection_, error);
}

bool SceneEditor::ResetField(InspectorGroupKind kind, const std::string& group, std::size_t index,
    const std::string& field, std::string& error) {
    JsonValue next = json_;
    JsonValue* object = GroupObject(next, kind, group, index, false);
    if (!object || !object->Has(field)) return Fail(error, "'" + field + "' is not set here");
    if (kind == InspectorGroupKind::Behaviour && field == "type") return Fail(error, "a behaviour needs its type");
    object->Erase(field);
    return Commit(std::move(next), selectionKind_, selection_, error);
}

bool SceneEditor::AddComponent(const std::string& type, std::string& error) {
    if (selectionKind_ != SelectionKind::Entity) return Fail(error, "select an entity first");
    if (!IsComponentType(type)) return Fail(error, "unknown component '" + type + "'");
    const EntitySpec* spec = Spec(selection_);
    if (spec && spec->components.Has(type)) return Fail(error, "the entity already has a " + type);
    JsonValue next = json_;
    JsonValue* node = Node(next, selection_);
    JsonValue* components = node->Find("components");
    if (!components || !components->IsObject()) components = &node->Set("components", JsonValue::MakeObject());
    components->Set(type, JsonValue::MakeObject());
    return Commit(std::move(next), selectionKind_, selection_, error);
}

bool SceneEditor::RemoveComponent(const std::string& type, std::string& error) {
    if (selectionKind_ != SelectionKind::Entity) return Fail(error, "select an entity first");
    JsonValue next = json_;
    JsonValue* node = Node(next, selection_);
    JsonValue* components = node->Find("components");
    if (!components || !components->Has(type)) {
        const std::string prefab = node->String("prefab");
        return Fail(error, prefab.empty() ? "the entity has no " + type
                                       : type + " comes from prefab '" + prefab + "'; edit the prefab to remove it");
    }
    const std::string prefab = node->String("prefab");
    if (!prefab.empty()) {
        const auto found = parsed_.prefabs.find(prefab);
        if (found != parsed_.prefabs.end() && found->second.components.Has(type)) {
            return Fail(error, type + " comes from prefab '" + prefab + "'; this only clears the overrides");
        }
    }
    components->Erase(type);
    if (components->Keys().empty()) node->Erase("components");
    return Commit(std::move(next), selectionKind_, selection_, error);
}

bool SceneEditor::AddBehaviour(const std::string& type, std::string& error) {
    if (selectionKind_ != SelectionKind::Entity) return Fail(error, "select an entity first");
    if (!Framework::BehaviourRegistry::Global().Has(type)) return Fail(error, "unknown behaviour '" + type + "'");
    JsonValue next = json_;
    JsonValue behaviour = JsonValue::MakeObject();
    behaviour.Set("type", type);
    ChildArray(*Node(next, selection_), "behaviours").Append(behaviour);
    return Commit(std::move(next), selectionKind_, selection_, error);
}

bool SceneEditor::RemoveBehaviour(std::size_t index, std::string& error) {
    if (selectionKind_ != SelectionKind::Entity) return Fail(error, "select an entity first");
    JsonValue next = json_;
    JsonValue* node = Node(next, selection_);
    JsonValue* behaviours = node->Find("behaviours");
    if (!behaviours || !behaviours->IsArray() || index >= behaviours->Size()) return Fail(error, "no such behaviour");
    behaviours->Items().erase(behaviours->Items().begin() + static_cast<std::ptrdiff_t>(index));
    if (behaviours->Size() == 0) node->Erase("behaviours");
    return Commit(std::move(next), selectionKind_, selection_, error);
}

bool SceneEditor::AddMaterial(const std::string& name, std::string& error) {
    if (name.empty()) return Fail(error, "a material needs a name");
    if (const JsonValue* materials = json_.Find("materials"); materials && materials->Has(name)) {
        return Fail(error, "material '" + name + "' already exists");
    }
    JsonValue next = json_;
    JsonValue* materials = next.Find("materials");
    if (!materials || !materials->IsObject()) materials = &next.Set("materials", JsonValue::MakeObject());
    JsonValue material = JsonValue::MakeObject();
    material.Set("shading", "Toon");
    material.Set("baseColor", Vec3Json({0.8f, 0.8f, 0.8f}));
    materials->Set(name, material);
    return Commit(std::move(next), selectionKind_, selection_, error);
}

bool SceneEditor::CreateEntity(const std::string& kind, std::string& error) {
    // Place on the ground under the view centre (or at the focus when looking up).
    Vec3 position = camera_.target;
    const Math::Ray ray{camera_.Eye(), camera_.Forward()};
    if (ray.direction.y < -1.0e-3f) {
        const float t = -ray.origin.y / ray.direction.y;
        position = ray.At(t);
    }
    position = {std::round(position.x * 2.0f) * 0.5f, 0.0f, std::round(position.z * 2.0f) * 0.5f};

    JsonValue node = JsonValue::MakeObject();
    JsonValue components = JsonValue::MakeObject();
    std::string name;
    Vec3 euler{};
    auto renderer = [&](const std::string& mesh) {
        JsonValue value = JsonValue::MakeObject();
        value.Set("mesh", "primitive:" + mesh);
        components.Set("MeshRenderer", value);
    };
    auto collider = [&](const char* shape) {
        JsonValue value = JsonValue::MakeObject();
        value.Set("shape", shape);
        components.Set("Collider", value);
    };
    if (kind == "empty") {
        name = "Entity";
    } else if (kind.rfind("primitive:", 0) == 0) {
        const std::string mesh = kind.substr(10);
        if (!Framework::MakePrimitiveMesh(mesh)) return Fail(error, "unknown primitive '" + mesh + "'");
        name = std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(mesh[0])))) + mesh.substr(1);
        renderer(mesh);
        if (mesh == "cube") collider("Box");
        if (mesh == "sphere") collider("Sphere");
        if (mesh == "capsule") collider("Capsule");
        if (mesh == "plane") collider("Mesh");
        position.y = mesh == "capsule" || mesh == "cylinder" ? 1.0f : (mesh == "plane" ? 0.0f : 0.5f);
    } else if (kind == "light:point") {
        name = "Point Light";
        JsonValue light = JsonValue::MakeObject();
        light.Set("type", "Point");
        light.Set("intensity", 2.0);
        light.Set("radius", 8.0);
        components.Set("Light", light);
        position.y = 2.5f;
    } else if (kind == "light:directional") {
        name = "Directional Light";
        JsonValue light = JsonValue::MakeObject();
        light.Set("type", "Directional");
        components.Set("Light", light);
        euler = {50.0f, 30.0f, 0.0f};
        position.y = 5.0f;
    } else if (kind == "camera") {
        // A camera looking the way the editor view does (UE's "create camera here").
        name = "Camera";
        components.Set("Camera", JsonValue::MakeObject());
        position = camera_.Eye();
        euler = {camera_.pitchDegrees, camera_.yawDegrees, 0.0f};
    } else if (kind.rfind("prefab:", 0) == 0) {
        const std::string prefab = kind.substr(7);
        if (!parsed_.prefabs.count(prefab)) return Fail(error, "unknown prefab '" + prefab + "'");
        node.Set("prefab", prefab);
        name = prefab;
        const Vec3 prefabScale = parsed_.prefabs.at(prefab).transform.scale;
        position.y = std::max(0.0f, prefabScale.y * 0.5f);
    } else if (kind.rfind("model:", 0) == 0) {
        const std::string model = kind.substr(6);
        node.Set("model", model);
        name = std::filesystem::path(model).stem().string();
    } else {
        return Fail(error, "cannot create '" + kind + "'");
    }
    node.Set("name", UniqueName(name));
    WriteTransform(node, position, euler, {1.0f, 1.0f, 1.0f});
    if (!components.Keys().empty()) node.Set("components", components);

    JsonValue next = json_;
    JsonValue& entities = ChildArray(next, "entities");
    entities.Append(node);
    return Commit(std::move(next), SelectionKind::Entity, {entities.Size() - 1}, error);
}

bool SceneEditor::Duplicate(std::string& error) {
    if (selectionKind_ != SelectionKind::Entity) return Fail(error, "select an entity first");
    JsonValue next = json_;
    JsonValue copy = *Node(next, selection_);
    const std::string base = NodeName(selection_).empty() ? std::string("Entity") : NodeName(selection_);
    copy.Set("name", UniqueName(base));
    // Offset the copy so it is visible next to the original.
    Vec3 position = Position();
    position.x += 1.0f;
    WriteTransform(copy, position, EulerDegrees(), Scale());
    NodePath parent(selection_.begin(), selection_.end() - 1);
    JsonValue& siblings = parent.empty() ? ChildArray(next, "entities") : ChildArray(*Node(next, parent), "children");
    const std::size_t index = selection_.back() + 1;
    siblings.Items().insert(siblings.Items().begin() + static_cast<std::ptrdiff_t>(index), std::move(copy));
    NodePath selection = selection_;
    selection.back() = index;
    return Commit(std::move(next), SelectionKind::Entity, selection, error);
}

bool SceneEditor::Delete(std::string& error) {
    if (selectionKind_ != SelectionKind::Entity) return Fail(error, "select an entity first");
    JsonValue next = json_;
    NodePath parent(selection_.begin(), selection_.end() - 1);
    JsonValue* owner = parent.empty() ? &next : Node(next, parent);
    JsonValue& siblings = ChildArray(*owner, parent.empty() ? "entities" : "children");
    siblings.Items().erase(siblings.Items().begin() + static_cast<std::ptrdiff_t>(selection_.back()));
    if (!parent.empty() && siblings.Size() == 0) owner->Erase("children");
    return Commit(std::move(next), parent.empty() ? SelectionKind::None : SelectionKind::Entity, parent, error);
}

bool SceneEditor::Reparent(const NodePath& parent, std::string& error) {
    if (selectionKind_ != SelectionKind::Entity) return Fail(error, "select an entity first");
    if (parent.size() >= selection_.size() && std::equal(selection_.begin(), selection_.end(), parent.begin())) {
        return Fail(error, "an entity cannot be parented under itself");
    }
    if (!parent.empty() && !Node(json_, parent)) return Fail(error, "no such parent");
    JsonValue next = json_;
    const NodePath oldParent(selection_.begin(), selection_.end() - 1);
    JsonValue* oldOwner = oldParent.empty() ? &next : Node(next, oldParent);
    JsonValue& oldSiblings = ChildArray(*oldOwner, oldParent.empty() ? "entities" : "children");
    JsonValue moved = oldSiblings.Items()[selection_.back()];
    oldSiblings.Items().erase(oldSiblings.Items().begin() + static_cast<std::ptrdiff_t>(selection_.back()));
    if (!oldParent.empty() && oldSiblings.Size() == 0) oldOwner->Erase("children");
    // Removing the node shifts later siblings (and anything under them) up by one.
    NodePath target = parent;
    if (target.size() > oldParent.size() && std::equal(oldParent.begin(), oldParent.end(), target.begin())
        && target[oldParent.size()] > selection_.back()) {
        --target[oldParent.size()];
    }
    JsonValue* newOwner = target.empty() ? &next : Node(next, target);
    if (!newOwner) return Fail(error, "no such parent");
    JsonValue& newSiblings = ChildArray(*newOwner, target.empty() ? "entities" : "children");
    newSiblings.Append(std::move(moved));
    NodePath selection = target;
    selection.push_back(newSiblings.Size() - 1);
    return Commit(std::move(next), SelectionKind::Entity, selection, error);
}

bool SceneEditor::Undo() {
    if (undo_.empty()) return false;
    Snapshot previous = std::move(undo_.back());
    undo_.pop_back();
    Framework::SceneDocument document;
    std::string error;
    if (!Framework::SceneSerializer(assets_.get()).Parse(previous.json, document, error)) {
        status_ = "Cannot undo: " + error;
        undo_.push_back(std::move(previous));
        return false;
    }
    redo_.push_back({json_, selectionKind_, selection_});
    json_ = std::move(previous.json);
    parsed_ = std::move(document);
    Rebuild();
    selectionKind_ = previous.kind;
    selection_ = previous.selection;
    if (selectionKind_ == SelectionKind::Entity && !Node(json_, selection_)) ClearSelection();
    status_ = "Undo";
    return true;
}

bool SceneEditor::Redo() {
    if (redo_.empty()) return false;
    Snapshot next = std::move(redo_.back());
    redo_.pop_back();
    Framework::SceneDocument document;
    std::string error;
    if (!Framework::SceneSerializer(assets_.get()).Parse(next.json, document, error)) {
        status_ = "Cannot redo: " + error;
        redo_.push_back(std::move(next));
        return false;
    }
    undo_.push_back({json_, selectionKind_, selection_});
    json_ = std::move(next.json);
    parsed_ = std::move(document);
    Rebuild();
    selectionKind_ = next.kind;
    selection_ = next.selection;
    if (selectionKind_ == SelectionKind::Entity && !Node(json_, selection_)) ClearSelection();
    status_ = "Redo";
    return true;
}

// ================================================================ inspector

std::vector<InspectorGroup> SceneEditor::Inspector() const {
    std::vector<InspectorGroup> groups;
    const Core::TypeRegistry& types = Framework::FrameworkTypes();
    auto materialGroup = [&](const std::string& name) {
        const JsonValue* materials = json_.Find("materials");
        const JsonValue* authored = materials ? materials->Find(name) : nullptr;
        if (!authored) return;
        InspectorGroup group;
        group.kind = InspectorGroupKind::Material;
        group.name = name;
        group.fields = ReflectedFields(*types.Find("Material"), *authored, authored);
        groups.push_back(std::move(group));
    };

    if (selectionKind_ == SelectionKind::Scene) {
        const JsonValue* settings = json_.Find("settings");
        InspectorGroup group;
        group.kind = InspectorGroupKind::Settings;
        group.name = "Settings";
        Vec3 gravity{0.0f, -9.81f, 0.0f};
        const bool hasGravity = settings && ReadVec3((*settings)["gravity"], gravity);
        group.fields.push_back(MakeField("gravity", FieldKind::Vec3, Vec3Json(gravity), hasGravity));
        group.fields.push_back(MakeField("fixedDelta", FieldKind::Float,
            settings && settings->Has("fixedDelta") ? (*settings)["fixedDelta"] : JsonValue(1.0 / 60.0),
            settings && settings->Has("fixedDelta"), 0.0001, 1.0));
        group.fields.push_back(MakeField("timeScale", FieldKind::Float,
            settings && settings->Has("timeScale") ? (*settings)["timeScale"] : JsonValue(1.0),
            settings && settings->Has("timeScale"), 0.0, 100.0));
        groups.push_back(std::move(group));

        const JsonValue* environment = json_.Find("environment");
        InspectorGroup env;
        env.kind = InspectorGroupKind::Environment;
        env.name = "Environment";
        env.fields = ReflectedFields(*types.Find("Environment"), environment ? *environment : JsonValue::MakeObject(),
            environment);
        groups.push_back(std::move(env));
        if (const JsonValue* materials = json_.Find("materials")) {
            for (const std::string& name : materials->Keys()) materialGroup(name);
        }
        return groups;
    }
    if (selectionKind_ != SelectionKind::Entity) return groups;
    const JsonValue* node = Node(json_, selection_);
    const EntitySpec* spec = Spec(selection_);
    if (!node || !spec) return groups;

    InspectorGroup entity;
    entity.kind = InspectorGroupKind::Entity;
    entity.name = node->Has("prefab") ? "Entity (prefab " + node->String("prefab") + ")" : "Entity";
    entity.fields.push_back(MakeField("name", FieldKind::String, NodeName(selection_), node->Has("name")));
    groups.push_back(std::move(entity));

    InspectorGroup transform;
    transform.kind = InspectorGroupKind::Transform;
    transform.name = "Transform";
    transform.fields.push_back(MakeField("position", FieldKind::Vec3, Vec3Json(Position()), node->Has("transform")));
    transform.fields.push_back(MakeField("rotation", FieldKind::Vec3, Vec3Json(EulerDegrees()), node->Has("transform")));
    transform.fields.push_back(MakeField("scale", FieldKind::Vec3, Vec3Json(Scale()), node->Has("transform")));
    transform.fields[1].tooltip = "Euler degrees: pitch (X), yaw (Y), roll (Z)";
    groups.push_back(std::move(transform));

    const JsonValue* authoredComponents = node->Find("components");
    for (const char* type : kComponentTypes) {
        const JsonValue* effective = spec->components.Find(type);
        if (!effective) continue;
        InspectorGroup group;
        group.kind = InspectorGroupKind::Component;
        group.name = type;
        const JsonValue* authored = authoredComponents ? authoredComponents->Find(type) : nullptr;
        group.removable = authored != nullptr;
        if (const std::string prefab = node->String("prefab"); !prefab.empty()) {
            const auto found = parsed_.prefabs.find(prefab);
            if (found != parsed_.prefabs.end() && found->second.components.Has(type)) group.removable = false;
        }
        group.fields = ReflectedFields(*types.Find(type), *effective, authored);
        groups.push_back(std::move(group));
    }
    if (const JsonValue* behaviours = node->Find("behaviours")) {
        for (std::size_t i = 0; i < behaviours->Size(); ++i) {
            const JsonValue& authored = (*behaviours)[i];
            const std::string type = authored.String("type");
            const Core::TypeInfo* info = Framework::BehaviourRegistry::Global().Info(type);
            InspectorGroup group;
            group.kind = InspectorGroupKind::Behaviour;
            group.name = type;
            group.index = i;
            group.removable = true;
            if (info) group.fields = ReflectedFields(*info, authored, &authored);
            groups.push_back(std::move(group));
        }
    }
    // The material this entity renders with, so it can be tuned in place.
    if (const JsonValue* renderer = spec->components.Find("MeshRenderer")) {
        const std::string material = renderer->String("material");
        if (!material.empty()) materialGroup(material);
    }
    return groups;
}

std::vector<std::string> SceneEditor::AddableComponents() const {
    std::vector<std::string> types;
    const EntitySpec* spec = selectionKind_ == SelectionKind::Entity ? Spec(selection_) : nullptr;
    if (!spec) return types;
    for (const char* type : kComponentTypes) {
        if (!spec->components.Has(type)) types.emplace_back(type);
    }
    return types;
}

std::vector<std::string> SceneEditor::BehaviourTypes() const {
    return Framework::BehaviourRegistry::Global().Names();
}

std::vector<std::string> SceneEditor::CreatableKinds() const {
    std::vector<std::string> kinds{"empty", "primitive:cube", "primitive:sphere", "primitive:plane",
        "primitive:capsule", "primitive:cylinder", "light:point", "light:directional", "camera"};
    for (const auto& prefab : parsed_.prefabs) kinds.push_back("prefab:" + prefab.first);
    std::error_code error;
    const std::filesystem::path root(contentRoot_);
    std::size_t models = 0;
    for (std::filesystem::recursive_directory_iterator it(root, error), end; !error && it != end && models < 200;
         it.increment(error)) {
        if (!it->is_regular_file(error)) continue;
        const std::string extension = it->path().extension().string();
        if (extension != ".glb" && extension != ".gltf") continue;
        kinds.push_back("model:" + std::filesystem::relative(it->path(), root, error).generic_string());
        ++models;
    }
    return kinds;
}

// ================================================================ viewport

Math::AABB SceneEditor::EntityBounds(Entity entity) const {
    Math::AABB bounds;
    const World::Registry& registry = preview_->Registry();
    const std::function<void(Entity)> visit = [&](Entity current) {
        const Framework::MeshRenderer* renderer = registry.Get<Framework::MeshRenderer>(current);
        const World::WorldTransform* world = registry.Get<World::WorldTransform>(current);
        const Graphics::MeshData* mesh = renderer ? renderer->mesh.get() : nullptr;
        if (!mesh && renderer && renderer->lods && renderer->lods->Count() > 0) {
            mesh = renderer->lods->levels.front().mesh.get();
        }
        if (mesh && world && mesh->bounds.IsValid()) bounds.Expand(Math::TransformAABB(world->matrix, mesh->bounds));
        for (Entity child : World::GetChildren(registry, current)) visit(child);
    };
    visit(entity);
    if (!bounds.IsValid()) {
        const Vec3 position = preview_->GetWorld(entity).translation;
        bounds = Math::AABB::FromCenterExtents(position, {0.25f, 0.25f, 0.25f});
    }
    return bounds;
}

Math::AABB SceneEditor::SelectionBounds() const {
    if (selectionKind_ == SelectionKind::Entity) {
        const Entity entity = PreviewEntity(selection_);
        if (!entity.IsNull()) return EntityBounds(entity);
    }
    Math::AABB bounds;
    for (const auto& entry : nodeEntities_) {
        if (entry.first.size() == 1) bounds.Expand(EntityBounds(entry.second));
    }
    return bounds;
}

void SceneEditor::FrameSelection() {
    Math::AABB bounds = SelectionBounds();
    if (!bounds.IsValid()) bounds = Math::AABB::FromCenterExtents({0.0f, 1.0f, 0.0f}, {5.0f, 1.0f, 5.0f});
    // Ground planes are huge and flat; frame something closer to the action.
    const Vec3 extents = bounds.Extents();
    if (selectionKind_ != SelectionKind::Entity && extents.x > 30.0f && extents.z > 30.0f) {
        bounds = Math::AABB::FromCenterExtents(bounds.Center(), {12.0f, std::max(2.0f, extents.y), 12.0f});
    }
    camera_.Frame(bounds);
}

bool SceneEditor::Pick(float x, float y, int width, int height) {
    const Math::Ray ray = camera_.RayThrough(x, y, width, height);
    const World::Registry& registry = preview_->Registry();
    float best = std::numeric_limits<float>::max();
    Entity hit{};
    for (Entity entity : registry.EntitiesWith<Framework::MeshRenderer>()) {
        const Framework::MeshRenderer* renderer = registry.Get<Framework::MeshRenderer>(entity);
        const World::WorldTransform* world = registry.Get<World::WorldTransform>(entity);
        const Graphics::MeshData* mesh = renderer ? renderer->mesh.get() : nullptr;
        if (!mesh && renderer && renderer->lods && renderer->lods->Count() > 0) {
            mesh = renderer->lods->levels.front().mesh.get();
        }
        if (!mesh || !world || !renderer->visible || !mesh->bounds.IsValid()) continue;
        float t = 0.0f;
        if (!Math::IntersectRayAABB(ray, Math::TransformAABB(world->matrix, mesh->bounds), best, t)) continue;
        // Exact test against the triangles (boxes of rotated or thin meshes are loose).
        const std::size_t triangles = mesh->TriangleCount();
        if (triangles == 0 || triangles > 50000) {
            best = t;
            hit = entity;
            continue;
        }
        for (std::size_t i = 0; i < triangles; ++i) {
            const Vec3 a = Math::TransformPoint(world->matrix, mesh->vertices[mesh->indices[i * 3]].position);
            const Vec3 b = Math::TransformPoint(world->matrix, mesh->vertices[mesh->indices[i * 3 + 1]].position);
            const Vec3 c = Math::TransformPoint(world->matrix, mesh->vertices[mesh->indices[i * 3 + 2]].position);
            float triangleT = 0.0f;
            float u = 0.0f;
            float v = 0.0f;
            if (Math::IntersectRayTriangle(ray, a, b, c, best, triangleT, u, v) && triangleT < best) {
                best = triangleT;
                hit = entity;
            }
        }
    }
    // Lights, cameras and empties have no mesh: pick their icon.
    for (const auto& entry : nodeEntities_) {
        const Entity entity = entry.second;
        if (registry.Has<Framework::MeshRenderer>(entity)) continue;
        const Vec3 center = preview_->GetWorld(entity).translation;
        float t = 0.0f;
        if (Math::IntersectRaySphere(ray, center, camera_.WorldPerPixel(center, height) * 9.0f, best, t)) {
            best = t;
            hit = entity;
        }
    }
    const SelectionKind previousKind = selectionKind_;
    const NodePath previous = selection_;
    // Entities a prefab or model added select their authored ancestor.
    while (!hit.IsNull() && !entityNodes_.count(Pack(hit))) hit = World::GetParent(registry, hit);
    if (hit.IsNull()) {
        SelectScene();
    } else {
        Select(entityNodes_.at(Pack(hit)));
    }
    return previousKind != selectionKind_ || previous != selection_;
}

void SceneEditor::GizmoFrame(Vec3& pivot, Vec3 axes[3]) const {
    const Entity entity = PreviewEntity(selection_);
    const Math::TRS world = entity.IsNull() ? Math::TRS{} : preview_->GetWorld(entity);
    pivot = world.translation;
    const Entity parent = entity.IsNull() ? Entity{} : World::GetParent(preview_->Registry(), entity);
    const Math::Quat parentRotation = parent.IsNull() ? Math::Quat{} : preview_->GetWorld(parent).rotation;
    const Vec3 euler = EulerDegrees();
    const Vec3 unit[3] = {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    for (int i = 0; i < 3; ++i) {
        switch (tool) {
        case GizmoTool::Rotate: {
            // Rings match the Euler order: yaw about the parent's Y, pitch about
            // the yawed X, roll about the fully rotated Z.
            const Math::Quat yaw = QuatFromEulerDegrees({0.0f, euler.y, 0.0f});
            const Math::Quat yawPitch = QuatFromEulerDegrees({euler.x, euler.y, 0.0f});
            const Math::Quat frame = i == 0 ? yaw : (i == 1 ? Math::Quat{} : yawPitch);
            axes[i] = Math::Rotate(parentRotation * frame, unit[i]);
            break;
        }
        case GizmoTool::Scale: axes[i] = Math::Rotate(world.rotation, unit[i]); break;
        default: axes[i] = Math::Rotate(parentRotation, unit[i]); break;
        }
    }
}

float SceneEditor::GizmoLength(Vec3 pivot, int height) const {
    return camera_.WorldPerPixel(pivot, height) * 90.0f; // handles stay 90 px long
}

GizmoAxis SceneEditor::HitGizmo(float x, float y, int width, int height) const {
    if (Playing() || selectionKind_ != SelectionKind::Entity || tool == GizmoTool::Select) return GizmoAxis::None;
    Vec3 pivot;
    Vec3 axes[3];
    GizmoFrame(pivot, axes);
    const float length = GizmoLength(pivot, height);
    Vec2 origin;
    if (!camera_.Project(pivot, width, height, origin)) return GizmoAxis::None;
    const Vec2 mouse{x, y};
    if (tool == GizmoTool::Scale && DistanceToSegment(mouse, origin, origin) < 9.0f) return GizmoAxis::All;
    GizmoAxis best = GizmoAxis::None;
    float bestDistance = 8.0f;
    for (int i = 0; i < 3; ++i) {
        float distance = std::numeric_limits<float>::max();
        if (tool == GizmoTool::Rotate) {
            const std::vector<Vec3> ring = Ring(pivot, axes[i], length);
            for (std::size_t s = 0; s + 1 < ring.size(); ++s) {
                Vec2 a;
                Vec2 b;
                if (camera_.Project(ring[s], width, height, a) && camera_.Project(ring[s + 1], width, height, b)) {
                    distance = std::min(distance, DistanceToSegment(mouse, a, b));
                }
            }
        } else {
            Vec2 tip;
            if (camera_.Project(pivot + axes[i] * length, width, height, tip)) distance = DistanceToSegment(mouse, origin, tip);
        }
        if (distance < bestDistance) {
            bestDistance = distance;
            best = static_cast<GizmoAxis>(static_cast<int>(GizmoAxis::X) + i);
        }
    }
    return best;
}

bool SceneEditor::BeginDrag(float x, float y, int width, int height) {
    const GizmoAxis axis = HitGizmo(x, y, width, height);
    if (axis == GizmoAxis::None) return false;
    Vec3 pivot;
    Vec3 axes[3];
    GizmoFrame(pivot, axes);
    const float length = GizmoLength(pivot, height);
    drag_ = {};
    drag_.active = true;
    drag_.axis = axis;
    drag_.start = {x, y};
    drag_.position = Position();
    drag_.euler = EulerDegrees();
    drag_.scale = Scale();
    drag_.screenAxis = {0.7071f, -0.7071f}; // up and right grows a uniform scale
    drag_.pixelsPerUnit = 100.0f;
    if (axis != GizmoAxis::All) {
        const int i = static_cast<int>(axis) - static_cast<int>(GizmoAxis::X);
        Vec2 origin;
        Vec2 tip;
        if (camera_.Project(pivot, width, height, origin) && camera_.Project(pivot + axes[i] * length, width, height, tip)) {
            const Vec2 onScreen{tip.x - origin.x, tip.y - origin.y};
            const float pixels = std::sqrt(onScreen.x * onScreen.x + onScreen.y * onScreen.y);
            if (pixels > 4.0f) {
                drag_.screenAxis = {onScreen.x / pixels, onScreen.y / pixels};
                drag_.pixelsPerUnit = pixels / length;
            } else {
                // The axis points at the camera: drag vertically instead.
                drag_.screenAxis = {0.0f, -1.0f};
                drag_.pixelsPerUnit = 90.0f / length;
            }
        }
    }
    return true;
}

bool SceneEditor::Drag(float x, float y, int width, int height) {
    (void)width;
    (void)height;
    if (!drag_.active || selectionKind_ != SelectionKind::Entity) return false;
    const Vec2 moved{x - drag_.start.x, y - drag_.start.y};
    const float along = moved.x * drag_.screenAxis.x + moved.y * drag_.screenAxis.y;
    Vec3 position = drag_.position;
    Vec3 euler = drag_.euler;
    Vec3 scale = drag_.scale;
    const int i = drag_.axis == GizmoAxis::All ? -1 : static_cast<int>(drag_.axis) - static_cast<int>(GizmoAxis::X);
    auto component = [](Vec3& v, int index) -> float& { return index == 0 ? v.x : (index == 1 ? v.y : v.z); };
    switch (tool) {
    case GizmoTool::Move: {
        if (i < 0) return false;
        const Entity entity = PreviewEntity(selection_);
        const Entity parent = entity.IsNull() ? Entity{} : World::GetParent(preview_->Registry(), entity);
        Vec3 parentScale = parent.IsNull() ? Vec3{1.0f, 1.0f, 1.0f} : preview_->GetWorld(parent).scale;
        float& value = component(position, i);
        const float divisor = std::max(1.0e-4f, std::fabs(component(parentScale, i)));
        value += along / drag_.pixelsPerUnit / divisor;
        if (snap) value = std::round(value * 2.0f) * 0.5f;
        break;
    }
    case GizmoTool::Rotate: {
        if (i < 0) return false;
        float& value = component(euler, i);
        value += moved.x * 0.5f;
        if (snap) value = std::round(value / 15.0f) * 15.0f;
        break;
    }
    case GizmoTool::Scale: {
        // Dragging a handle by its own on-screen length (90 px) doubles the scale.
        const float factor = std::max(0.01f, 1.0f + along / 90.0f);
        auto apply = [&](int index) {
            float& value = component(scale, index);
            value *= factor;
            if (snap) value = std::max(0.1f, std::round(value * 10.0f) / 10.0f);
        };
        if (i < 0) {
            apply(0);
            apply(1);
            apply(2);
        } else {
            apply(i);
        }
        break;
    }
    case GizmoTool::Select: return false;
    }
    if (!drag_.changed) PushUndo(); // one undo step for the whole drag
    drag_.changed = true;
    WriteTransform(*Node(json_, selection_), position, euler, scale);
    ApplyPreviewTransform(position, euler, scale);
    return true;
}

void SceneEditor::EndDrag() { drag_ = {}; }

// ================================================================ rendering

const Graphics::ImageRgba8& SceneEditor::Render(int width, int height) {
    width = std::max(1, width);
    height = std::max(1, height);
    if (game_) return game_->Render(width, height);
    Math::TRS eye;
    eye.translation = camera_.Eye();
    eye.rotation = camera_.Rotation();
    preview_->SetLocal(editorCamera_, eye);
    if (Framework::Camera* camera = preview_->Get<Framework::Camera>(editorCamera_)) {
        camera->fieldOfView = camera_.fieldOfView;
        camera->nearPlane = camera_.nearPlane;
        camera->farPlane = camera_.farPlane;
    }
    preview_->UpdateTransforms();
    Graphics::RenderScene scene;
    Graphics::RenderView view;
    target_.Resize(width, height);
    if (preview_->BuildRenderScene(scene, view, width, height)) {
        renderer_.Render(scene, view, target_);
    } else {
        target_.output.Resize(width, height, {24, 27, 34, 255});
    }
    DrawOverlay(target_.output, width, height);
    return target_.output;
}

void SceneEditor::DrawLine3D(Graphics::ImageRgba8& image, Vec3 a, Vec3 b, Graphics::Rgba8 color, int width,
    int height, bool depthTested, float alpha) const {
    // Clip against the near plane first, then to the image.
    const Vec3 eye = camera_.Eye();
    const Vec3 forward = camera_.Forward();
    const float nearDepth = camera_.nearPlane * 1.01f;
    float depthA = Math::Dot(a - eye, forward);
    float depthB = Math::Dot(b - eye, forward);
    if (depthA < nearDepth && depthB < nearDepth) return;
    if (depthA < nearDepth) {
        a = a + (b - a) * ((nearDepth - depthA) / (depthB - depthA));
        depthA = nearDepth;
    }
    if (depthB < nearDepth) {
        b = b + (a - b) * ((nearDepth - depthB) / (depthA - depthB));
        depthB = nearDepth;
    }
    Vec2 pa;
    Vec2 pb;
    if (!camera_.Project(a, width, height, pa) || !camera_.Project(b, width, height, pb)) return;
    float t0 = 0.0f;
    float t1 = 1.0f;
    if (!ClipSegment(pa, pb, 0.0f, 0.0f, static_cast<float>(width - 1), static_cast<float>(height - 1), &t0, &t1)) return;
    const bool haveDepth = depthTested && target_.Width() == width && target_.Height() == height
        && target_.linearDepth.size() == target_.PixelCount();
    if (!haveDepth && alpha >= 1.0f) {
        Graphics::Canvas(image).Line(static_cast<int>(std::lround(pa.x)), static_cast<int>(std::lround(pa.y)),
            static_cast<int>(std::lround(pb.x)), static_cast<int>(std::lround(pb.y)), color);
        return;
    }
    // Depth varies linearly in 1/z across the screen.
    const float inverseA = 1.0f / depthA + (1.0f / depthB - 1.0f / depthA) * t0;
    const float inverseB = 1.0f / depthA + (1.0f / depthB - 1.0f / depthA) * t1;
    const int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(pb.x - pa.x), std::fabs(pb.y - pa.y)))));
    for (int step = 0; step <= steps; ++step) {
        const float t = static_cast<float>(step) / static_cast<float>(steps);
        const int x = static_cast<int>(std::lround(pa.x + (pb.x - pa.x) * t));
        const int y = static_cast<int>(std::lround(pa.y + (pb.y - pa.y) * t));
        if (x < 0 || y < 0 || x >= width || y >= height) continue;
        const std::size_t pixel = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
        if (haveDepth) {
            const float depth = 1.0f / (inverseA + (inverseB - inverseA) * t);
            // A small bias keeps lines lying on a surface (the grid on a floor) visible.
            if (depth > target_.linearDepth[pixel] * 1.002f + 0.02f) continue;
        }
        std::uint8_t* rgba = image.pixels.data() + pixel * 4;
        rgba[0] = static_cast<std::uint8_t>(rgba[0] + (color.r - rgba[0]) * alpha);
        rgba[1] = static_cast<std::uint8_t>(rgba[1] + (color.g - rgba[1]) * alpha);
        rgba[2] = static_cast<std::uint8_t>(rgba[2] + (color.b - rgba[2]) * alpha);
    }
}

void SceneEditor::DrawGrid(Graphics::ImageRgba8& image, int width, int height) const {
    const float step = camera_.distance > 120.0f ? 10.0f : (camera_.distance > 45.0f ? 5.0f : 1.0f);
    const int half = 30;
    const float centerX = std::round(camera_.target.x / step) * step;
    const float centerZ = std::round(camera_.target.z / step) * step;
    const float extent = step * static_cast<float>(half);
    for (int i = -half; i <= half; ++i) {
        const float offset = static_cast<float>(i) * step;
        const float x = centerX + offset;
        const float z = centerZ + offset;
        const bool majorX = std::fmod(std::fabs(x), step * 5.0f) < 1.0e-3f;
        const bool majorZ = std::fmod(std::fabs(z), step * 5.0f) < 1.0e-3f;
        const Graphics::Rgba8 minor{72, 80, 94, 255};
        const Graphics::Rgba8 major{112, 122, 138, 255};
        const bool axisX = std::fabs(x) < 1.0e-3f;
        const bool axisZ = std::fabs(z) < 1.0e-3f;
        DrawLine3D(image, {x, 0.0f, centerZ - extent}, {x, 0.0f, centerZ + extent},
            axisX ? kAxisColors[2] : (majorX ? major : minor), width, height, true, axisX ? 0.8f : (majorX ? 0.55f : 0.3f));
        DrawLine3D(image, {centerX - extent, 0.0f, z}, {centerX + extent, 0.0f, z},
            axisZ ? kAxisColors[0] : (majorZ ? major : minor), width, height, true, axisZ ? 0.8f : (majorZ ? 0.55f : 0.3f));
    }
}

void SceneEditor::DrawOverlay(Graphics::ImageRgba8& image, int width, int height) const {
    Graphics::Canvas canvas(image);
    if (showGrid) DrawGrid(image, width, height);
    const World::Registry& registry = preview_->Registry();
    // Icons for lights and cameras.
    for (const auto& entry : nodeEntities_) {
        const Entity entity = entry.second;
        const bool light = registry.Has<Framework::Light>(entity);
        const bool camera = registry.Has<Framework::Camera>(entity);
        if (!light && !camera) continue;
        const Math::TRS world = preview_->GetWorld(entity);
        Vec2 pixel;
        if (!camera_.Project(world.translation, width, height, pixel)) continue;
        const int px = static_cast<int>(pixel.x);
        const int py = static_cast<int>(pixel.y);
        if (light) {
            canvas.FillCircle(px, py, 6.0f, {255, 214, 90, 255});
            const Framework::Light* data = registry.Get<Framework::Light>(entity);
            if (data && data->type == Framework::LightType::Directional) {
                const Vec3 direction = Math::Rotate(world.rotation, {0.0f, 0.0f, 1.0f});
                DrawLine3D(image, world.translation,
                    world.translation + direction * (camera_.WorldPerPixel(world.translation, height) * 40.0f),
                    {255, 214, 90, 255}, width, height);
            }
        } else {
            canvas.StrokeRect(px - 7, py - 5, 14, 10, {210, 225, 255, 255}, 2);
            const Vec3 forward = Math::Rotate(world.rotation, {0.0f, 0.0f, 1.0f});
            DrawLine3D(image, world.translation,
                world.translation + forward * (camera_.WorldPerPixel(world.translation, height) * 30.0f),
                {210, 225, 255, 255}, width, height);
        }
    }
    if (selectionKind_ == SelectionKind::Entity) {
        const Math::AABB box = SelectionBounds();
        if (box.IsValid()) {
            const Vec3 corner[8] = {{box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z},
                {box.max.x, box.max.y, box.min.z}, {box.min.x, box.max.y, box.min.z}, {box.min.x, box.min.y, box.max.z},
                {box.max.x, box.min.y, box.max.z}, {box.max.x, box.max.y, box.max.z}, {box.min.x, box.max.y, box.max.z}};
            const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5},
                {2, 6}, {3, 7}};
            for (const auto& edge : edges) DrawLine3D(image, corner[edge[0]], corner[edge[1]], kSelectionColor, width, height);
        }
        if (tool != GizmoTool::Select) {
            Vec3 pivot;
            Vec3 axes[3];
            GizmoFrame(pivot, axes);
            const float length = GizmoLength(pivot, height);
            const GizmoAxis active = drag_.active ? drag_.axis : hoverAxis_;
            for (int i = 0; i < 3; ++i) {
                const bool highlighted = active == static_cast<GizmoAxis>(static_cast<int>(GizmoAxis::X) + i);
                const Graphics::Rgba8 color = highlighted ? kHighlight : kAxisColors[i];
                if (tool == GizmoTool::Rotate) {
                    const std::vector<Vec3> ring = Ring(pivot, axes[i], length);
                    for (std::size_t s = 0; s + 1 < ring.size(); ++s) DrawLine3D(image, ring[s], ring[s + 1], color, width, height);
                    continue;
                }
                const Vec3 tip = pivot + axes[i] * length;
                for (int thickness = -1; thickness <= 1; ++thickness) {
                    const Vec3 nudge = camera_.Up() * (camera_.WorldPerPixel(pivot, height) * static_cast<float>(thickness));
                    DrawLine3D(image, pivot + nudge, tip + nudge, color, width, height);
                }
                Vec2 pixel;
                if (camera_.Project(tip, width, height, pixel)) {
                    const int size = tool == GizmoTool::Scale ? 9 : 7;
                    canvas.FillRect(static_cast<int>(pixel.x) - size / 2, static_cast<int>(pixel.y) - size / 2, size,
                        size, color);
                }
            }
            Vec2 origin;
            if (tool == GizmoTool::Scale && camera_.Project(pivot, width, height, origin)) {
                canvas.FillRect(static_cast<int>(origin.x) - 5, static_cast<int>(origin.y) - 5, 10, 10,
                    active == GizmoAxis::All ? kHighlight : Graphics::Rgba8{235, 235, 240, 255});
            }
        }
    }
    static const char* const kToolNames[] = {"Select (Q)", "Move (W)", "Rotate (E)", "Scale (R)"};
    Graphics::TextStyle style;
    style.color = {225, 230, 240, 255};
    style.scale = 2;
    const std::string header = std::string(kToolNames[static_cast<int>(tool)]) + (snap ? "   snap" : "");
    canvas.FillRect(0, 0, Graphics::Canvas::MeasureText(header, style.scale) + 16,
        Graphics::Canvas::LineHeight(style.scale) + 10, {16, 18, 24, 255}, 0.7f);
    canvas.Text(8, 5, header, style);
}

// ================================================================ play in editor

bool SceneEditor::Play(std::string& error) {
    if (game_) return true;
    EndDrag();
    Framework::HostSettings settings;
    settings.contentRoot = contentRoot_;
    settings.audio = false;
    auto host = std::make_unique<Framework::GameHost>(settings);
    if (hasProject_ && !host->ApplyProjectSettings(project_, error)) return false;
    if (!host->LoadSceneJson(json_, error)) return false;
    game_ = std::move(host);
    status_ = "Playing (Esc or Stop returns to the editor; edits made while playing are not kept)";
    return true;
}

void SceneEditor::Stop() {
    if (!game_) return;
    game_.reset();
    status_ = "Stopped";
}

} // namespace Astral::Editor
