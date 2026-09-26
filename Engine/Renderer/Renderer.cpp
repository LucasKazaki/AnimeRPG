#include "Engine/Renderer/Renderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cwchar>

#include <gdiplus.h>

namespace {
using Astral::Math::Vec2;
using Astral::Math::Vec3;
using Astral::Scene::PerspectiveCamera;
using Gdiplus::Color;
using Gdiplus::Graphics;
using Gdiplus::Image;
using Gdiplus::Pen;
using Gdiplus::Rect;
using Gdiplus::RectF;
using Gdiplus::SolidBrush;
using Gdiplus::UnitPixel;

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

void DrawBackdrop(HDC deviceContext, RECT viewport, Image* backdrop) {
    if (!backdrop) return;

    Graphics graphics(deviceContext);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeBilinear);
    const int width = viewport.right - viewport.left;
    const int height = viewport.bottom - viewport.top;
    graphics.DrawImage(backdrop, Rect(0, 0, width, height), 0, 0,
        static_cast<int>(backdrop->GetWidth()), static_cast<int>(backdrop->GetHeight()),
        UnitPixel);

    // A restrained color wash preserves HUD contrast while letting the generated key art
    // carry the scene's silhouette and mood.
    SolidBrush wash(Color(38, 7, 12, 38));
    graphics.FillRectangle(&wash, Rect(0, 0, width, height));
}

bool ProjectGround(const PerspectiveCamera& camera, int width, int height,
    const Vec3& groundPosition, Vec2& screenPosition) {
    return camera.WorldToScreen(groundPosition, width, height, screenPosition);
}

void DrawProjectedRing(HDC deviceContext, const PerspectiveCamera& camera, int width, int height,
    const Vec3& groundPosition, float radius, Color color, float pulse = 1.0f) {
    Vec2 center{};
    Vec2 edge{};
    if (!ProjectGround(camera, width, height, groundPosition, center)
        || !ProjectGround(camera, width, height,
            {groundPosition.x + radius * pulse, groundPosition.y, groundPosition.z}, edge)) {
        return;
    }

    Graphics graphics(deviceContext);
    Pen ringPen(color, 2.0f);
    const float radiusPixels = std::max(8.0f, std::abs(edge.x - center.x));
    graphics.DrawEllipse(&ringPen, RectF(center.x - radiusPixels, center.y - radiusPixels * 0.32f,
        radiusPixels * 2.0f, radiusPixels * 0.64f));
}

void DrawBillboard(HDC deviceContext, const PerspectiveCamera& camera, int width, int height,
    const Vec3& groundPosition, float worldHeight, Image* image, float opacity = 1.0f) {
    if (!image || worldHeight <= 0.0f) return;

    Vec2 bottom{};
    Vec2 top{};
    if (!ProjectGround(camera, width, height, groundPosition, bottom)
        || !camera.WorldToScreen({groundPosition.x, groundPosition.y + worldHeight,
            groundPosition.z}, width, height, top)) {
        return;
    }

    const float pixelHeight = std::clamp(std::abs(bottom.y - top.y), 32.0f, 720.0f);
    const float pixelWidth = pixelHeight * static_cast<float>(image->GetWidth())
        / static_cast<float>(image->GetHeight());
    const float alpha = std::clamp(opacity, 0.0f, 1.0f);
    Graphics graphics(deviceContext);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeBilinear);
    if (alpha >= 0.999f) {
        graphics.DrawImage(image,
            RectF(bottom.x - pixelWidth * 0.5f, top.y, pixelWidth, pixelHeight));
        return;
    }

    Gdiplus::ImageAttributes attributes;
    const float matrixElements[5][5]{
        {1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, alpha, 0.0f},
        {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    };
    Gdiplus::ColorMatrix matrix{};
    std::copy(&matrixElements[0][0], &matrixElements[0][0] + 25,
        &matrix.m[0][0]);
    attributes.SetColorMatrix(&matrix);
    graphics.DrawImage(image,
        RectF(bottom.x - pixelWidth * 0.5f, top.y, pixelWidth, pixelHeight),
        0.0f, 0.0f, static_cast<float>(image->GetWidth()),
        static_cast<float>(image->GetHeight()), UnitPixel, &attributes);
}

void DrawHudFrame(HDC deviceContext, int left, int top, int right, int bottom,
    Color fill, Color outline) {
    Graphics graphics(deviceContext);
    SolidBrush brush(fill);
    Pen pen(outline, 1.0f);
    graphics.FillRectangle(&brush, Rect(left, top, right - left, bottom - top));
    graphics.DrawRectangle(&pen, Rect(left, top, right - left, bottom - top));
}

void DrawWorldMarker(HDC deviceContext, const PerspectiveCamera& camera, int width, int height,
    const Vec3& position, Color color, float pulse) {
    Vec2 screen{};
    if (!camera.WorldToScreen(position, width, height, screen)) return;

    Graphics graphics(deviceContext);
    Pen pen(color, 2.0f);
    const float size = 12.0f + pulse * 4.0f;
    graphics.DrawLine(&pen, screen.x - size, screen.y, screen.x + size, screen.y);
    graphics.DrawLine(&pen, screen.x, screen.y - size, screen.x, screen.y + size);
    graphics.DrawEllipse(&pen, RectF(screen.x - size * 0.65f, screen.y - size * 0.65f,
        size * 1.3f, size * 1.3f));
}
}

namespace Astral::Renderer {

void Renderer::Clear(HDC deviceContext, RECT viewport) const {
    const HBRUSH background = CreateSolidBrush(RGB(12, 18, 36));
    FillRect(deviceContext, &viewport, background);
    DeleteObject(background);
    DrawBackdrop(deviceContext, viewport, assets_.WorldBackdrop());
}

void Renderer::RenderWorld(HDC deviceContext, RECT viewport,
    const Scene::PerspectiveCamera& camera, const Scene::WorldBlockout& world,
    const Scene::Transform& playerTransform, const Scene::CombatSandbox& combatSandbox,
    const Scene::ShadowbladeActions& shadowbladeActions,
    const Scene::ThoughtCommands& thoughtCommands,
    const Scene::LandmarkInteraction& landmarkInteraction,
    const Scene::LandmarkEncounter& landmarkEncounter) const {
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
        const bool selected = landmarkInteraction.HasSelection()
            && landmarkInteraction.SelectedIndex() == index;
        const bool visited = landmarkInteraction.IsVisited(world.Landmarks()[index].kind);
        const COLORREF color = visited ? RGB(80, 235, 125)
            : (selected ? RGB(255, 90, 220) : landmarkColors[index]);
        const HPEN landmarkPen = CreatePen(PS_SOLID, selected ? 5 : (visited ? 3 : 2), color);
        SelectObject(deviceContext, landmarkPen);
        DrawLandmark(deviceContext, camera, width, height, world.Landmarks()[index]);
        SelectObject(deviceContext, gridPen);
        DeleteObject(landmarkPen);
    }

    const float pulse = 0.5f + 0.5f
        * std::sin(combatSandbox.ElapsedSeconds() * 3.5f);
    if (landmarkInteraction.HasSelection()) {
        const Scene::LandmarkProxy& selectedLandmark =
            world.Landmarks()[landmarkInteraction.SelectedIndex()];
        DrawWorldMarker(deviceContext, camera, width, height,
            {selectedLandmark.position.x, 1.25f, selectedLandmark.position.z},
            Color(215, 255, 95, 220), pulse);
    }

    const Math::Vec3 player = world.GroundPosition(playerTransform.WorldPosition());
    const HPEN playerPen = CreatePen(PS_SOLID, 3, RGB(168, 92, 255));
    SelectObject(deviceContext, playerPen);
    const std::array<Math::Vec3, 4> playerBase{{
        {player.x - 0.65f, 0.0f, player.z - 0.65f},
        {player.x + 0.65f, 0.0f, player.z - 0.65f},
        {player.x + 0.65f, 0.0f, player.z + 0.65f},
        {player.x - 0.65f, 0.0f, player.z + 0.65f}}};
    const Math::Vec3 playerApex{player.x, 2.2f, player.z};
    for (std::size_t index = 0; index < playerBase.size(); ++index) {
        DrawSegment(deviceContext, camera, width, height, playerBase[index],
            playerBase[(index + 1) % playerBase.size()]);
        DrawSegment(deviceContext, camera, width, height, playerBase[index], playerApex);
    }
    if (assets_.Shadowblade()) {
        DrawProjectedRing(deviceContext, camera, width, height, player, 1.0f,
            Color(160, 88, 255, 200), 0.95f + pulse * 0.1f);
        DrawBillboard(deviceContext, camera, width, height, player, 3.6f,
            assets_.Shadowblade());
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
    if (assets_.TrainingDummy()) {
        DrawProjectedRing(deviceContext, camera, width, height, dummyGround, 0.85f,
            dummy.IsDefeated() ? Color(130, 135, 155, 170) : Color(255, 118, 70, 220),
            0.95f + pulse * 0.1f);
        DrawBillboard(deviceContext, camera, width, height, dummyGround, 3.2f,
            assets_.TrainingDummy(), dummy.IsDefeated() ? 0.42f : 1.0f);
    }

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

    DrawHudFrame(deviceContext, 14, 12, 332, 164,
        Color(185, 7, 12, 24), Color(130, 112, 180, 120));
    if (assets_.AstralSigil()) {
        Graphics graphics(deviceContext);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeBilinear);
        graphics.DrawImage(assets_.AstralSigil(), Rect(286, 18, 34, 34));
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

    HBRUSH interactionBrush = nullptr;
    if (landmarkInteraction.HasSelection()) {
        const bool selectedVisited = landmarkInteraction.IsVisited(
            landmarkInteraction.SelectedKind());
        interactionBrush = CreateSolidBrush(selectedVisited
            ? RGB(80, 235, 125) : RGB(255, 90, 220));
        RECT interactionBanner{20, 108, 300, 126};
        FillRect(deviceContext, &interactionBanner, interactionBrush);
    }
    SetTextColor(deviceContext, landmarkInteraction.HasSelection()
        ? RGB(245, 245, 255) : RGB(150, 165, 190));
    const wchar_t* interactionLabel = landmarkInteraction.HasSelection()
        ? L"LANDMARK IN RANGE  E INTERACT" : L"LANDMARK DISCOVERY  EXPLORE TO INTERACT";
    TextOutW(deviceContext, 20, 111, interactionLabel,
        static_cast<int>(wcslen(interactionLabel)));

    const bool encounterActive = landmarkEncounter.State()
        == Scene::LandmarkEncounterState::Active;
    const bool encounterCompleted = landmarkEncounter.State()
        == Scene::LandmarkEncounterState::Completed;
    const HBRUSH encounterBrush = CreateSolidBrush(encounterCompleted
        ? RGB(80, 235, 125) : (encounterActive ? RGB(255, 155, 60) : RGB(65, 75, 100)));
    RECT encounterBanner{20, 132, 300, 150};
    FillRect(deviceContext, &encounterBanner, encounterBrush);
    SetTextColor(deviceContext, RGB(245, 245, 255));
    const wchar_t* encounterLabel = encounterCompleted
        ? L"TRAINING ENCOUNTER COMPLETED  REWARD GRANTED"
        : (encounterActive ? L"TRAINING ENCOUNTER ACTIVE  DEFEAT DUMMY"
            : L"TRAINING ENCOUNTER LOCKED  DISCOVER LINCOLN");
    TextOutW(deviceContext, 20, 135, encounterLabel,
        static_cast<int>(wcslen(encounterLabel)));

    const int rightPanelLeft = std::max(350, width - 356);
    DrawHudFrame(deviceContext, rightPanelLeft, 16, width - 18, 94,
        Color(165, 8, 13, 28), Color(105, 105, 170, 110));
    if (assets_.AstralSigil()) {
        Graphics graphics(deviceContext);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeBilinear);
        graphics.DrawImage(assets_.AstralSigil(), Rect(width - 62, 27, 34, 34));
    }
    SetTextColor(deviceContext, RGB(230, 235, 255));
    const wchar_t* worldTitle = L"ASTRAL WINDOW  //  NIGHTFALL PROTOCOL";
    TextOutW(deviceContext, rightPanelLeft + 14, 28, worldTitle,
        static_cast<int>(wcslen(worldTitle)));
    SetTextColor(deviceContext, RGB(155, 185, 225));
    const wchar_t* worldSubtitle = encounterCompleted
        ? L"RIFT STABILIZED  //  REWARD SECURED"
        : (encounterActive ? L"RIFT SURGE  //  TRAINING SIGNAL LOCKED"
            : L"RIFT ACTIVE  //  EXPLORE THE MEMORIAL AXIS");
    TextOutW(deviceContext, rightPanelLeft + 14, 58, worldSubtitle,
        static_cast<int>(wcslen(worldSubtitle)));

    SelectObject(deviceContext, previousPen);
    DeleteObject(gridPen);
    DeleteObject(playerPen);
    DeleteObject(dummyPen);
    DeleteObject(healthBackgroundBrush);
    DeleteObject(healthBrush);
    DeleteObject(shadowBackgroundBrush);
    DeleteObject(shadowResourceBrush);
    if (interactionBrush) DeleteObject(interactionBrush);
    DeleteObject(encounterBrush);
}

} // namespace Astral::Renderer
