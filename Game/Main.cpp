#include "Engine/Platform/Win32Application.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    Astral::Platform::Win32Application application;
    if (!application.Create(instance, showCommand)) {
        return 1;
    }
    return application.Run();
}
