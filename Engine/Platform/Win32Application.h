#pragma once

#include "Engine/Assets/StaticMesh.h"
#include "Engine/Scene/Camera.h"
#include "Engine/Scene/PlayerController.h"

#include <windows.h>

namespace Astral::Platform {

class Win32Application {
public:
    bool Create(HINSTANCE instance, int showCommand);
    int Run();

private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    HWND window_{};
    Assets::StaticMesh debugMesh_;
    Scene::OrthographicCamera camera_;
    Scene::PlayerController playerController_;
};

} // namespace Astral::Platform
