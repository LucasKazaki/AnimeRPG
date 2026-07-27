#pragma once

#include "Engine/Assets/StaticMesh.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/Transform.h"

#include <windows.h>

namespace Astral::Renderer {

class Renderer {
public:
    void Clear(HDC deviceContext, RECT viewport) const;
    void RenderDebugScene(HDC deviceContext, RECT viewport, const Scene::OrthographicCamera& camera,
        const Assets::StaticMesh& mesh, const Scene::Transform& transform) const;
};

} // namespace Astral::Renderer
