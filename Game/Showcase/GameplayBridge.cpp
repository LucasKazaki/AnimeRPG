#include "Game/Showcase/GameplayBridge.h"

namespace Astral::Showcase {

void ResolveWalk(Scene::PlayerController& player, Math::Vec3 before, MallShowcase& showcase) {
    player.SetPosition(showcase.ResolveMovement(before, player.TransformState().WorldPosition(), false));
}

void ResolveDash(Scene::PlayerController& player, Math::Vec3 from, const Scene::ShadowActionReport& report,
    MallShowcase* showcase, FrameEvents& events) {
    if (showcase) {
        player.SetPosition(showcase->ResolveMovement(from, player.TransformState().WorldPosition(), true));
    }
    events.dash = true;
    events.dashReport = report;
    events.dashFrom = from;
    events.dashTo = player.TransformState().WorldPosition();
}

} // namespace Astral::Showcase
