#include "Engine/Assets/StaticMesh.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/PlayerController.h"
#include "Engine/Scene/Transform.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace {
bool NearlyEqual(float left, float right) {
    return std::fabs(left - right) < 0.001f;
}
}

int main() {
    Astral::Scene::PlayerController asymmetricBoundsController(4.0f,
        Astral::Scene::MovementBounds{-4.0f, 6.0f, -2.0f, 8.0f});
    assert(NearlyEqual(asymmetricBoundsController.TransformState().localPosition.x, 1.0f));
    assert(NearlyEqual(asymmetricBoundsController.TransformState().localPosition.y, 3.0f));

    Astral::Scene::PlayerController controller(4.0f,
        Astral::Scene::MovementBounds{-2.0f, 2.0f, -2.0f, 2.0f});
    controller.Update({true, false, false, false}, 0.5f);
    assert(NearlyEqual(controller.TransformState().localPosition.y, 2.0f));

    controller.SetPosition({0.0f, 0.0f, 0.0f});
    controller.Update({true, false, true, false}, 0.5f);
    assert(NearlyEqual(controller.TransformState().localPosition.x, -1.4142f));
    assert(NearlyEqual(controller.TransformState().localPosition.y, 1.4142f));

    const auto positionBeforeInvalidDelta = controller.TransformState().localPosition;
    controller.Update({false, false, false, true}, 0.0f);
    controller.Update({false, true, false, false}, -1.0f);
    assert(NearlyEqual(controller.TransformState().localPosition.x, positionBeforeInvalidDelta.x));
    assert(NearlyEqual(controller.TransformState().localPosition.y, positionBeforeInvalidDelta.y));

    controller.SetPosition({1.9f, 0.9f, 0.0f});
    controller.Update({true, false, false, true}, 1.0f);
    assert(NearlyEqual(controller.TransformState().localPosition.x, 2.0f));
    assert(NearlyEqual(controller.TransformState().localPosition.y, 2.0f));
    assert(controller.IsWithinBounds());

    Astral::Scene::Transform parent;
    parent.localPosition = {3.0f, 4.0f, 0.0f};

    Astral::Scene::Transform child;
    child.localPosition = {1.0f, -2.0f, 0.0f};
    child.parent = &parent;
    const Astral::Math::Vec3 worldPosition = child.WorldPosition();
    assert(NearlyEqual(worldPosition.x, 4.0f));
    assert(NearlyEqual(worldPosition.y, 2.0f));

    Astral::Scene::OrthographicCamera camera;
    const Astral::Math::Vec2 center = camera.WorldToScreen({0.0f, 0.0f, 0.0f}, 100, 80);
    assert(NearlyEqual(center.x, 50.0f));
    assert(NearlyEqual(center.y, 40.0f));

    camera.Follow(controller.TransformState(), {1.0f, -2.0f, 0.0f});
    const Astral::Math::Vec2 followed = camera.WorldToScreen(
        {3.0f, 0.0f, 0.0f}, 100, 80);
    assert(NearlyEqual(followed.x, 50.0f));
    assert(NearlyEqual(followed.y, 40.0f));

    const std::filesystem::path meshPath =
        std::filesystem::temp_directory_path() / "astral_m2_scene_test.mesh";
    {
        std::ofstream output(meshPath);
        output << "ASTRAL_MESH 1\n"
               << "vertex -1 -1 0\n"
               << "vertex 1 -1 0\n"
               << "vertex 0 1 0\n"
               << "edge 0 1\n"
               << "edge 1 2\n"
               << "edge 2 0\n";
    }

    Astral::Assets::StaticMesh mesh;
    assert(mesh.LoadFromFile(meshPath.string()));
    assert(mesh.Vertices().size() == 3);
    assert(mesh.Edges().size() == 3);
    std::filesystem::remove(meshPath);

    Astral::Assets::StaticMesh malformedMesh;
    assert(!malformedMesh.LoadFromFile("missing/astral.mesh"));
    return 0;
}