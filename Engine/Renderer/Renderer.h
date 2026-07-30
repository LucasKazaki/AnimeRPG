#pragma once

#include "Engine/Scene/Camera.h"
#include "Engine/Scene/CombatSandbox.h"
#include "Engine/Scene/ShadowbladeActions.h"
#include "Engine/Scene/ThoughtCommands.h"
#include "Engine/Scene/Transform.h"
#include "Engine/Scene/WorldBlockout.h"

#include <windows.h>

namespace Astral::Renderer {

class Renderer {
public:
    void Clear(HDC deviceContext, RECT viewport) const;
    void RenderWorld(HDC deviceContext, RECT viewport, const Scene::PerspectiveCamera& camera,
        const Scene::WorldBlockout& world, const Scene::Transform& playerTransform,
        const Scene::CombatSandbox& combatSandbox,
        const Scene::ShadowbladeActions& shadowbladeActions,
        const Scene::ThoughtCommands& thoughtCommands) const;
};

} // namespace Astral::Renderer
