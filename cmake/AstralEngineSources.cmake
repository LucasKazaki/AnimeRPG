# Single source of truth for the portable Astral engine runtime.
# Included by the root CMakeLists.txt (Windows/MSVC) and by
# Tests/EngineRuntime/CMakeLists.txt (portable Linux/sanitizer builds).
# Every file listed here must be platform-independent C++17.

get_filename_component(ASTRAL_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(ASTRAL_ENGINE_RUNTIME_SOURCES
    "${ASTRAL_ENGINE_ROOT}/Engine/AI/BehaviorTree.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/AI/Navigation.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Animation/Animator.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Animation/HumanoidRig.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Animation/Skeleton.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Animation/Skinning.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Animation/Timeline.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Animation/Tween.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Assets/AssetManager.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Assets/Gltf.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Assets/Jpeg.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Audio/AudioMixer.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Core/Console.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Core/GameTime.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Core/JobSystem.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Core/Json.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Core/Profiler.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Core/Reflection.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Framework/Behaviour.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Framework/Components.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Framework/GameHost.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Framework/GameWorld.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Framework/Scene.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Framework/Sequencer.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Framework/Timers.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Canvas.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Image.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Mesh.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/MeshSimplify.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/PostProcess.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/ProceduralTextures.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Rasterizer.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/RenderTarget.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/SceneRenderer.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Sprite2D.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Texture.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Input/InputSystem.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Physics/BroadPhase.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Physics/CharacterController.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Physics/Collision.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Physics/Destruction.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Physics/PhysicsWorld.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Physics/TriangleMesh.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/UI/ConsoleOverlay.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/UI/Widgets.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/VFX/Particles.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/World/Components.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/World/Registry.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/World/Serialization.cpp"
)

# Astral-mode presentation of the game (opt-in renderer, HUD, audio, VFX).
# The root build compiles these into AstralGame next to the gameplay sources.
set(ASTRAL_SHOWCASE_SOURCES
    "${ASTRAL_ENGINE_ROOT}/Game/Showcase/CharacterPresenter.cpp"
    "${ASTRAL_ENGINE_ROOT}/Game/Showcase/GameplayBridge.cpp"
    "${ASTRAL_ENGINE_ROOT}/Game/Showcase/MallScene.cpp"
    "${ASTRAL_ENGINE_ROOT}/Game/Showcase/MallShowcase.cpp"
    "${ASTRAL_ENGINE_ROOT}/Game/Showcase/ShowcaseSession.cpp"
)

# Authoritative gameplay domains the showcase presents. The root build already
# lists them for AstralGame; the portable build compiles them itself so the
# headless capture tool and showcase suite drive the real rules.
set(ASTRAL_GAMEPLAY_SOURCES
    "${ASTRAL_ENGINE_ROOT}/Engine/Scene/Camera.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Scene/CombatSandbox.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Scene/LandmarkEncounter.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Scene/LandmarkInteraction.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Scene/PlayerController.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Scene/ShadowbladeActions.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Scene/ThoughtCommands.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Scene/Transform.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Scene/WorldBlockout.cpp"
)

# Engine sample projects (behaviours for Content/Samples/*). Built into the
# AstralShowcase library for the player tools and suites, never into AstralGame.
set(ASTRAL_SAMPLE_SOURCES
    "${ASTRAL_ENGINE_ROOT}/Game/Samples/Playground/PlaygroundBehaviours.cpp"
)

# Each suite is Tests/<name>.cpp and links the runtime and showcase libraries.
set(ASTRAL_ENGINE_TEST_SUITES
    EngineMathTests
    EngineCoreTests
    EngineGraphicsTests
    Engine2DTests
    EngineWorldTests
    EnginePhysicsTests
    EngineAnimationTests
    EngineSystemsTests
    EngineShowcaseTests
    EngineServicesTests
    EngineAssetsTests
    EngineFrameworkTests
    EngineTimelineTests
    EngineUITests
    EngineSampleTests
)
