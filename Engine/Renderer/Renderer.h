#pragma once

#include "Engine/Scene/Camera.h"
#include "Engine/Scene/CombatSandbox.h"
#include "Engine/Scene/Transform.h"
#include "Engine/Scene/WorldBlockout.h"

#include <windows.h>

namespace Astral::Renderer {

class Renderer {
public:
    void Clear(HDC deviceContext, RECT viewport) const;
    void RenderWorld(HDC deviceContext, RECT viewport, const Scene::PerspectiveCamera& camera,
        const Scene::WorldBlockout& world, const Scene::Transform& playerTransform,
        const Scene::CombatSandbox& combatSandbox) const;
};

} // namespace Astral::Renderer
