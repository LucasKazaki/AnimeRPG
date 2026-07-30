#include "Engine/Renderer/Renderer.h"

#include <array>
#include <cmath>
#include <cwchar>

namespace {
using Astral::Math::Vec2;
using Astral::Math::Vec3;
using Astral::Scene::PerspectiveCamera;

bool DrawSegment(HDC deviceContext, const PerspectiveCamera& camera, int width, int height,
    Vec3 start, Vec3 end) {
    float startDepth = camera.Depth(start);
    float endDepth = camera.Depth(end);
    const float clippedDepth = camera.NearPlane() + 0.01f;
    if (startDepth <= camera.NearPlane() && endDepth <= camera.NearPlane()) return false;
    if (startDepth <= camera.NearPlane()) {
        const float amount = (clippedDepth - startDepth) / (endDepth - startDepth);
        start = {start.x + (end.x - start.x) * amount,
            start.y + (end.y - start.y) * amount,
            start.z + (end.z - start.z) * amount};
    } else if (endDepth <= camera.NearPlane()) {
        const float amount = (clippedDepth - endDepth) / (startDepth - endDepth);
        end = {end.x + (start.x - end.x) * amount,
            end.y + (start.y - end.y) * amount,
            end.z + (start.z - end.z) * amount};
    }

    Vec2 screenStart{};
    Vec2 screenEnd{};
    if (!camera.WorldToScreen(start, width, height, screenStart)
        || !camera.WorldToScreen(end, width, height, screenEnd)) return false;
    MoveToEx(deviceContext, static_cast<int>(std::lround(screenStart.x)),
        static_cast<int>(std::lround(screenStart.y)), nullptr);
    LineTo(deviceContext, static_cast<int>(std::lround(screenEnd.x)),
        static_cast<int>(std::lround(screenEnd.y)));
    return true;
}

void DrawBox(HDC deviceContext, const PerspectiveCamera& camera, int width, int height,
    const Vec3& groundCenter, const Vec3& dimensions, float heightScale = 1.0f) {
    const float halfX = dimensions.x * 0.5f;
    const float halfZ = dimensions.z * 0.5f;
    const float top = groundCenter.y + dimensions.y * heightScale;
    const std::array<Vec3, 8> vertices{{
        {groundCenter.x - halfX, groundCenter.y, groundCenter.z - halfZ},
        {groundCenter.x + halfX, groundCenter.y, groundCenter.z - halfZ},
        {groundCenter.x + halfX, groundCenter.y, groundCenter.z + halfZ},
        {groundCenter.x - halfX, groundCenter.y, groundCenter.z + halfZ},
        {groundCenter.x - halfX, top, groundCenter.z - halfZ},
        {groundCenter.x + halfX, top, groundCenter.z - halfZ},
        {groundCenter.x + halfX, top, groundCenter.z + halfZ},
        {groundCenter.x - halfX, top, groundCenter.z + halfZ},
    }};
    constexpr std::array<std::array<int, 2>, 12> edges{{
        {{0, 1}}, {{1, 2}}, {{2, 3}}, {{3, 0}}, {{4, 5}}, {{5, 6}},
        {{6, 7}}, {{7, 4}}, {{0, 4}}, {{1, 5}}, {{2, 6}}, {{3, 7}},
    }};
    for (const auto& edge : edges) {
        DrawSegment(deviceContext, camera, width, height, vertices[edge[0]], vertices[edge[1]]);
    }
}

void DrawLandmark(HDC deviceContext, const PerspectiveCamera& camera, int width, int height,
    const Astral::Scene::LandmarkProxy& landmark) {
    DrawBox(deviceContext, camera, width, height, landmark.position, landmark.dimensions,
        landmark.kind == Astral::Scene::LandmarkKind::WashingtonMonument ? 0.82f : 1.0f);

    if (landmark.kind == Astral::Scene::LandmarkKind::LincolnMemorial) {
        const float halfX = landmark.dimensions.x * 0.5f;
        const float halfZ = landmark.dimensions.z * 0.5f;
        const float roofY = landmark.position.y + landmark.dimensions.y + 1.5f;
        const Vec3 roofFront{landmark.position.x, roofY, landmark.position.z - halfZ};
        const Vec3 roofBack{landmark.position.x, roofY, landmark.position.z + halfZ};
        DrawSegment(deviceContext, camera, width, height, roofFront, roofBack);
        DrawSegment(deviceContext, camera, width, height, roofFront,
            {landmark.position.x - halfX, landmark.position.y + landmark.dimensions.y,
                landmark.position.z - halfZ});
        DrawSegment(deviceContext, camera, width, height, roofFront,
            {landmark.position.x + halfX, landmark.position.y + landmark.dimensions.y,
                landmark.position.z - halfZ});
        DrawSegment(deviceContext, camera, width, height, roofBack,
            {landmark.position.x - halfX, landmark.position.y + landmark.dimensions.y,
                landmark.position.z + halfZ});
        DrawSegment(deviceContext, camera, width, height, roofBack,
            {landmark.position.x + halfX, landmark.position.y + landmark.dimensions.y,
                landmark.position.z + halfZ});
    } else if (landmark.kind == Astral::Scene::LandmarkKind::WashingtonMonument) {
        const float halfX = landmark.dimensions.x * 0.5f;
        const float halfZ = landmark.dimensions.z * 0.5f;
        const float shoulderY = landmark.position.y + landmark.dimensions.y * 0.82f;
        const Vec3 apex{landmark.position.x, landmark.position.y + landmark.dimensions.y,
            landmark.position.z};
        for (float x : {-halfX, halfX}) {
            for (float z : {-halfZ, halfZ}) {
                DrawSegment(deviceContext, camera, width, height,
                    {landmark.position.x + x, shoulderY, landmark.position.z + z}, apex);
            }
        }
    }
}
}

namespace Astral::Renderer {

void Renderer::Clear(HDC deviceContext, RECT viewport) const {
    const HBRUSH background = CreateSolidBrush(RGB(12, 18, 36));
    FillRect(deviceContext, &viewport, background);
    DeleteObject(background);
}

void Renderer::RenderWorld(HDC deviceContext, RECT viewport,
    const Scene::PerspectiveCamera& camera, const Scene::WorldBlockout& world,
    const Scene::Transform& playerTransform, const Scene::CombatSandbox& combatSandbox,
    const Scene::ShadowbladeActions& shadowbladeActions,
    const Scene::ThoughtCommands& thoughtCommands) const {
    const int width = viewport.right - viewport.left;
    const int height = viewport.bottom - viewport.top;
    const HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(35, 52, 78));
    const HGDIOBJ previousPen = SelectObject(deviceContext, gridPen);
    const Scene::GroundGrid& grid = world.Grid();
    for (float x = grid.minimumX; x <= grid.maximumX; x += grid.spacing) {
        DrawSegment(deviceContext, camera, width, height, {x, 0.0f, grid.minimumZ},
            {x, 0.0f, grid.maximumZ});
    }
    for (float z = grid.minimumZ; z <= grid.maximumZ; z += grid.spacing) {
        DrawSegment(deviceContext, camera, width, height, {grid.minimumX, 0.0f, z},
            {grid.maximumX, 0.0f, z});
    }

    constexpr std::array<COLORREF, 3> landmarkColors{{
        RGB(235, 205, 120), RGB(70, 190, 235), RGB(225, 225, 235)}};
    for (std::size_t index = 0; index < world.Landmarks().size(); ++index) {
        const HPEN landmarkPen = CreatePen(PS_SOLID, 2, landmarkColors[index]);
        SelectObject(deviceContext, landmarkPen);
        DrawLandmark(deviceContext, camera, width, height, world.Landmarks()[index]);
        SelectObject(deviceContext, gridPen);
        DeleteObject(landmarkPen);
    }

    const Math::Vec3 player = world.GroundPosition(playerTransform.WorldPosition());
    const HPEN playerPen = CreatePen(PS_SOLID, 3, RGB(168, 92, 255));
    SelectObject(deviceContext, playerPen);
    const std::array<Math::Vec3, 4> playerBase{{
        {player.x - 0.65f, 0.0f, player.z - 0.65f}, {player.x + 0.65f, 0.0f, player.z - 0.65f},
        {player.x + 0.65f, 0.0f, player.z + 0.65f}, {player.x - 0.65f, 0.0f, player.z + 0.65f}}};
    const Math::Vec3 playerApex{player.x, 2.2f, player.z};
    for (std::size_t index = 0; index < playerBase.size(); ++index) {
        DrawSegment(deviceContext, camera, width, height, playerBase[index],
            playerBase[(index + 1) % playerBase.size()]);
        DrawSegment(deviceContext, camera, width, height, playerBase[index], playerApex);
    }

    if (shadowbladeActions.IsGuarding()) {
        Math::Vec2 playerScreen{};
        if (camera.WorldToScreen({player.x, 1.0f, player.z}, width, height, playerScreen)) {
            const HPEN guardPen = CreatePen(PS_SOLID, 4, RGB(255, 220, 70));
            const HGDIOBJ priorGuardPen = SelectObject(deviceContext, guardPen);
            const HGDIOBJ priorBrush = SelectObject(deviceContext, GetStockObject(HOLLOW_BRUSH));
            const int centerX = static_cast<int>(std::lround(playerScreen.x));
            const int centerY = static_cast<int>(std::lround(playerScreen.y));
            Ellipse(deviceContext, centerX - 28, centerY - 38, centerX + 28, centerY + 38);
            SelectObject(deviceContext, priorBrush);
            SelectObject(deviceContext, priorGuardPen);
            DeleteObject(guardPen);
        }
    }

    const Scene::TrainingDummy& dummy = combatSandbox.Dummy();
    const Math::Vec3 dummyGround = world.GroundPosition(dummy.position);
    const COLORREF dummyColor = dummy.IsDefeated() ? RGB(90, 90, 100) : RGB(255, 105, 80);
    const HPEN dummyPen = CreatePen(PS_SOLID, 3, dummyColor);
    SelectObject(deviceContext, dummyPen);
    DrawBox(deviceContext, camera, width, height, dummyGround, {1.2f, 2.5f, 1.2f});

    const int healthWidth = dummy.maximumHealth > 0 ? (60 * dummy.health / dummy.maximumHealth) : 0;
    Math::Vec2 dummyTop{};
    const bool dummyVisible = camera.WorldToScreen(
        {dummyGround.x, 3.0f, dummyGround.z}, width, height, dummyTop);
    const HBRUSH healthBackgroundBrush = CreateSolidBrush(RGB(55, 25, 30));
    const HBRUSH healthBrush = CreateSolidBrush(RGB(90, 230, 120));
    if (dummyVisible) {
        const LONG centerX = static_cast<LONG>(std::lround(dummyTop.x));
        const LONG top = static_cast<LONG>(std::lround(dummyTop.y)) - 8;
        RECT healthBackground{centerX - 30, top, centerX + 30, top + 6};
        RECT health{healthBackground.left, healthBackground.top,
            healthBackground.left + healthWidth, healthBackground.bottom};
        FillRect(deviceContext, &healthBackground, healthBackgroundBrush);
        FillRect(deviceContext, &health, healthBrush);
    }

    const HBRUSH shadowBackgroundBrush = CreateSolidBrush(RGB(25, 30, 52));
    const HBRUSH shadowResourceBrush = CreateSolidBrush(thoughtCommands.IsFocusActive()
        ? RGB(120, 90, 255) : RGB(70, 220, 235));
    const int shadowWidth = static_cast<int>(180.0f * shadowbladeActions.Resource()
        / Scene::ShadowbladeActions::MaximumResource);
    RECT shadowBackground{20, 20, 200, 34};
    RECT shadowResource{20, 20, 20 + shadowWidth, 34};
    FillRect(deviceContext, &shadowBackground, shadowBackgroundBrush);
    FillRect(deviceContext, &shadowResource, shadowResourceBrush);
    SetBkMode(deviceContext, TRANSPARENT);
    SetTextColor(deviceContext, shadowbladeActions.IsGuarding()
        ? RGB(255, 220, 70) : RGB(210, 230, 255));
    const wchar_t* shadowLabel = shadowbladeActions.IsGuarding()
        ? L"SHADOWBLADE  GUARD ACTIVE" : L"SHADOWBLADE  Q DASH  L FATAL  SHIFT GUARD";
    TextOutW(deviceContext, 20, 39, shadowLabel, static_cast<int>(wcslen(shadowLabel)));

    if (thoughtCommands.IsFocusActive()) {
        const HBRUSH focusBrush = CreateSolidBrush(RGB(120, 90, 255));
        RECT focusBanner{20, 65, 300, 82};
        FillRect(deviceContext, &focusBanner, focusBrush);
        DeleteObject(focusBrush);
    }
    SetTextColor(deviceContext, thoughtCommands.IsFocusActive()
        ? RGB(210, 200, 255) : RGB(165, 180, 205));
    const wchar_t* commandLabel = thoughtCommands.IsFocusActive()
        ? L"THOUGHT FOCUS ACTIVE  LOCAL TIME x0.35"
        : L"THOUGHT COMMANDS  1 DASH 2 FATAL 3/4 GUARD 5 FOCUS";
    TextOutW(deviceContext, 20, 86, commandLabel, static_cast<int>(wcslen(commandLabel)));

    SelectObject(deviceContext, previousPen);
    DeleteObject(gridPen);
    DeleteObject(playerPen);
    DeleteObject(dummyPen);
    DeleteObject(healthBackgroundBrush);
    DeleteObject(healthBrush);
    DeleteObject(shadowBackgroundBrush);
    DeleteObject(shadowResourceBrush);
}

} // namespace Astral::Renderer
