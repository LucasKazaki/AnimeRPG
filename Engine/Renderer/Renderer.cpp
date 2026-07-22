#include "Engine/Renderer/Renderer.h"

namespace Astral::Renderer {

void Renderer::Clear(HDC deviceContext, RECT viewport) const {
    const HBRUSH background = CreateSolidBrush(RGB(12, 18, 36));
    FillRect(deviceContext, &viewport, background);
    DeleteObject(background);
}

} // namespace Astral::Renderer
