#include "Engine/Framework/Scene.h"

#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/Gltf.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <utility>

namespace Astral::Framework {

using Core::JsonValue;
using namespace Math;

namespace {

constexpr const char* kSceneFormat = "astral-scene";
constexpr int kSceneVersion = 1;
// Serialisation order of component types.
const char* const kComponentOrder[] = {"MeshRenderer", "Light", "Camera", "Collider", "RigidBody", "CharacterMover",
    "AudioSource", "AudioListener", "ParticleSystem", "Animator"};

bool Fail(std::string& error, const std::string& path, const std::string& message) {
    error = (path.empty() ? std::string("scene") : path) + ": " + message;
    return false;
}

// Reflection errors start with the type name, which the JSON path already carries.
std::string WithoutType(const std::string& error, const std::string& type) {
    const std::string prefix = type + ": ";
    return error.rfind(prefix, 0) == 0 ? error.substr(prefix.size()) : error;
}

std::string Join(const std::string& path, const std::string& key) { return path.empty() ? key : path + "." + key; }
std::string At(const std::string& path, std::size_t index) { return path + "[" + std::to_string(index) + "]"; }

bool IsComponentName(const std::string& name) {
    for (const char* known : kComponentOrder) {
        if (name == known) return true;
    }
    return false;
}

bool ParseIndex(const std::string& text, std::size_t& value) {
    if (text.empty() || text.size() > 9) return false;
    value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') return false;
        value = value * 10 + static_cast<std::size_t>(c - '0');
    }
    return true;
}

void SplitRef(const std::string& ref, std::string& file, std::string& fragment) {
    const std::size_t hash = ref.find('#');
    file = ref.substr(0, hash);
    fragment = hash == std::string::npos ? std::string() : ref.substr(hash + 1);
}

std::string Stem(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    const std::size_t dot = name.find('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

// Material holds a raw texture pointer; this keeps the texture alive with it.
struct TexturedMaterial {
    Graphics::Material material;
    std::shared_ptr<const Graphics::Texture2D> texture;
};

std::shared_ptr<const Graphics::Material> MakeMaterial(Graphics::Material material,
    std::shared_ptr<const Graphics::Texture2D> texture) {
    auto owner = std::make_shared<TexturedMaterial>();
    owner->material = std::move(material);
    owner->texture = std::move(texture);
    owner->material.baseTexture = owner->texture.get();
    return std::shared_ptr<const Graphics::Material>(owner, &owner->material);
}

JsonValue Vec3Json(Vec3 v) {
    JsonValue array = JsonValue::MakeArray();
    array.Append(v.x);
    array.Append(v.y);
    array.Append(v.z);
    return array;
}

JsonValue QuatJson(Quat q) {
    JsonValue array = JsonValue::MakeArray();
    array.Append(q.x);
    array.Append(q.y);
    array.Append(q.z);
    array.Append(q.w);
    return array;
}

bool ReadVec3(const JsonValue& json, Vec3& out) {
    if (!json.IsArray() || json.Size() != 3) return false;
    float v[3];
    for (std::size_t i = 0; i < 3; ++i) {
        if (!json[i].IsNumber() || !std::isfinite(json[i].AsNumber())) return false;
        v[i] = json[i].AsFloat();
    }
    out = {v[0], v[1], v[2]};
    return true;
}

bool ParseTransform(const JsonValue& json, const std::string& path, TransformDesc& out, std::string& error) {
    if (!json.IsObject()) return Fail(error, path, "must be an object");
    JsonValue copy = json;
    // "scale": 2 is shorthand for [2, 2, 2].
    if (const JsonValue* scale = json.Find("scale"); scale && scale->IsNumber()) {
        const double s = scale->AsNumber();
        JsonValue array = JsonValue::MakeArray();
        array.Append(s);
        array.Append(s);
        array.Append(s);
        copy.Set("scale", array);
    }
    TransformDesc parsed = out;
    std::string fieldError;
    if (!FrameworkTypes().Find("Transform")->FromJson(&parsed, copy, fieldError, true)) {
        return Fail(error, path, WithoutType(fieldError, "Transform"));
    }
    out = parsed;
    return true;
}

template <typename T>
std::shared_ptr<T> Lookup(const std::map<std::string, std::shared_ptr<T>>& map, const std::string& key) {
    const auto found = map.find(key);
    return found == map.end() ? nullptr : found->second;
}

} // namespace

std::shared_ptr<const Graphics::MeshData> MakePrimitiveMesh(const std::string& kind) {
    auto mesh = std::make_shared<Graphics::MeshData>();
    Graphics::MeshBuilder builder(*mesh);
    if (kind == "cube") builder.AddBox({}, {0.5f, 0.5f, 0.5f});
    else if (kind == "sphere") builder.AddSphere({}, 0.5f, 16, 24);
    else if (kind == "plane") builder.AddPlane({}, {0.5f, 0.5f}, 1);
    else if (kind == "capsule") builder.AddCapsule({0.0f, -1.0f, 0.0f}, 0.5f, 2.0f, 8, 16);
    else if (kind == "cylinder") builder.AddCylinder({0.0f, -1.0f, 0.0f}, 0.5f, 2.0f, 24);
    else return nullptr;
    mesh->name = "primitive:" + kind;
    mesh->ComputeBounds();
    return mesh;
}

struct SceneSerializer::SpecExtent {
    std::size_t nodes{1};
    std::size_t height{};
};

struct SceneSerializer::ParseState {
    ParseState(SceneDocument& documentIn, std::string& errorIn, std::vector<Assets::AssetDependency>& dependenciesIn,
        bool strictIn) : document(documentIn), error(errorIn), dependencies(dependenciesIn), strictReload(strictIn) {}
    SceneDocument& document;
    std::string& error;
    std::vector<Assets::AssetDependency>& dependencies;
    bool strictReload{};
    const JsonValue* prefabSources{};
    std::set<std::string> resolving;
    std::size_t entities{};
    std::size_t cachedEntities{};
    std::map<std::string, SpecExtent> prefabExtents;

    template <typename T>
    bool Current(const Assets::AssetHandle<T>& handle) {
        const auto dependency = handle.Dependency();
        if (std::find(dependencies.begin(), dependencies.end(), dependency) == dependencies.end())
            dependencies.push_back(dependency);
        return handle.Ready() && handle.Error().empty() && (!strictReload || handle.Issues().empty());
    }
    template <typename T>
    std::string Diagnostic(const Assets::AssetHandle<T>& handle) const {
        if (!handle.Error().empty()) return handle.Error();
        return handle.Issues().empty() ? "asset is not ready" : handle.Issues().front();
    }
};

namespace {

JsonValue AnimatorPlayback(JsonValue fields) { fields.Erase("binding"); return fields; }

bool AffineInverse(const Mat4& matrix, Mat4& inverse) {
    for (const auto& row : matrix.m) for (float value : row) if (!std::isfinite(value)) return false;
    if (matrix.m[3][0] != 0 || matrix.m[3][1] != 0 || matrix.m[3][2] != 0 || matrix.m[3][3] != 1) return false;
    if (!Inverse(matrix, inverse)) return false;
    for (const auto& row : inverse.m) for (float value : row) if (!std::isfinite(value)) return false;
    return true;
}

bool ReadAnimatorBinding(const JsonValue& json, ImportedAnimatorBinding& binding, std::string& error,
    const std::string& path) {
    if (!json.IsObject()) return Fail(error, path, "must be an object");
    const std::string role = json.String("role");
    if (role != "controller" && role != "primitive") return Fail(error, path, "role must be controller or primitive");
    const bool primitive = role == "primitive";
    for (const auto& key : json.Keys()) {
        if (key != "role" && key != "model" && !(primitive && (key == "node" || key == "skin" || key == "mesh"
            || key == "primitive" || key == "meshBind"))) return Fail(error, Join(path, key), "unknown binding field");
    }
    if (!json["model"].IsString() || json.String("model").empty() || json.String("model").find('#') != std::string::npos)
        return Fail(error, Join(path, "model"), "needs a model content path without a fragment");
    binding.role = primitive ? ImportedAnimatorRole::Primitive : ImportedAnimatorRole::Controller;
    binding.model = json.String("model");
    if (!primitive) return true;
    const char* names[] = {"node", "skin", "mesh", "primitive"};
    int* destinations[] = {&binding.node, &binding.skin, &binding.mesh, &binding.primitive};
    for (int i = 0; i < 4; ++i) {
        const auto& value = json[names[i]];
        const auto index = value.AsInt(-1);
        if (!value.IsNumber() || index < 0 || index > std::numeric_limits<int>::max())
            return Fail(error, Join(path, names[i]), "must be a nonnegative bounded integer");
        *destinations[i] = static_cast<int>(index);
    }
    const auto& frame = json["meshBind"];
    if (!frame.IsArray() || frame.Size() != 16) return Fail(error, path, "meshBind needs sixteen row-major numbers");
    for (std::size_t i = 0; i < 16; ++i) {
        const double value = frame[i].AsNumber(std::numeric_limits<double>::quiet_NaN());
        if (!std::isfinite(value) || std::fabs(value) > std::numeric_limits<float>::max())
            return Fail(error, path, "meshBind needs finite Float32 numbers");
        if (i >= 12 && value != (i == 15 ? 1.0 : 0.0)) return Fail(error, path, "meshBind must be affine");
        binding.meshBind.m[i / 4][i % 4] = static_cast<float>(value);
    }
    Mat4 inverse;
    return AffineInverse(binding.meshBind, inverse) || Fail(error, path, "meshBind must be finite, affine and invertible");
}

JsonValue AnimatorBindingJson(const ImportedAnimatorBinding& binding) {
    JsonValue json = JsonValue::MakeObject();
    json.Set("role", binding.role == ImportedAnimatorRole::Controller ? "controller" : "primitive");
    json.Set("model", binding.model);
    if (binding.role == ImportedAnimatorRole::Primitive) {
        json.Set("node", binding.node); json.Set("skin", binding.skin);
        json.Set("mesh", binding.mesh); json.Set("primitive", binding.primitive);
        JsonValue matrix = JsonValue::MakeArray();
        for (const auto& row : binding.meshBind.m) for (float value : row) matrix.Append(value);
        json.Set("meshBind", matrix);
    }
    return json;
}

} // namespace

SceneSerializer::SceneSerializer(Assets::AssetManager* assets, const BehaviourRegistry& behaviours)
    : assets_(assets), behaviours_(behaviours) {}

// ------------------------------------------------------------------ resources

bool SceneSerializer::ResolveMesh(const std::string& ref, const std::string& path, ParseState& state) const {
    SceneDocument& document = state.document;
    if (document.meshes.count(ref)) return true;
    if (ref.rfind("primitive:", 0) == 0) {
        auto mesh = MakePrimitiveMesh(ref.substr(10));
        if (!mesh) return Fail(state.error, path, "unknown primitive '" + ref + "' (cube, sphere, plane, capsule, cylinder)");
        document.meshes[ref] = std::move(mesh);
        return true;
    }
    std::string file, fragment;
    SplitRef(ref, file, fragment);
    if (!assets_) return Fail(state.error, path, "mesh '" + ref + "' needs an asset manager");
    const auto handle = assets_->Load<Assets::GltfDocument>(file);
    if (!state.Current(handle)) return Fail(state.error, path, "cannot load model '" + file + "': " + state.Diagnostic(handle));
    std::size_t meshIndex = 0, primitiveIndex = 0;
    if (!fragment.empty()) {
        const std::size_t dot = fragment.find('.');
        const bool ok = ParseIndex(fragment.substr(0, dot), meshIndex)
            && (dot == std::string::npos || ParseIndex(fragment.substr(dot + 1), primitiveIndex));
        if (!ok) return Fail(state.error, path, "mesh reference '" + ref + "' is not model#mesh.primitive");
    }
    const Assets::GltfDocument& gltf = *handle.Get();
    if (meshIndex >= gltf.meshes.size() || primitiveIndex >= gltf.meshes[meshIndex].primitives.size()) {
        return Fail(state.error, path, "model '" + file + "' has no mesh " + fragment);
    }
    auto mesh = std::make_shared<Graphics::MeshData>(gltf.meshes[meshIndex].primitives[primitiveIndex].mesh);
    if (mesh->name.empty()) mesh->name = ref;
    document.meshes[ref] = std::move(mesh);
    return true;
}

bool SceneSerializer::ResolveMaterial(const std::string& ref, const std::string& path, ParseState& state) const {
    SceneDocument& document = state.document;
    if (document.materials.count(ref)) return true;
    std::string file, fragment;
    SplitRef(ref, file, fragment);
    if (ref.find('#') == std::string::npos) return Fail(state.error, path, "unknown material '" + ref + "'");
    if (!assets_) return Fail(state.error, path, "material '" + ref + "' needs an asset manager");
    const auto handle = assets_->Load<Assets::GltfDocument>(file);
    if (!state.Current(handle)) return Fail(state.error, path, "cannot load model '" + file + "': " + state.Diagnostic(handle));
    std::size_t index = 0;
    if (fragment.rfind("material", 0) != 0 || !ParseIndex(fragment.substr(8), index)) {
        return Fail(state.error, path, "material reference '" + ref + "' is not model#materialN");
    }
    const Assets::GltfDocument& gltf = *handle.Get();
    if (index >= gltf.materials.size()) return Fail(state.error, path, "model '" + file + "' has no " + fragment);
    const Assets::GltfMaterial& source = gltf.materials[index];
    std::shared_ptr<Graphics::Texture2D> texture;
    if (source.baseColorTexture >= 0 && static_cast<std::size_t>(source.baseColorTexture) < gltf.textures.size()) {
        const int image = gltf.textures[static_cast<std::size_t>(source.baseColorTexture)].image;
        if (image >= 0 && static_cast<std::size_t>(image) < gltf.images.size()
            && !gltf.images[static_cast<std::size_t>(image)].image.pixels.empty()) {
            texture = std::make_shared<Graphics::Texture2D>();
            texture->FromImage(gltf.images[static_cast<std::size_t>(image)].image, true, true);
        }
    }
    // The engine's house style: glTF materials become toon materials (unlit stays unlit).
    Graphics::Material material = Assets::BuildMaterial(source, texture.get(), Graphics::ShadingModel::Toon);
    if (material.name.empty()) material.name = ref;
    document.materials[ref] = MakeMaterial(std::move(material), std::move(texture));
    return true;
}

bool SceneSerializer::ResolveClip(const std::string& ref, const std::string& path, ParseState& state) const {
    SceneDocument& document = state.document;
    if (document.clips.count(ref)) return true;
    if (!assets_) return Fail(state.error, path, "sound '" + ref + "' needs an asset manager");
    const auto handle = assets_->Load<Audio::AudioClip>(ref);
    if (!state.Current(handle)) return Fail(state.error, path, "cannot load sound '" + ref + "': " + state.Diagnostic(handle));
    document.clips[ref] = std::make_shared<Audio::AudioClip>(*handle.Get());
    return true;
}

// ------------------------------------------------------------------ parsing

bool SceneSerializer::Parse(const JsonValue& json, SceneDocument& out, std::string& error,
    std::vector<Assets::AssetDependency>* attempted, bool strictReload) const {
    SceneDocument document;
    std::vector<Assets::AssetDependency> local;
    ParseState state(document, error, attempted ? *attempted : local, strictReload);
    if (!json.IsObject()) return Fail(error, "", "a scene must be a JSON object");
    for (const std::string& key : json.Keys()) {
        if (key != "format" && key != "version" && key != "settings" && key != "environment" && key != "materials"
            && key != "prefabs" && key != "entities") {
            return Fail(error, key, "unknown key");
        }
    }
    if (const JsonValue* format = json.Find("format"); format && (!format->IsString() || format->AsString() != kSceneFormat)) {
        return Fail(error, "format", std::string("must be \"") + kSceneFormat + "\"");
    }
    if (const JsonValue* version = json.Find("version");
        version && (!version->IsNumber() || version->AsNumber() < 1 || version->AsNumber() > kSceneVersion)) {
        return Fail(error, "version", "unsupported version (this build reads version " + std::to_string(kSceneVersion) + ")");
    }
    if (const JsonValue* settings = json.Find("settings")) {
        if (!settings->IsObject()) return Fail(error, "settings", "must be an object");
        for (const std::string& key : settings->Keys()) {
            const JsonValue& value = (*settings)[key];
            if (key == "gravity") {
                if (!ReadVec3(value, document.gravity)) return Fail(error, "settings.gravity", "must be [x, y, z]");
                document.hasGravity = true;
            } else if (key == "fixedDelta") {
                if (!value.IsNumber() || !(value.AsNumber() > 1.0e-4 && value.AsNumber() <= 1.0)) {
                    return Fail(error, "settings.fixedDelta", "must be a number in (0.0001, 1]");
                }
                document.fixedDelta = value.AsFloat();
            } else if (key == "timeScale") {
                if (!value.IsNumber() || !(value.AsNumber() >= 0.0 && value.AsNumber() <= 100.0)) {
                    return Fail(error, "settings.timeScale", "must be a number in [0, 100]");
                }
                document.timeScale = value.AsFloat();
            } else {
                return Fail(error, Join("settings", key), "unknown setting");
            }
        }
    }
    if (const JsonValue* environment = json.Find("environment")) {
        std::string fieldError;
        if (!environment->IsObject()) return Fail(error, "environment", "must be an object");
        if (!FrameworkTypes().Find("Environment")->FromJson(&document.environment, *environment, fieldError, true)) {
            return Fail(error, "environment", WithoutType(fieldError, "Environment"));
        }
        document.hasEnvironment = true;
    }
    if (const JsonValue* materials = json.Find("materials")) {
        if (!materials->IsObject()) return Fail(error, "materials", "must be an object");
        for (const std::string& name : materials->Keys()) {
            const std::string path = Join("materials", name);
            if (name.empty() || name.find('#') != std::string::npos || name.rfind("primitive:", 0) == 0) {
                return Fail(error, path, "material names may not be empty, contain '#' or start with 'primitive:'");
            }
            const JsonValue& source = (*materials)[name];
            if (!source.IsObject()) return Fail(error, path, "must be an object");
            Graphics::Material material;
            material.name = name;
            std::string fieldError;
            if (!FrameworkTypes().Find("Material")->FromJson(&material, source, fieldError, true)) {
                return Fail(error, path, WithoutType(fieldError, "Material"));
            }
            std::shared_ptr<Graphics::Texture2D> texture;
            if (!material.baseTexturePath.empty()) {
                if (!assets_) return Fail(error, Join(path, "texture"), "textures need an asset manager");
                const auto handle = assets_->Load<Graphics::Texture2D>(material.baseTexturePath);
                if (!state.Current(handle)) {
                    return Fail(error, Join(path, "texture"), "cannot load '" + material.baseTexturePath + "': " + state.Diagnostic(handle));
                }
                texture = std::make_shared<Graphics::Texture2D>(*handle.Get());
            }
            document.materials[name] = MakeMaterial(std::move(material), std::move(texture));
            document.materialSources[name] = source;
        }
    }
    if (const JsonValue* prefabs = json.Find("prefabs")) {
        if (!prefabs->IsObject()) return Fail(error, "prefabs", "must be an object");
        state.prefabSources = prefabs;
        for (const std::string& name : prefabs->Keys()) {
            if (!ParsePrefab(name, Join("prefabs", name), state)) return false;
        }
    }
    if (const JsonValue* entities = json.Find("entities")) {
        if (!entities->IsArray()) return Fail(error, "entities", "must be an array");
        for (std::size_t i = 0; i < entities->Size(); ++i) {
            EntitySpec spec;
            SpecExtent extent;
            if (!ParseEntity((*entities)[i], At("entities", i), state, spec, 0, false, extent)) return false;
            document.entities.push_back(std::move(spec));
        }
    }
    for (std::size_t i = 0; i < document.entities.size(); ++i)
        if (!ResolveImportedAnimations(document.entities[i], At("entities", i), state)) return false;
    for (auto& [name, prefab] : document.prefabs)
        if (!ResolveImportedAnimations(prefab, Join("prefabs", name), state)) return false;
    document.dependencies = state.dependencies;
    out = std::move(document);
    return true;
}

bool SceneSerializer::ParseFile(const std::string& path, SceneDocument& out, std::string& error,
    std::vector<Assets::AssetDependency>* attempted, bool strictReload) const {
    JsonValue json;
    std::vector<Assets::AssetDependency> local;
    auto& dependencies = attempted ? *attempted : local;
    if (assets_) {
        const auto handle = assets_->Load<JsonValue>(path);
        const auto dependency = handle.Dependency();
        if (std::find(dependencies.begin(), dependencies.end(), dependency) == dependencies.end())
            dependencies.push_back(dependency);
        if (!handle.Ready() || !handle.Error().empty()) return Fail(error, path, handle.Error());
        json = *handle.Get();
    } else if (!Core::ReadJsonFile(path, json, error)) {
        return Fail(error, path, error);
    }
    return Parse(json, out, error, &dependencies, strictReload);
}

bool SceneSerializer::ParsePrefab(const std::string& name, const std::string& path, ParseState& state) const {
    SceneDocument& document = state.document;
    if (document.prefabs.count(name)) return true;
    if (state.resolving.count(name)) return Fail(state.error, path, "prefab '" + name + "' contains itself");
    if (state.resolving.size() > maxDepth)
        return Fail(state.error, path, "prefab resolution deeper than " + std::to_string(maxDepth));
    const JsonValue* source = state.prefabSources ? state.prefabSources->Find(name) : nullptr;
    JsonValue file;
    if (!source) {
        const bool isFile = name.size() > 5 && name.compare(name.size() - 5, 5, ".json") == 0;
        if (!isFile) return Fail(state.error, path, "unknown prefab '" + name + "'");
        if (!assets_) return Fail(state.error, path, "prefab file '" + name + "' needs an asset manager");
        const auto handle = assets_->Load<JsonValue>(name);
        if (!state.Current(handle)) return Fail(state.error, path, "cannot load prefab '" + name + "': " + state.Diagnostic(handle));
        file = *handle.Get();
        if (!file.IsObject() || !file.Find("entity")) {
            return Fail(state.error, path, "prefab file '" + name + "' must hold {\"entity\": {...}}");
        }
        for (const std::string& key : file.Keys()) {
            if (key != "entity" && key != "materials") return Fail(state.error, Join(name, key), "unknown key");
        }
        if (const JsonValue* materials = file.Find("materials")) {
            // Prefab materials join the document; the scene's own definitions win.
            JsonValue scene = JsonValue::MakeObject();
            scene.Set("materials", *materials);
            SceneDocument prefabDocument;
            std::string materialError;
            if (!Parse(scene, prefabDocument, materialError, &state.dependencies, state.strictReload))
                return Fail(state.error, name, materialError);
            for (auto& entry : prefabDocument.materials) document.materials.emplace(entry.first, entry.second);
            for (auto& entry : prefabDocument.materialSources) document.materialSources.emplace(entry.first, entry.second);
        }
        source = file.Find("entity");
    }
    state.resolving.insert(name);
    EntitySpec spec;
    SpecExtent extent;
    const bool ok = ParseEntity(*source, path, state, spec, 0, true, extent);
    state.resolving.erase(name);
    if (!ok) return false;
    state.prefabExtents[name] = extent;
    document.prefabs[name] = std::move(spec);
    return true;
}

bool SceneSerializer::Admit(const SpecExtent& extent, std::size_t depth, bool cached, ParseState& state,
    const std::string& path) const {
    if (depth > maxDepth || extent.height > maxDepth - depth)
        return Fail(state.error, path, "hierarchy deeper than " + std::to_string(maxDepth));
    std::size_t& used = cached ? state.cachedEntities : state.entities;
    if (used > maxEntities || extent.nodes > maxEntities - used)
        return Fail(state.error, path, "more than " + std::to_string(maxEntities)
            + (cached ? " cached prefab entities" : " entities"));
    used += extent.nodes;
    return true;
}

bool SceneSerializer::ParseEntity(const JsonValue& json, const std::string& path, ParseState& state, EntitySpec& out,
    std::size_t depth, bool cached, SpecExtent& parsedExtent) const {
    std::string& error = state.error;
    if (depth > maxDepth) return Fail(error, path, "hierarchy deeper than " + std::to_string(maxDepth));
    if (!json.IsObject()) return Fail(error, path, "an entity must be an object");
    for (const std::string& key : json.Keys()) {
        if (key != "name" && key != "transform" && key != "components" && key != "behaviours" && key != "children"
            && key != "prefab" && key != "model" && key != "collider") {
            return Fail(error, Join(path, key), "unknown key");
        }
    }
    const EntitySpec* source = nullptr;
    SpecExtent extent;
    if (const JsonValue* prefab = json.Find("prefab")) {
        if (!prefab->IsString()) return Fail(error, Join(path, "prefab"), "must be a prefab name");
        if (!ParsePrefab(prefab->AsString(), Join(path, "prefab"), state)) return false;
        source = &state.document.prefabs.at(prefab->AsString());
        extent = state.prefabExtents.at(prefab->AsString());
    }
    // A reference replaces this root rather than adding an extra wrapper. Its
    // entire already-expanded subtree must fit before recursive vector copying.
    if (!Admit(extent, depth, cached, state, source ? Join(path, "prefab") : path)) return false;
    EntitySpec spec;
    if (source) spec = *source;
    if (const JsonValue* model = json.Find("model")) {
        if (!model->IsString()) return Fail(error, Join(path, "model"), "must be a model path");
        bool colliders = false;
        if (const JsonValue* collider = json.Find("collider")) {
            if (!collider->IsString() || (collider->AsString() != "mesh" && collider->AsString() != "none")) {
                return Fail(error, Join(path, "collider"), "must be \"mesh\" or \"none\"");
            }
            colliders = collider->AsString() == "mesh";
        }
        if (!ExpandModel(model->AsString(), colliders, Join(path, "model"), state, spec, depth, cached, extent)) return false;
        spec.modelSource = model->AsString();
    } else if (json.Find("collider")) {
        return Fail(error, Join(path, "collider"), "only applies to \"model\" entities");
    }
    if (const JsonValue* name = json.Find("name")) {
        if (!name->IsString()) return Fail(error, Join(path, "name"), "must be a string");
        spec.name = name->AsString();
    }
    if (const JsonValue* transform = json.Find("transform")) {
        spec.transform = {};
        if (!ParseTransform(*transform, Join(path, "transform"), spec.transform, error)) return false;
    }
    if (const JsonValue* components = json.Find("components")) {
        if (!ParseComponents(*components, Join(path, "components"), state, spec)) return false;
    }
    if (const JsonValue* behaviours = json.Find("behaviours")) {
        if (!behaviours->IsArray()) return Fail(error, Join(path, "behaviours"), "must be an array");
        for (std::size_t i = 0; i < behaviours->Size(); ++i) {
            const JsonValue& entry = (*behaviours)[i];
            const std::string entryPath = At(Join(path, "behaviours"), i);
            if (!entry.IsObject() || !entry.Find("type") || !entry["type"].IsString()) {
                return Fail(error, entryPath, "must be an object with a \"type\"");
            }
            BehaviourSpec behaviour;
            behaviour.type = entry["type"].AsString();
            behaviour.properties = entry;
            behaviour.properties.Erase("type");
            if (const JsonValue* enabled = entry.Find("enabled")) {
                if (!enabled->IsBool()) return Fail(error, Join(entryPath, "enabled"), "must be true or false");
                behaviour.enabled = enabled->AsBool();
                behaviour.properties.Erase("enabled");
            }
            std::string behaviourError;
            if (!behaviours_.Validate(behaviour.type, behaviour.properties, behaviourError)) {
                return Fail(error, entryPath, behaviourError);
            }
            spec.behaviours.push_back(std::move(behaviour));
        }
    }
    if (const JsonValue* children = json.Find("children")) {
        if (!children->IsArray()) return Fail(error, Join(path, "children"), "must be an array");
        if (children->Size() != 0 && depth == maxDepth)
            return Fail(error, At(Join(path, "children"), 0), "hierarchy deeper than " + std::to_string(maxDepth));
        for (std::size_t i = 0; i < children->Size(); ++i) {
            EntitySpec child;
            SpecExtent childExtent;
            if (!ParseEntity((*children)[i], At(Join(path, "children"), i), state, child, depth + 1, cached, childExtent)) return false;
            // Admission already bounded the domain sum; child admission also
            // proved height <= maxDepth - (depth + 1), so these cannot wrap.
            extent.nodes += childExtent.nodes;
            extent.height = std::max(extent.height, childExtent.height + 1);
            spec.children.push_back(std::move(child));
        }
    }
    parsedExtent = extent;
    out = std::move(spec);
    return true;
}

bool SceneSerializer::ParseComponents(const JsonValue& json, const std::string& path, ParseState& state, EntitySpec& spec) const {
    std::string& error = state.error;
    if (!json.IsObject()) return Fail(error, path, "must be an object");
    for (const std::string& name : json.Keys()) {
        const std::string componentPath = Join(path, name);
        if (!IsComponentName(name)) return Fail(error, componentPath, "unknown component");
        const JsonValue& fields = json[name];
        if (!fields.IsObject()) return Fail(error, componentPath, "must be an object");
        // Instance fields override a prefab's, key by key.
        JsonValue merged = spec.components.Find(name) ? spec.components[name] : JsonValue::MakeObject();
        for (const std::string& key : fields.Keys()) merged.Set(key, fields[key]);
        const Core::TypeInfo* info = FrameworkTypes().Find(name);
        void* scratch = info->create();
        std::string fieldError;
        const bool ok = info->FromJson(scratch, name == "Animator" ? AnimatorPlayback(merged) : merged, fieldError, true);
        info->destroy(scratch);
        if (!ok) return Fail(error, componentPath, WithoutType(fieldError, name));
        spec.components.Set(name, merged);
    }
    // Resolve what the merged components reference.
    std::string rendererMesh;
    if (const JsonValue* renderer = spec.components.Find("MeshRenderer")) {
        rendererMesh = renderer->String("mesh");
        const std::string material = renderer->String("material");
        if (!rendererMesh.empty() && !ResolveMesh(rendererMesh, Join(Join(path, "MeshRenderer"), "mesh"), state)) return false;
        if (renderer->Bool("autoLod") && !rendererMesh.empty() && !state.document.lodGroups.count(rendererMesh)) {
            // One LOD chain per mesh, shared by every instance.
            state.document.lodGroups[rendererMesh] =
                std::make_shared<const Graphics::LodGroup>(Graphics::BuildLodGroup(state.document.meshes.at(rendererMesh)));
        }
        if (!material.empty() && !ResolveMaterial(material, Join(Join(path, "MeshRenderer"), "material"), state)) return false;
    }
    if (const JsonValue* collider = spec.components.Find("Collider")) {
        if (collider->String("shape", "Box") == "Mesh") {
            const std::string ref = collider->String("mesh").empty() ? rendererMesh : collider->String("mesh");
            const std::string meshPath = Join(Join(path, "Collider"), "mesh");
            if (ref.empty()) return Fail(error, meshPath, "a Mesh collider needs a mesh (its own or the MeshRenderer's)");
            if (!ResolveMesh(ref, meshPath, state)) return false;
            if (!state.document.collisionMeshes.count(ref)) {
                std::string cookError;
                auto cooked = CookCollisionMesh(*state.document.meshes.at(ref), {1.0f, 1.0f, 1.0f}, cookError);
                if (!cooked) return Fail(error, meshPath, cookError);
                state.document.collisionMeshes[ref] = std::move(cooked);
            }
        }
    }
    if (const JsonValue* source = spec.components.Find("AudioSource")) {
        const std::string clip = source->String("clip");
        if (!clip.empty() && !ResolveClip(clip, Join(Join(path, "AudioSource"), "clip"), state)) return false;
    }
    return true;
}

bool SceneSerializer::ExpandModel(const std::string& model, bool colliders, const std::string& path, ParseState& state,
    EntitySpec& out, std::size_t depth, bool cached, SpecExtent& extent) const {
    std::string& error = state.error;
    if (!assets_) return Fail(error, path, "model '" + model + "' needs an asset manager");
    const auto handle = assets_->Load<Assets::GltfDocument>(model);
    if (!state.Current(handle)) return Fail(error, path, "cannot load model '" + model + "': " + state.Diagnostic(handle));
    const Assets::GltfDocument& gltf = *handle.Get();
    // Count the exact selected hierarchy and generated primitive children before
    // constructing EntitySpecs. Pending node indices are charged before enqueue.
    const std::size_t used = cached ? state.cachedEntities : state.entities;
    const std::size_t available = maxEntities - used; // prior root admission bounded used
    const std::size_t remainingDepth = maxDepth - depth;
    SpecExtent expansion{0, 0};
    const auto add = [&](std::size_t count, std::size_t relativeDepth) {
        if (relativeDepth > remainingDepth)
            return Fail(error, path, "model hierarchy deeper than " + std::to_string(maxDepth));
        if (count > available - expansion.nodes)
            return Fail(error, path, "more than " + std::to_string(maxEntities)
                + (cached ? " cached prefab entities" : " entities"));
        expansion.nodes += count;
        expansion.height = std::max(expansion.height, relativeDepth);
        return true;
    };
    const std::vector<int> roots = gltf.RootNodes();
    std::vector<std::pair<int, std::size_t>> pending;
    if (!roots.empty()) {
        if (!add(roots.size(), 1)) return false;
        for (int root : roots) pending.emplace_back(root, 1);
    }
    while (!pending.empty()) {
        const auto next = pending.back();
        pending.pop_back();
        const Assets::GltfNode& source = gltf.nodes[static_cast<std::size_t>(next.first)];
        if (source.mesh >= 0) {
            const std::size_t primitives = gltf.meshes[static_cast<std::size_t>(source.mesh)].primitives.size();
            if (primitives > 1) {
                if (next.second == remainingDepth)
                    return Fail(error, path, "model hierarchy deeper than " + std::to_string(maxDepth));
                if (!add(primitives - 1, next.second + 1)) return false;
            }
        }
        if (!source.children.empty()) {
            if (next.second == remainingDepth)
                return Fail(error, path, "model hierarchy deeper than " + std::to_string(maxDepth));
            if (!add(source.children.size(), next.second + 1)) return false;
            for (int child : source.children) pending.emplace_back(child, next.second + 1);
        }
    }
    if (!Admit(expansion, depth, cached, state, path)) return false;
    if (out.name.empty()) out.name = Stem(model);

    auto addMesh = [&](EntitySpec& spec, std::size_t mesh, std::size_t primitive, int nodeIndex) {
        const std::string meshRef = model + "#" + std::to_string(mesh) + "." + std::to_string(primitive);
        const int material = gltf.meshes[mesh].primitives[primitive].material;
        const std::string materialRef = material >= 0 ? model + "#material" + std::to_string(material) : std::string();
        if (!ResolveMesh(meshRef, path, state)) return false;
        if (!materialRef.empty() && !ResolveMaterial(materialRef, path, state)) return false;
        JsonValue renderer = JsonValue::MakeObject();
        renderer.Set("mesh", meshRef);
        if (!materialRef.empty()) renderer.Set("material", materialRef);
        spec.components.Set("MeshRenderer", renderer);
        if (gltf.nodes[static_cast<std::size_t>(nodeIndex)].skin >= 0) {
            spec.modelPrimitive = {ImportedAnimatorRole::Primitive, model, nodeIndex,
                gltf.nodes[static_cast<std::size_t>(nodeIndex)].skin, static_cast<int>(mesh),
                static_cast<int>(primitive), gltf.WorldMatrix(nodeIndex)};
        }
        if (colliders) {
            std::string cookError;
            if (!state.document.collisionMeshes.count(meshRef)) {
                auto cooked = CookCollisionMesh(*state.document.meshes.at(meshRef), {1.0f, 1.0f, 1.0f}, cookError);
                if (!cooked) return Fail(error, path, cookError);
                state.document.collisionMeshes[meshRef] = std::move(cooked);
            }
            JsonValue collider = JsonValue::MakeObject();
            collider.Set("shape", "Mesh");
            spec.components.Set("Collider", collider);
        }
        return true;
    };

    std::function<bool(int, EntitySpec&)> node = [&](int index, EntitySpec& spec) {
        const Assets::GltfNode& source = gltf.nodes[static_cast<std::size_t>(index)];
        spec.name = source.name.empty() ? "node" + std::to_string(index) : source.name;
        spec.transform = {source.local.translation, source.local.rotation, source.local.scale};
        if (source.mesh >= 0) {
            const auto mesh = static_cast<std::size_t>(source.mesh);
            for (std::size_t p = 0; p < gltf.meshes[mesh].primitives.size(); ++p) {
                if (p == 0) {
                    if (!addMesh(spec, mesh, 0, index)) return false;
                } else {
                    // Extra primitives (other materials) become child entities.
                    EntitySpec part;
                    part.name = spec.name + ".primitive" + std::to_string(p);
                    if (!addMesh(part, mesh, p, index)) return false;
                    spec.children.push_back(std::move(part));
                }
            }
        }
        for (int child : source.children) {
            EntitySpec childSpec;
            if (!node(child, childSpec)) return false;
            spec.children.push_back(std::move(childSpec));
        }
        return true;
    };
    for (int root : roots) {
        EntitySpec rootSpec;
        if (!node(root, rootSpec)) return false;
        out.children.push_back(std::move(rootSpec));
    }
    // Whole-expansion admission bounded this addition before any construction.
    extent.nodes += expansion.nodes;
    extent.height = std::max(extent.height, expansion.height);
    return true;
}

bool SceneSerializer::ResolveImportedAnimations(EntitySpec& spec, const std::string& path, ParseState& state,
    std::shared_ptr<ImportedModelAnimation> controller, const JsonValue* controls) const {
    const std::string componentPath = Join(path, "components.Animator");
    JsonValue* fields = spec.components.Find("Animator");
    ImportedAnimatorBinding binding;
    bool explicitBinding = fields && fields->Has("binding");
    if (explicitBinding && !ReadAnimatorBinding((*fields)["binding"], binding, state.error, componentPath)) return false;
    if (explicitBinding && !spec.modelSource.empty()) return Fail(state.error, componentPath, "model and flattened binding cannot be combined");
    const bool newController = (fields && !spec.modelSource.empty()) || binding.role == ImportedAnimatorRole::Controller;
    if (!newController && !spec.modelSource.empty()) { controller.reset(); controls = nullptr; }
    if (newController) {
        if (fields->Bool("applyRootMotion")) return Fail(state.error, componentPath, "imported root motion is not supported");
        if (const auto* collider = spec.components.Find("Collider"); collider && collider->String("shape") == "Mesh")
            return Fail(state.error, componentPath, "imported animation does not deform mesh collision");
        binding.role = ImportedAnimatorRole::Controller;
        if (!spec.modelSource.empty()) binding.model = spec.modelSource;
        if (!assets_) return Fail(state.error, componentPath, "imported animation needs an asset manager");
        const auto handle = assets_->Load<Assets::GltfDocument>(binding.model);
        if (!state.Current(handle)) return Fail(state.error, componentPath, state.Diagnostic(handle));
        controller = std::make_shared<ImportedModelAnimation>();
        controller->model = binding.model;
        controller->version = handle.Version();
        controller->source = std::make_shared<const Assets::GltfDocument>(*handle.Get());
        const auto& source = *controller->source;
        if (source.animations.empty()) return Fail(state.error, componentPath, "model has no animation clips");
        const std::string requested = fields->String("clip");
        int selected = 0;
        if (!requested.empty()) {
            selected = -1;
            for (std::size_t i = 0; i < source.animations.size(); ++i) if (source.animations[i].name == requested) {
                if (selected >= 0) return Fail(state.error, componentPath, "clip name is ambiguous: " + requested);
                selected = static_cast<int>(i);
            }
            if (selected < 0) return Fail(state.error, componentPath, "clip not found: " + requested);
        }
        controller->animation = selected;
        const auto& clip = source.animations[static_cast<std::size_t>(selected)];
        // Preserve the authored selector. Empty deliberately means automatic
        // first, including when the importer supplies a generated display name.
        controller->selector = requested;
        std::set<int> jointNodes;
        std::vector<int> pending = source.RootNodes();
        while (!pending.empty()) {
            const int node = pending.back(); pending.pop_back();
            if (node < 0 || static_cast<std::size_t>(node) >= source.nodes.size()) return Fail(state.error, componentPath, "invalid model node");
            const auto& item = source.nodes[static_cast<std::size_t>(node)];
            if (item.mesh >= 0 && item.skin >= 0) {
                if (static_cast<std::size_t>(item.skin) >= source.skins.size()) return Fail(state.error, componentPath, "invalid skin");
                const auto& joints = source.skins[static_cast<std::size_t>(item.skin)].joints;
                jointNodes.insert(joints.begin(), joints.end());
            }
            pending.insert(pending.end(), item.children.begin(), item.children.end());
        }
        if (jointNodes.empty() || clip.channels.empty()) return Fail(state.error, componentPath, "clip needs a skinned model with joint channels");
        for (const auto& channel : clip.channels) if (!jointNodes.count(channel.node))
            return Fail(state.error, componentPath, "selected clip contains unsupported non-joint animation");
        fields->Set("clip", controller->selector);
        controls = fields;
        auto resolved = std::make_shared<ResolvedImportedAnimator>();
        resolved->binding = binding;
        resolved->owner = controller;
        spec.importedAnimator = std::move(resolved);
    } else {
        if (!explicitBinding && spec.modelPrimitive.role == ImportedAnimatorRole::Primitive && controller
            && spec.modelPrimitive.model == controller->model) binding = spec.modelPrimitive;
        if (binding.role == ImportedAnimatorRole::Primitive) {
            if (!controller || binding.model != controller->model || !controls)
                return Fail(state.error, componentPath, "primitive needs its matching ancestor Animator controller");
            if (explicitBinding) for (const auto& key : fields->Keys()) if (key != "binding")
                return Fail(state.error, componentPath, "edit playback on the ancestor Animator controller");
            const auto& source = *controller->source;
            if (binding.node < 0 || static_cast<std::size_t>(binding.node) >= source.nodes.size()
                || binding.skin < 0 || static_cast<std::size_t>(binding.skin) >= source.skins.size()
                || binding.mesh < 0 || static_cast<std::size_t>(binding.mesh) >= source.meshes.size()
                || binding.primitive < 0 || static_cast<std::size_t>(binding.primitive) >= source.meshes[static_cast<std::size_t>(binding.mesh)].primitives.size())
                return Fail(state.error, componentPath, "binding index is outside the referenced model");
            const auto& node = source.nodes[static_cast<std::size_t>(binding.node)];
            const std::string meshRef = binding.model + "#" + std::to_string(binding.mesh) + "." + std::to_string(binding.primitive);
            const auto* renderer = spec.components.Find("MeshRenderer");
            if (node.mesh != binding.mesh || node.skin != binding.skin || !renderer || renderer->String("mesh") != meshRef)
                return Fail(state.error, componentPath, "binding does not match source node, skin and renderer");
            if (renderer->Bool("autoLod")) return Fail(state.error, componentPath, "imported animation cannot use static LOD");
            if (const auto* collider = spec.components.Find("Collider"); collider && collider->String("shape") == "Mesh")
                return Fail(state.error, componentPath, "imported animation does not deform mesh collision");
            auto resolved = std::make_shared<ResolvedImportedAnimator>();
            resolved->binding = binding;
            resolved->owner = controller;
            if (!AffineInverse(binding.meshBind, resolved->modelToMesh)) return Fail(state.error, componentPath, "meshBind must be finite, affine and invertible");
            auto found = controller->rigs.find(binding.skin);
            if (found == controller->rigs.end()) {
                Assets::GltfSkeletonBinding skeleton;
                std::string conversionError;
                if (!Assets::BuildSkeleton(source, binding.skin, skeleton, conversionError)) return Fail(state.error, componentPath, conversionError);
                if (skeleton.skeleton.JointCount() > 65536) return Fail(state.error, componentPath, "too many runtime skin joints");
                auto rig = std::make_shared<ImportedAnimationRig>();
                rig->clip = Assets::BuildClip(source, controller->animation, skeleton);
                rig->skeleton = std::move(skeleton.skeleton);
                rig->nodeToJoint = std::move(skeleton.nodeToJoint);
                if (!rig->clip.Validate(rig->skeleton, conversionError)) return Fail(state.error, componentPath, conversionError);
                Animation::AnimState stateClip;
                stateClip.name = rig->clip.name;
                stateClip.clip = &rig->clip;
                rig->machine.AddState(stateClip);
                found = controller->rigs.emplace(binding.skin, std::move(rig)).first;
            }
            resolved->rig = found->second;
            auto bindMesh = std::make_shared<Graphics::MeshData>(source.meshes[static_cast<std::size_t>(binding.mesh)].primitives[static_cast<std::size_t>(binding.primitive)].mesh);
            std::string meshError;
            if (!bindMesh->Validate(meshError)) return Fail(state.error, componentPath, meshError);
            if (bindMesh->skin.size() != bindMesh->vertices.size() || bindMesh->skin.empty()) return Fail(state.error, componentPath, "primitive has no skin influences");
            const auto& skin = source.skins[static_cast<std::size_t>(binding.skin)];
            for (auto& influence : bindMesh->skin) for (std::size_t i = 0; i < 4; ++i) {
                if (influence.weights[i] <= 0) { influence.joints[i] = 0; continue; }
                const std::size_t slot = influence.joints[i];
                if (slot >= skin.joints.size()) return Fail(state.error, componentPath, "skin influence references a missing slot");
                const int jointNode = skin.joints[slot];
                if (jointNode < 0 || static_cast<std::size_t>(jointNode) >= resolved->rig->nodeToJoint.size()) return Fail(state.error, componentPath, "skin joint node is missing");
                const int joint = resolved->rig->nodeToJoint[static_cast<std::size_t>(jointNode)];
                if (joint < 0 || joint > 65535) return Fail(state.error, componentPath, "skin joint cannot fit the runtime mesh");
                influence.joints[i] = static_cast<std::uint16_t>(joint);
            }
            resolved->bindMesh = std::move(bindMesh);
            spec.importedAnimator = std::move(resolved);
            JsonValue inherited = AnimatorPlayback(*controls);
            inherited.Set("binding", AnimatorBindingJson(binding));
            spec.components.Set("Animator", inherited);
        } else if (fields && !fields->String("clip").empty()) {
            return Fail(state.error, componentPath, "a clip needs a model root or imported binding");
        }
    }
    for (std::size_t i = 0; i < spec.children.size(); ++i)
        if (!ResolveImportedAnimations(spec.children[i], At(Join(path, "children"), i), state, controller, controls)) return false;
    if (newController) {
        const auto current = assets_->Load<Assets::GltfDocument>(controller->model);
        if (!state.Current(current) || current.Version() != controller->version)
            return Fail(state.error, componentPath, "model changed during animation binding");
        if (controller->rigs.empty()) return Fail(state.error, componentPath, "controller has no bound skinned primitives");
    }
    return true;
}

// ------------------------------------------------------------------ instancing

std::vector<Entity> SceneSerializer::Instantiate(const SceneDocument& document, GameWorld& world, Entity parent,
    bool applySettings) const {
    if (applySettings) {
        if (document.hasGravity) world.Physics().Settings().gravity = document.gravity;
        if (document.fixedDelta > 0.0f) world.Settings().fixedDelta = document.fixedDelta;
        if (document.timeScale >= 0.0f) world.Settings().timeScale = document.timeScale;
        if (document.hasEnvironment) world.Env() = document.environment;
    }
    std::vector<Entity> roots;
    world.BeginCreateBatch();
    for (const EntitySpec& spec : document.entities) roots.push_back(Spawn(spec, document, world, parent));
    world.EndCreateBatch();
    return roots;
}

Entity SceneSerializer::InstantiatePrefab(const SceneDocument& document, const std::string& prefab, GameWorld& world,
    const TRS& transform, Entity parent) const {
    const auto found = document.prefabs.find(prefab);
    if (found == document.prefabs.end()) return {};
    EntitySpec spec = found->second;
    spec.transform.position = transform.translation;
    spec.transform.rotation = Normalize(transform.rotation);
    spec.transform.scale = Multiply(spec.transform.scale, transform.scale);
    world.BeginCreateBatch();
    const Entity entity = Spawn(spec, document, world, parent);
    world.EndCreateBatch();
    return entity;
}

Entity SceneSerializer::Spawn(const EntitySpec& spec, const SceneDocument& document, GameWorld& world, Entity parent) const {
    TRS local;
    local.translation = spec.transform.position;
    local.rotation = spec.transform.rotation;
    local.scale = spec.transform.scale;
    const Entity entity = world.CreateEntity(spec.name, local, parent);
    AddComponents(spec, document, world, entity);
    for (const BehaviourSpec& behaviourSpec : spec.behaviours) {
        std::string error;
        if (Behaviour* behaviour = world.AddBehaviour(entity, behaviourSpec.type, behaviourSpec.properties, error, behaviours_)) {
            behaviour->enabled = behaviourSpec.enabled;
        }
    }
    for (const EntitySpec& child : spec.children) Spawn(child, document, world, entity);
    return entity;
}

void SceneSerializer::AddComponents(const EntitySpec& spec, const SceneDocument& document, GameWorld& world, Entity entity) const {
    const Core::TypeRegistry& types = FrameworkTypes();
    auto fill = [&](const char* name, void* component) {
        std::string error; // validated during Parse
        types.Find(name)->FromJson(component, spec.components[name], error, true);
    };
    std::string rendererMesh;
    if (spec.components.Find("MeshRenderer")) {
        MeshRenderer renderer;
        fill("MeshRenderer", &renderer);
        renderer.mesh = Lookup(document.meshes, renderer.meshRef);
        renderer.material = Lookup(document.materials, renderer.materialRef);
        if (renderer.autoLod) renderer.lods = Lookup(document.lodGroups, renderer.meshRef);
        rendererMesh = renderer.meshRef;
        world.Add<MeshRenderer>(entity, std::move(renderer));
    }
    if (spec.components.Find("Light")) {
        Light light;
        fill("Light", &light);
        world.Add<Light>(entity, light);
    }
    if (spec.components.Find("Camera")) {
        Camera camera;
        fill("Camera", &camera);
        world.Add<Camera>(entity, camera);
    }
    if (spec.components.Find("Collider")) {
        Collider collider;
        fill("Collider", &collider);
        if (collider.shape == ColliderShape::Mesh) {
            const std::string ref = collider.meshRef.empty() ? rendererMesh : collider.meshRef;
            collider.sourceMesh = Lookup(document.meshes, ref);
            collider.triangles = Lookup(document.collisionMeshes, ref);
        }
        world.Add<Collider>(entity, std::move(collider));
    }
    if (spec.components.Find("RigidBody")) {
        RigidBody body;
        fill("RigidBody", &body);
        world.Add<RigidBody>(entity, body);
    }
    if (spec.components.Find("CharacterMover")) {
        CharacterMover mover;
        fill("CharacterMover", &mover);
        world.Add<CharacterMover>(entity, std::move(mover));
    }
    if (spec.components.Find("AudioSource")) {
        AudioSource source;
        fill("AudioSource", &source);
        source.clip = Lookup(document.clips, source.clipRef);
        world.Add<AudioSource>(entity, std::move(source));
    }
    if (spec.components.Find("AudioListener")) {
        AudioListener listener;
        fill("AudioListener", &listener);
        world.Add<AudioListener>(entity, listener);
    }
    if (spec.components.Find("ParticleSystem")) {
        ParticleSystem particles;
        fill("ParticleSystem", &particles);
        world.Add<ParticleSystem>(entity, std::move(particles));
    }
    if (spec.components.Find("Animator")) {
        AnimatorComponent animator;
        std::string ignored;
        types.Find("Animator")->FromJson(&animator, AnimatorPlayback(spec.components["Animator"]), ignored, true);
        if (spec.importedAnimator) {
            const auto& resolved = *spec.importedAnimator;
            animator.binding = resolved.binding;
            animator.importedOwner = resolved.owner;
            if (resolved.rig) {
                animator.skeleton = std::shared_ptr<const Animation::Skeleton>(resolved.rig, &resolved.rig->skeleton);
                animator.machine = std::shared_ptr<const Animation::AnimStateMachine>(resolved.rig, &resolved.rig->machine);
                animator.bindMesh = resolved.bindMesh;
                animator.modelToMesh = resolved.modelToMesh;
                for (Entity ancestor = World::GetParent(world.Registry(), entity); !ancestor.IsNull(); ancestor = World::GetParent(world.Registry(), ancestor)) {
                    const auto* owner = world.Get<AnimatorComponent>(ancestor);
                    if (owner && owner->binding.role == ImportedAnimatorRole::Controller) { animator.importedController = ancestor; break; }
                }
                animator.animator = std::make_shared<Animation::Animator>(*animator.skeleton, *animator.machine);
                if (auto* renderer = world.Get<MeshRenderer>(entity)) PublishImportedAnimationMesh(animator, *renderer);
            }
        }
        world.Add<AnimatorComponent>(entity, std::move(animator));
    }
}

// ------------------------------------------------------------------ saving

JsonValue SceneSerializer::Save(const GameWorld& world) const {
    JsonValue root = JsonValue::MakeObject();
    root.Set("format", kSceneFormat);
    root.Set("version", kSceneVersion);
    JsonValue settings = JsonValue::MakeObject();
    settings.Set("gravity", Vec3Json(world.Physics().Settings().gravity));
    settings.Set("fixedDelta", world.Settings().fixedDelta);
    settings.Set("timeScale", world.Settings().timeScale);
    root.Set("settings", settings);
    root.Set("environment", FrameworkTypes().Find("Environment")->ToJson(&world.Env()));

    JsonValue entities = JsonValue::MakeArray();
    JsonValue materials = JsonValue::MakeObject();
    std::function<void(Entity)> collectMaterials = [&](Entity entity) {
        if (const MeshRenderer* renderer = world.Get<MeshRenderer>(entity)) {
            const std::string& ref = renderer->materialRef;
            if (renderer->material && !ref.empty() && ref.find('#') == std::string::npos && !materials.Find(ref)) {
                JsonValue material = FrameworkTypes().Find("Material")->ToJson(renderer->material.get());
                if (material.String("name") == ref) material.Erase("name");
                materials.Set(ref, material);
            }
        }
        for (Entity child : World::GetChildren(world.Registry(), entity)) collectMaterials(child);
    };
    for (Entity rootEntity : world.Roots()) {
        if (!world.IsAlive(rootEntity)) continue;
        collectMaterials(rootEntity);
        entities.Append(SaveEntity(world, rootEntity));
    }
    if (materials.Size() > 0) root.Set("materials", materials);
    root.Set("entities", entities);
    return root;
}

JsonValue SceneSerializer::SaveEntity(const GameWorld& world, Entity entity) const {
    JsonValue json = JsonValue::MakeObject();
    if (const World::Name* name = world.Get<World::Name>(entity); name && !name->value.empty()) json.Set("name", name->value);
    const TRS local = world.GetLocal(entity);
    JsonValue transform = JsonValue::MakeObject();
    transform.Set("position", Vec3Json(local.translation));
    transform.Set("rotation", QuatJson(local.rotation));
    transform.Set("scale", Vec3Json(local.scale));
    json.Set("transform", transform);

    const Core::TypeRegistry& types = FrameworkTypes();
    JsonValue components = JsonValue::MakeObject();
    auto save = [&](const char* name, const void* component) {
        if (component) components.Set(name, types.Find(name)->ToJson(component));
    };
    save("MeshRenderer", world.Get<MeshRenderer>(entity));
    save("Light", world.Get<Light>(entity));
    save("Camera", world.Get<Camera>(entity));
    save("Collider", world.Get<Collider>(entity));
    save("RigidBody", world.Get<RigidBody>(entity));
    save("CharacterMover", world.Get<CharacterMover>(entity));
    save("AudioSource", world.Get<AudioSource>(entity));
    save("AudioListener", world.Get<AudioListener>(entity));
    save("ParticleSystem", world.Get<ParticleSystem>(entity));
    if (const auto* animator = world.Get<AnimatorComponent>(entity)) {
        JsonValue fields = animator->binding.role == ImportedAnimatorRole::Primitive ? JsonValue::MakeObject()
            : types.Find("Animator")->ToJson(animator);
        if (animator->binding.role != ImportedAnimatorRole::None) fields.Set("binding", AnimatorBindingJson(animator->binding));
        components.Set("Animator", fields);
    }
    if (components.Size() > 0) json.Set("components", components);

    if (const Behaviours* behaviours = world.Get<Behaviours>(entity)) {
        JsonValue list = JsonValue::MakeArray();
        for (const auto& behaviour : behaviours->list) {
            if (behaviour->TypeName().empty() || !behaviours_.Has(behaviour->TypeName())) continue; // code-only
            JsonValue item = JsonValue::MakeObject();
            item.Set("type", behaviour->TypeName());
            if (!behaviour->enabled) item.Set("enabled", false);
            const JsonValue properties = behaviours_.Properties(*behaviour);
            for (const std::string& key : properties.Keys()) item.Set(key, properties[key]);
            list.Append(item);
        }
        if (list.Size() > 0) json.Set("behaviours", list);
    }
    JsonValue children = JsonValue::MakeArray();
    for (Entity child : World::GetChildren(world.Registry(), entity)) {
        if (world.IsAlive(child)) children.Append(SaveEntity(world, child));
    }
    if (children.Size() > 0) json.Set("children", children);
    return json;
}

} // namespace Astral::Framework
