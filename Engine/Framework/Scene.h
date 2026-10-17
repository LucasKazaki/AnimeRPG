#pragma once

// JSON scenes and prefabs (the role of Unity scenes/prefabs and Unreal levels
// with Blueprint instances), built on the reflected framework components.
//
//   {
//     "format": "astral-scene", "version": 1,
//     "settings":    {"gravity": [0, -9.81, 0], "fixedDelta": 0.0166667, "timeScale": 1},
//     "environment": {"ambientSky": [0.3, 0.34, 0.45], "fogDensity": 0.004},
//     "materials":   {"stone": {"shading": "Toon", "baseColor": [0.7, 0.7, 0.72], "texture": "tex/stone.png"}},
//     "prefabs":     {"Crate": {"components": {"MeshRenderer": {"mesh": "primitive:cube"},
//                                              "Collider": {"shape": "Box"}, "RigidBody": {"mass": 4}}}},
//     "entities": [
//       {"name": "Floor", "transform": {"scale": [40, 1, 40]},
//        "components": {"MeshRenderer": {"mesh": "primitive:plane", "material": "stone"},
//                       "Collider": {"shape": "Mesh"}}},
//       {"prefab": "Crate", "name": "Crate 1", "transform": {"position": [0, 3, 0]},
//        "components": {"RigidBody": {"mass": 8}}},
//       {"name": "Statue", "model": "models/statue.glb", "collider": "mesh"},
//       {"name": "Spinner", "behaviours": [{"type": "Spinner", "degreesPerSecond": 45}],
//        "children": [ ... ]}
//     ]
//   }
//
// Mesh references: "primitive:cube|sphere|plane|capsule|cylinder" (unit size,
// Unity-style) or "model.glb#M.P" (glTF mesh M, primitive P). Material
// references: a name from "materials" or "model.glb#materialN". Prefabs are
// inline or files ("prefab": "prefabs/crate.json" holding {"materials", "entity"});
// instance fields override the prefab's component fields key by key, and
// instance behaviours and children are appended. "model" expands a glTF scene
// into child entities (hierarchy, meshes, materials; "collider": "mesh" adds
// static mesh colliders).
//
// Loading is two-phase and transactional: Parse validates the whole document
// and resolves every asset it references; Instantiate then cannot fail, so a
// bad scene never leaves a half-built world. Errors name the JSON path.

#include "Engine/Assets/AssetManager.h"
#include "Engine/Core/Json.h"
#include "Engine/Framework/GameWorld.h"
#include "Engine/Graphics/Texture.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Astral::Framework {

struct BehaviourSpec {
    std::string type;
    bool enabled{true};
    Core::JsonValue properties; // object
};

struct EntitySpec {
    std::string name;
    TransformDesc transform;
    Core::JsonValue components = Core::JsonValue::MakeObject(); // type name -> field object
    std::vector<BehaviourSpec> behaviours;
    std::vector<EntitySpec> children;
};

struct SceneDocument {
    std::vector<EntitySpec> entities;
    std::map<std::string, EntitySpec> prefabs;
    bool hasEnvironment{};
    Environment environment;
    bool hasGravity{};
    Math::Vec3 gravity{};
    float fixedDelta{};  // 0 = keep the world's
    float timeScale{-1}; // < 0 = keep the world's
    // Resolved, shared resources keyed by reference string.
    std::map<std::string, std::shared_ptr<const Graphics::MeshData>> meshes;
    std::map<std::string, std::shared_ptr<const Physics::TriangleMesh>> collisionMeshes;
    std::map<std::string, std::shared_ptr<const Graphics::Material>> materials;
    std::map<std::string, std::shared_ptr<const Audio::AudioClip>> clips;
    std::map<std::string, std::shared_ptr<const Graphics::LodGroup>> lodGroups; // autoLod meshes
    std::map<std::string, Core::JsonValue> materialSources; // library entries as authored
    std::vector<Assets::AssetDependency> dependencies; // reached typed slots, including nested prefabs
};

// Unit-size primitive meshes: cube (1 m), sphere (d = 1 m), plane (1 x 1 m,
// facing +Y), capsule and cylinder (d = 1 m, 2 m tall), centred on the origin.
std::shared_ptr<const Graphics::MeshData> MakePrimitiveMesh(const std::string& kind);

class SceneSerializer {
public:
    // `assets` resolves file references (models, textures, sounds, prefab and
    // scene files); without it only primitives and inline content are allowed.
    explicit SceneSerializer(Assets::AssetManager* assets = nullptr,
        const BehaviourRegistry& behaviours = BehaviourRegistry::Global());

    // Optional collector is appended even on failure. strictReload rejects
    // degraded glTF images while initial/standalone imports retain warnings.
    bool Parse(const Core::JsonValue& json, SceneDocument& out, std::string& error,
        std::vector<Assets::AssetDependency>* attempted = nullptr, bool strictReload = false) const;
    // Reads a scene file through the asset manager (or directly without one).
    bool ParseFile(const std::string& path, SceneDocument& out, std::string& error,
        std::vector<Assets::AssetDependency>* attempted = nullptr, bool strictReload = false) const;

    // Adds the document's entities (under `parent`) and returns the new roots.
    // With applySettings, gravity/time/environment are applied to the world.
    std::vector<Entity> Instantiate(const SceneDocument& document, GameWorld& world, Entity parent = {},
        bool applySettings = true) const;
    // Spawns one prefab (Unity's Instantiate / UE SpawnActor); null for an unknown prefab.
    Entity InstantiatePrefab(const SceneDocument& document, const std::string& prefab, GameWorld& world,
        const Math::TRS& transform, Entity parent = {}) const;

    // Serialises every live root entity (with descendants), the materials they
    // use, the environment and settings. Parse(Save(world)) reproduces the world.
    Core::JsonValue Save(const GameWorld& world) const;
    Core::JsonValue SaveEntity(const GameWorld& world, Entity entity) const;

    // Root-zero expanded hierarchy depth; also bounds active prefab-resolution
    // indirection (the first definition is resolution depth zero).
    std::size_t maxDepth{64};
    // Independent limits for expanded output nodes and retained cached prefab
    // nodes. Every instance copy is charged in full in its own domain.
    std::size_t maxEntities{100000};

private:
    struct SpecExtent;
    struct ParseState;
    bool Admit(const SpecExtent& extent, std::size_t depth, bool cached, ParseState& state,
        const std::string& path) const;
    bool ParseEntity(const Core::JsonValue& json, const std::string& path, ParseState& state, EntitySpec& out,
        std::size_t depth, bool cached, SpecExtent& extent) const;
    bool ParsePrefab(const std::string& name, const std::string& path, ParseState& state) const;
    bool ParseComponents(const Core::JsonValue& json, const std::string& path, ParseState& state, EntitySpec& out) const;
    bool ResolveMesh(const std::string& ref, const std::string& path, ParseState& state) const;
    bool ResolveMaterial(const std::string& ref, const std::string& path, ParseState& state) const;
    bool ResolveClip(const std::string& ref, const std::string& path, ParseState& state) const;
    bool ExpandModel(const std::string& model, bool colliders, const std::string& path, ParseState& state,
        EntitySpec& out, std::size_t depth, bool cached, SpecExtent& extent) const;
    Entity Spawn(const EntitySpec& spec, const SceneDocument& document, GameWorld& world, Entity parent) const;
    void AddComponents(const EntitySpec& spec, const SceneDocument& document, GameWorld& world, Entity entity) const;

    Assets::AssetManager* assets_;
    const BehaviourRegistry& behaviours_;
};

} // namespace Astral::Framework
