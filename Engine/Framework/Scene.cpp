#include "Engine/Framework/Scene.h"

#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/Gltf.h"

#include <cmath>
#include <cstddef>
#include <functional>
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

struct SceneSerializer::ParseState {
    ParseState(SceneDocument& documentIn, std::string& errorIn) : document(documentIn), error(errorIn) {}
    SceneDocument& document;
    std::string& error;
    const JsonValue* prefabSources{};
    std::set<std::string> resolving;
    std::size_t entities{};
};

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
    if (!handle.Ready()) return Fail(state.error, path, "cannot load model '" + file + "': " + handle.Error());
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
    if (!handle.Ready()) return Fail(state.error, path, "cannot load model '" + file + "': " + handle.Error());
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
    if (!handle.Ready()) return Fail(state.error, path, "cannot load sound '" + ref + "': " + handle.Error());
    document.clips[ref] = std::make_shared<Audio::AudioClip>(*handle.Get());
    return true;
}

// ------------------------------------------------------------------ parsing

bool SceneSerializer::Parse(const JsonValue& json, SceneDocument& out, std::string& error) const {
    SceneDocument document;
    ParseState state(document, error);
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
                if (!handle.Ready()) {
                    return Fail(error, Join(path, "texture"), "cannot load '" + material.baseTexturePath + "': " + handle.Error());
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
            if (!ParseEntity((*entities)[i], At("entities", i), state, spec, 0)) return false;
            document.entities.push_back(std::move(spec));
        }
    }
    out = std::move(document);
    return true;
}

bool SceneSerializer::ParseFile(const std::string& path, SceneDocument& out, std::string& error) const {
    JsonValue json;
    if (assets_) {
        const auto handle = assets_->Load<JsonValue>(path);
        if (!handle.Ready()) return Fail(error, path, handle.Error());
        json = *handle.Get();
    } else if (!Core::ReadJsonFile(path, json, error)) {
        return Fail(error, path, error);
    }
    return Parse(json, out, error);
}

bool SceneSerializer::ParsePrefab(const std::string& name, const std::string& path, ParseState& state) const {
    SceneDocument& document = state.document;
    if (document.prefabs.count(name)) return true;
    if (state.resolving.count(name)) return Fail(state.error, path, "prefab '" + name + "' contains itself");
    const JsonValue* source = state.prefabSources ? state.prefabSources->Find(name) : nullptr;
    JsonValue file;
    if (!source) {
        const bool isFile = name.size() > 5 && name.compare(name.size() - 5, 5, ".json") == 0;
        if (!isFile) return Fail(state.error, path, "unknown prefab '" + name + "'");
        if (!assets_) return Fail(state.error, path, "prefab file '" + name + "' needs an asset manager");
        const auto handle = assets_->Load<JsonValue>(name);
        if (!handle.Ready()) return Fail(state.error, path, "cannot load prefab '" + name + "': " + handle.Error());
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
            if (!Parse(scene, prefabDocument, materialError)) return Fail(state.error, name, materialError);
            for (auto& entry : prefabDocument.materials) document.materials.emplace(entry.first, entry.second);
            for (auto& entry : prefabDocument.materialSources) document.materialSources.emplace(entry.first, entry.second);
        }
        source = file.Find("entity");
    }
    state.resolving.insert(name);
    EntitySpec spec;
    const bool ok = ParseEntity(*source, path, state, spec, 0);
    state.resolving.erase(name);
    if (!ok) return false;
    document.prefabs[name] = std::move(spec);
    return true;
}

bool SceneSerializer::ParseEntity(const JsonValue& json, const std::string& path, ParseState& state, EntitySpec& out,
    std::size_t depth) const {
    std::string& error = state.error;
    if (depth > maxDepth) return Fail(error, path, "hierarchy deeper than " + std::to_string(maxDepth));
    if (++state.entities > maxEntities) return Fail(error, path, "more than " + std::to_string(maxEntities) + " entities");
    if (!json.IsObject()) return Fail(error, path, "an entity must be an object");
    for (const std::string& key : json.Keys()) {
        if (key != "name" && key != "transform" && key != "components" && key != "behaviours" && key != "children"
            && key != "prefab" && key != "model" && key != "collider") {
            return Fail(error, Join(path, key), "unknown key");
        }
    }
    EntitySpec spec;
    if (const JsonValue* prefab = json.Find("prefab")) {
        if (!prefab->IsString()) return Fail(error, Join(path, "prefab"), "must be a prefab name");
        if (!ParsePrefab(prefab->AsString(), Join(path, "prefab"), state)) return false;
        spec = state.document.prefabs.at(prefab->AsString());
    }
    if (const JsonValue* model = json.Find("model")) {
        if (!model->IsString()) return Fail(error, Join(path, "model"), "must be a model path");
        bool colliders = false;
        if (const JsonValue* collider = json.Find("collider")) {
            if (!collider->IsString() || (collider->AsString() != "mesh" && collider->AsString() != "none")) {
                return Fail(error, Join(path, "collider"), "must be \"mesh\" or \"none\"");
            }
            colliders = collider->AsString() == "mesh";
        }
        if (!ExpandModel(model->AsString(), colliders, Join(path, "model"), state, spec)) return false;
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
        for (std::size_t i = 0; i < children->Size(); ++i) {
            EntitySpec child;
            if (!ParseEntity((*children)[i], At(Join(path, "children"), i), state, child, depth + 1)) return false;
            spec.children.push_back(std::move(child));
        }
    }
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
        const bool ok = info->FromJson(scratch, merged, fieldError, true);
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
    EntitySpec& out) const {
    std::string& error = state.error;
    if (!assets_) return Fail(error, path, "model '" + model + "' needs an asset manager");
    const auto handle = assets_->Load<Assets::GltfDocument>(model);
    if (!handle.Ready()) return Fail(error, path, "cannot load model '" + model + "': " + handle.Error());
    const Assets::GltfDocument& gltf = *handle.Get();
    if (out.name.empty()) out.name = Stem(model);

    auto addMesh = [&](EntitySpec& spec, std::size_t mesh, std::size_t primitive) {
        const std::string meshRef = model + "#" + std::to_string(mesh) + "." + std::to_string(primitive);
        const int material = gltf.meshes[mesh].primitives[primitive].material;
        const std::string materialRef = material >= 0 ? model + "#material" + std::to_string(material) : std::string();
        if (!ResolveMesh(meshRef, path, state)) return false;
        if (!materialRef.empty() && !ResolveMaterial(materialRef, path, state)) return false;
        JsonValue renderer = JsonValue::MakeObject();
        renderer.Set("mesh", meshRef);
        if (!materialRef.empty()) renderer.Set("material", materialRef);
        spec.components.Set("MeshRenderer", renderer);
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

    std::function<bool(int, std::size_t, EntitySpec&)> node = [&](int index, std::size_t depth, EntitySpec& spec) {
        if (depth > maxDepth) return Fail(error, path, "model hierarchy deeper than " + std::to_string(maxDepth));
        if (++state.entities > maxEntities) return Fail(error, path, "more than " + std::to_string(maxEntities) + " entities");
        const Assets::GltfNode& source = gltf.nodes[static_cast<std::size_t>(index)];
        spec.name = source.name.empty() ? "node" + std::to_string(index) : source.name;
        spec.transform = {source.local.translation, source.local.rotation, source.local.scale};
        if (source.mesh >= 0) {
            const auto mesh = static_cast<std::size_t>(source.mesh);
            for (std::size_t p = 0; p < gltf.meshes[mesh].primitives.size(); ++p) {
                if (p == 0) {
                    if (!addMesh(spec, mesh, 0)) return false;
                } else {
                    // Extra primitives (other materials) become child entities.
                    EntitySpec part;
                    part.name = spec.name + ".primitive" + std::to_string(p);
                    if (!addMesh(part, mesh, p)) return false;
                    spec.children.push_back(std::move(part));
                }
            }
        }
        for (int child : source.children) {
            EntitySpec childSpec;
            if (!node(child, depth + 1, childSpec)) return false;
            spec.children.push_back(std::move(childSpec));
        }
        return true;
    };
    for (int root : gltf.RootNodes()) {
        EntitySpec rootSpec;
        if (!node(root, 1, rootSpec)) return false;
        out.children.push_back(std::move(rootSpec));
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
        fill("Animator", &animator);
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
    save("Animator", world.Get<AnimatorComponent>(entity));
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
