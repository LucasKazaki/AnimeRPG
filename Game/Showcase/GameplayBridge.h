#pragma once

// The only two places Astral mode touches gameplay state, shared by the Win32
// loop and the headless ShowcaseSession so both follow identical rules. The
// gameplay domains decide everything first (movement speed, bounds, dash
// cost/cooldown/distance); these helpers then sweep the result against the
// physical National Mall so walls and landmarks stay solid.

#include "Engine/Scene/PlayerController.h"
#include "Game/Showcase/MallShowcase.h"

namespace Astral::Showcase {

// Call after PlayerController::Update. `before` is the position ahead of it.
void ResolveWalk(Scene::PlayerController& player, Math::Vec3 before, MallShowcase& showcase);

// Call after the gameplay placed the player at an activated dash destination
// (PlayerController::SetPosition, bounds clamped). `from` is the position the
// dash started at. Records the dash for presentation; `showcase` may be null
// (raw gameplay positions, as the GDI renderer uses).
void ResolveDash(Scene::PlayerController& player, Math::Vec3 from, const Scene::ShadowActionReport& report,
    MallShowcase* showcase, FrameEvents& events);

} // namespace Astral::Showcase
