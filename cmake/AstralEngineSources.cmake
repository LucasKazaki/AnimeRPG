# Single source of truth for the portable Astral engine runtime.
# Included by the root CMakeLists.txt (Windows/MSVC) and by
# Tests/EngineRuntime/CMakeLists.txt (portable Linux/sanitizer builds).
# Every file listed here must be platform-independent C++17.

get_filename_component(ASTRAL_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(ASTRAL_ENGINE_RUNTIME_SOURCES
    "${ASTRAL_ENGINE_ROOT}/Engine/Core/GameTime.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Core/JobSystem.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Core/Profiler.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Canvas.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Image.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Mesh.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/PostProcess.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/ProceduralTextures.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Rasterizer.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/RenderTarget.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/SceneRenderer.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Sprite2D.cpp"
    "${ASTRAL_ENGINE_ROOT}/Engine/Graphics/Texture.cpp"
)

set(ASTRAL_SHOWCASE_SOURCES
)

# Each suite is Tests/<name>.cpp and links the runtime and showcase libraries.
set(ASTRAL_ENGINE_TEST_SUITES
    EngineMathTests
    EngineCoreTests
    EngineGraphicsTests
    Engine2DTests
)
