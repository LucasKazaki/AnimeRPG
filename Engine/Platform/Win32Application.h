#pragma once

#include "Engine/Scene/Camera.h"
#include "Engine/Scene/CombatSandbox.h"
#include "Engine/Scene/PlayerController.h"
#include "Engine/Scene/WorldBlockout.h"

#include <windows.h>

namespace Astral::Platform {

class Win32Application {
public:
    bool Create(HINSTANCE instance, int showCommand);
    int Run();

private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    HWND window_{};
    Scene::PerspectiveCamera camera_;
    Scene::WorldBlockout world_;
    Scene::PlayerController playerController_{6.0f, {-18.0f, 18.0f, -4.0f, 72.0f}};
    Scene::CombatSandbox combatSandbox_;
    bool lightAttackPressed_{};
    bool heavyAttackPressed_{};
};

} // namespace Astral::Platform
