#pragma once

#include <windows.h>

namespace Astral::Platform {

inline constexpr UINT kTestKeyDownMessage = WM_APP + 0x321;
inline constexpr UINT kTestKeyUpMessage = WM_APP + 0x322;

} // namespace Astral::Platform
