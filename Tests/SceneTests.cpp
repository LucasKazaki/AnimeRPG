#include "Engine/Assets/StaticMesh.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/PlayerController.h"
#include "Engine/Scene/Transform.h"
#include "Tests/EngineTestSupport.h"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
bool NearlyEqual(float left, float right) {
    return std::fabs(left - right) < 0.001f;
}

void Check(bool condition) {
    if (!condition) std::abort();
}

bool Same(Astral::Math::Vec3 left, Astral::Math::Vec3 right) {
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

void CheckControllerNumericRegressions() {
    using namespace Astral::Scene;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float limit = std::numeric_limits<float>::max();
    PlayerController controller;
    controller.SetPosition({1.0f, 2.0f, 3.0f});
    const auto before = controller.TransformState().localPosition;
    for (float invalid : {nan, inf, -inf, 0.0f, -1.0f}) {
        controller.Update({}, invalid);
        ASTRAL_CHECK(Same(controller.TransformState().localPosition, before));
        controller.Update({true, false, false, true}, invalid);
        ASTRAL_CHECK(Same(controller.TransformState().localPosition, before));
    }
    for (auto invalid : {Astral::Math::Vec3{nan, 0, 0},
        Astral::Math::Vec3{0, inf, 0}, Astral::Math::Vec3{0, 0, -inf}}) {
        controller.SetPosition(invalid);
        ASTRAL_CHECK(Same(controller.TransformState().localPosition, before));
        ASTRAL_CHECK(controller.IsWithinBounds());
    }
    const auto rejectsConfiguration = [](float speed, MovementBounds bounds) {
        try { PlayerController invalid(speed, bounds); }
        catch (const std::invalid_argument&) { return true; }
        return false;
    };
    for (float invalid : {nan, inf, -inf, -1.0f})
        ASTRAL_CHECK(rejectsConfiguration(invalid, {}));
    for (MovementBounds invalid : {MovementBounds{2, -2, -1, 1}, MovementBounds{-1, 1, 2, -2},
        MovementBounds{nan, 1, -1, 1}, MovementBounds{-1, inf, -1, 1},
        MovementBounds{-1, 1, -inf, 1}, MovementBounds{-1, 1, -1, nan}})
        ASTRAL_CHECK(rejectsConfiguration(1.0f, invalid));

    PlayerController stationary(0.0f, {2, 2, 3, 3});
    stationary.Update({true, false, false, true}, limit);
    ASTRAL_CHECK(Same(stationary.TransformState().localPosition, {2, 3, 0}));
    PlayerController midpoint(1.0f, {limit / 2, limit, -limit, -limit / 2});
    const float expected = static_cast<float>((static_cast<double>(limit / 2) + limit) * 0.5);
    ASTRAL_CHECK(Same(midpoint.TransformState().localPosition, {expected, -expected, 0}));
    ASTRAL_CHECK(midpoint.IsWithinBounds());
    PlayerController fast(limit);
    fast.Update({false, false, false, true}, limit);
    ASTRAL_CHECK(Same(fast.TransformState().localPosition, {9, 0, 0}));
    fast.Update({true, true, true, true}, limit);
    ASTRAL_CHECK(Same(fast.TransformState().localPosition, {9, 0, 0}));
    ASTRAL_CHECK(fast.IsWithinBounds());
}

void CheckHierarchyNumericRegressions() {
    using namespace Astral::Scene;
    const float limit = std::numeric_limits<float>::max();
    const Astral::Math::Vec3 sentinel{12, 34, 56};
    Astral::Math::Vec3 output = sentinel;
    Transform root, child;
    child.parent = &root;
    root.localPosition = {limit, 0, 0};
    child.localPosition = {limit, 0, 0};
    ASTRAL_CHECK(!child.TryWorldPosition(output));
    ASTRAL_CHECK(Same(output, sentinel));
    const auto invalid = child.WorldPosition();
    ASTRAL_CHECK(std::isnan(invalid.x) && std::isnan(invalid.y) && std::isnan(invalid.z));

    // A final representable sum remains valid despite an intermediate float overflow.
    Transform leaf;
    leaf.localPosition = {-limit, 0, 0};
    leaf.parent = &child;
    ASTRAL_CHECK(leaf.TryWorldPosition(output));
    ASTRAL_CHECK(Same(output, {limit, 0, 0}));
    output = sentinel;
    root.localPosition = {0, std::numeric_limits<float>::quiet_NaN(), 0};
    ASTRAL_CHECK(!child.TryWorldPosition(output));
    ASTRAL_CHECK(Same(output, sentinel));
    root.localPosition = {0, 0, std::numeric_limits<float>::infinity()};
    ASTRAL_CHECK(!child.TryWorldPosition(output));
    ASTRAL_CHECK(Same(output, sentinel));

    root.localPosition = {};
    root.parent = &root;
    ASTRAL_CHECK(!root.TryWorldPosition(output));
    ASTRAL_CHECK(Same(output, sentinel));
    root.parent = &child;
    ASTRAL_CHECK(!leaf.TryWorldPosition(output));
    ASTRAL_CHECK(Same(output, sentinel));
    child.parent = &leaf;
    ASTRAL_CHECK(!root.TryWorldPosition(output));
    ASTRAL_CHECK(Same(output, sentinel));

    // Stable storage keeps all borrowed parents alive during this deep query.
    std::vector<Transform> chain(50000);
    for (std::size_t i = 0; i < chain.size(); ++i) {
        chain[i].localPosition = {1, -1, 2};
        if (i != 0) chain[i].parent = &chain[i - 1];
    }
    ASTRAL_CHECK(chain.back().TryWorldPosition(output));
    ASTRAL_CHECK(Same(output, {50000, -50000, 100000}));
}

void CheckCameraNumericRegressions() {
    using namespace Astral::Scene;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float limit = std::numeric_limits<float>::max();
    Astral::Math::Vec2 output{12, 34};
    const auto unchanged = [&] { return output.x == 12 && output.y == 34; };
    PerspectiveCamera perspective;
    for (float invalid : {nan, inf, -inf, 0.0f, -1.0f, 180.0f}) {
        perspective.verticalFieldOfViewDegrees = invalid;
        ASTRAL_CHECK(!perspective.WorldToScreen({}, 800, 600, output) && unchanged());
    }
    perspective.verticalFieldOfViewDegrees = 60;
    for (float invalid : {nan, inf, -inf, 0.0f, -1.0f}) {
        perspective.nearPlane = invalid;
        ASTRAL_CHECK(!perspective.WorldToScreen({}, 800, 600, output) && unchanged());
    }
    perspective.nearPlane = 0.5f;
    ASTRAL_CHECK(!perspective.WorldToScreen({}, 0, 600, output) && unchanged());
    ASTRAL_CHECK(!perspective.WorldToScreen({}, 800, -1, output) && unchanged());
    for (auto invalid : {Astral::Math::Vec3{nan, 0, 0}, Astral::Math::Vec3{0, inf, 0},
        Astral::Math::Vec3{0, 0, -inf}, Astral::Math::Vec3{1e20f, 0, 0}})
        ASTRAL_CHECK(!perspective.WorldToScreen(invalid, 800, 600, output) && unchanged());
    ASTRAL_CHECK(std::isnan(perspective.Depth({nan, 0, 0})));
    ASTRAL_CHECK(std::isnan(perspective.Depth({0, -limit, limit})));
    ASTRAL_CHECK(perspective.WorldToScreen({100, 0, 0}, 800, 600, output));
    ASTRAL_CHECK(output.x > 800 && std::isfinite(output.y));
    // Large but safe offscreen raster values remain accepted.
    ASTRAL_CHECK(perspective.WorldToScreen({1e6f, 0, 0}, 800, 600, output));

    Transform target;
    target.localPosition = {4, 7, 0};
    perspective.Follow(target);
    const auto before = perspective.Position();
    target.localPosition.x = nan;
    perspective.Follow(target);
    ASTRAL_CHECK(Same(before, perspective.Position()));
    target.localPosition = {};
    target.parent = &target;
    perspective.Follow(target);
    ASTRAL_CHECK(Same(before, perspective.Position()));

    OrthographicCamera orthographic;
    output = {12, 34};
    for (float invalid : {nan, inf, -inf, 0.0f, -1.0f}) {
        orthographic.worldWidth = invalid;
        ASTRAL_CHECK(!orthographic.TryWorldToScreen({}, 800, 600, output) && unchanged());
        const auto fallback = orthographic.WorldToScreen({}, 800, 600);
        ASTRAL_CHECK(fallback.x == 0 && fallback.y == 0);
    }
    orthographic.worldWidth = 20;
    orthographic.worldHeight = 0;
    ASTRAL_CHECK(!orthographic.TryWorldToScreen({}, 800, 600, output) && unchanged());
    orthographic.worldHeight = 12;
    ASTRAL_CHECK(!orthographic.TryWorldToScreen({nan, 0, 0}, 800, 600, output) && unchanged());
    ASTRAL_CHECK(!orthographic.TryWorldToScreen({}, 0, 600, output) && unchanged());
    ASTRAL_CHECK(!orthographic.TryWorldToScreen({}, 800, -1, output) && unchanged());
    orthographic.worldWidth = std::numeric_limits<float>::denorm_min();
    ASTRAL_CHECK(!orthographic.TryWorldToScreen({1, 0, 0}, 800, 600, output) && unchanged());
    orthographic.worldWidth = 20;
    target.parent = nullptr;
    target.localPosition = {1, 2, 3};
    orthographic.Follow(target);
    const auto screenBefore = orthographic.WorldToScreen({}, 800, 600);
    orthographic.Follow(target, {inf, 0, 0});
    target.parent = &target;
    orthographic.Follow(target);
    target.parent = nullptr;
    target.localPosition = {limit, 0, 0};
    orthographic.Follow(target, {limit, 0, 0});
    const auto screenAfter = orthographic.WorldToScreen({}, 800, 600);
    ASTRAL_CHECK(screenBefore.x == screenAfter.x && screenBefore.y == screenAfter.y);
}
}

int main() {
    CheckControllerNumericRegressions();
    CheckHierarchyNumericRegressions();
    CheckCameraNumericRegressions();
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

    Astral::Scene::PerspectiveCamera perspective;
    Astral::Math::Vec2 projected{};
    const Astral::Math::Vec3 cameraPosition = perspective.Position();
    constexpr float forwardY = -0.514495755f;
    constexpr float forwardZ = 0.857492926f;
    const auto atDepth = [&](float x, float up, float depth) {
        return Astral::Math::Vec3{x,
            cameraPosition.y + forwardY * depth + forwardZ * up,
            cameraPosition.z + forwardZ * depth - forwardY * up};
    };

    Check(perspective.WorldToScreen(atDepth(0.0f, 0.0f, 10.0f), 800, 600, projected));
    Check(NearlyEqual(projected.x, 400.0f));
    Check(NearlyEqual(projected.y, 300.0f));

    Astral::Math::Vec2 nearOffset{};
    Astral::Math::Vec2 farOffset{};
    Check(perspective.WorldToScreen(atDepth(1.0f, 0.0f, 5.0f), 800, 600, nearOffset));
    Check(perspective.WorldToScreen(atDepth(1.0f, 0.0f, 10.0f), 800, 600, farOffset));
    Check((nearOffset.x - 400.0f) > (farOffset.x - 400.0f));
    Check(NearlyEqual(nearOffset.x - 400.0f, 2.0f * (farOffset.x - 400.0f)));

    Astral::Math::Vec2 above{};
    Check(perspective.WorldToScreen(atDepth(0.0f, 1.0f, 10.0f), 800, 600, above));
    Check(above.y < projected.y);
    Check(!perspective.WorldToScreen(atDepth(0.0f, 0.0f, perspective.NearPlane()),
        800, 600, projected));
    Check(!perspective.WorldToScreen(atDepth(0.0f, 0.0f, -1.0f), 800, 600, projected));

    Astral::Scene::Transform perspectiveTarget;
    perspectiveTarget.localPosition = {4.0f, 7.0f, 0.0f};
    perspective.Follow(perspectiveTarget);
    Check(NearlyEqual(perspective.Position().x, 4.0f));
    Check(NearlyEqual(perspective.Position().z, -3.0f));

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
