#include "Engine/Renderer/Renderer.h"

#include <cmath>

namespace Astral::Renderer {

void Renderer::Clear(HDC deviceContext, RECT viewport) const {
    const HBRUSH background = CreateSolidBrush(RGB(12, 18, 36));
    FillRect(deviceContext, &viewport, background);
    DeleteObject(background);
}

void Renderer::RenderDebugScene(HDC deviceContext, RECT viewport,
    const Scene::OrthographicCamera& camera, const Assets::StaticMesh& mesh,
    const Scene::Transform& transform, const Scene::CombatSandbox& combatSandbox) const {
    const int width = viewport.right - viewport.left;
    const int height = viewport.bottom - viewport.top;
    const auto toPoint = [&](const Math::Vec3& position) {
        const Math::Vec2 screen = camera.WorldToScreen(position, width, height);
        return POINT{static_cast<LONG>(std::lround(screen.x)), static_cast<LONG>(std::lround(screen.y))};
    };

    const HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(35, 52, 78));
    const HPEN meshPen = CreatePen(PS_SOLID, 3, RGB(168, 92, 255));
    const HGDIOBJ previousPen = SelectObject(deviceContext, gridPen);
    for (int grid = -10; grid <= 10; ++grid) {
        const POINT verticalStart = toPoint({static_cast<float>(grid), -6.0f, 0.0f});
        const POINT verticalEnd = toPoint({static_cast<float>(grid), 6.0f, 0.0f});
        MoveToEx(deviceContext, verticalStart.x, verticalStart.y, nullptr);
        LineTo(deviceContext, verticalEnd.x, verticalEnd.y);

        const POINT horizontalStart = toPoint({-10.0f, static_cast<float>(grid), 0.0f});
        const POINT horizontalEnd = toPoint({10.0f, static_cast<float>(grid), 0.0f});
        MoveToEx(deviceContext, horizontalStart.x, horizontalStart.y, nullptr);
        LineTo(deviceContext, horizontalEnd.x, horizontalEnd.y);
    }

    SelectObject(deviceContext, meshPen);
    const Math::Vec3 worldPosition = transform.WorldPosition();
    for (const Assets::MeshEdge& edge : mesh.Edges()) {
        const Math::Vec3 start = mesh.Vertices()[edge.start];
        const Math::Vec3 end = mesh.Vertices()[edge.end];
        const POINT screenStart = toPoint({start.x + worldPosition.x, start.y + worldPosition.y, 0.0f});
        const POINT screenEnd = toPoint({end.x + worldPosition.x, end.y + worldPosition.y, 0.0f});
        MoveToEx(deviceContext, screenStart.x, screenStart.y, nullptr);
        LineTo(deviceContext, screenEnd.x, screenEnd.y);
    }

    const Scene::TrainingDummy& dummy = combatSandbox.Dummy();
    const POINT dummyCenter = toPoint(dummy.position);
    const COLORREF dummyColor = dummy.IsDefeated() ? RGB(90, 90, 100) : RGB(255, 105, 80);
    const HPEN dummyPen = CreatePen(PS_SOLID, 3, dummyColor);
    const HBRUSH dummyBrush = CreateSolidBrush(dummy.IsDefeated() ? RGB(45, 45, 55) : RGB(110, 40, 45));
    SelectObject(deviceContext, dummyPen);
    const HGDIOBJ previousBrush = SelectObject(deviceContext, dummyBrush);
    Ellipse(deviceContext, dummyCenter.x - 18, dummyCenter.y - 30,
        dummyCenter.x + 18, dummyCenter.y + 30);

    const int healthWidth = dummy.maximumHealth > 0 ? (60 * dummy.health / dummy.maximumHealth) : 0;
    RECT healthBackground{dummyCenter.x - 30, dummyCenter.y - 42, dummyCenter.x + 30,
        dummyCenter.y - 36};
    RECT health{healthBackground.left, healthBackground.top,
        healthBackground.left + healthWidth, healthBackground.bottom};
    const HBRUSH healthBackgroundBrush = CreateSolidBrush(RGB(55, 25, 30));
    const HBRUSH healthBrush = CreateSolidBrush(RGB(90, 230, 120));
    FillRect(deviceContext, &healthBackground, healthBackgroundBrush);
    FillRect(deviceContext, &health, healthBrush);

    SelectObject(deviceContext, previousBrush);
    SelectObject(deviceContext, previousPen);
    DeleteObject(gridPen);
    DeleteObject(meshPen);
    DeleteObject(dummyPen);
    DeleteObject(dummyBrush);
    DeleteObject(healthBackgroundBrush);
    DeleteObject(healthBrush);
}

} // namespace Astral::Renderer
