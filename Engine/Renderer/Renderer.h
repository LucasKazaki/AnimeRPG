#pragma once

#include <windows.h>

namespace Astral::Renderer {

class Renderer {
public:
    void Clear(HDC deviceContext, RECT viewport) const;
};

} // namespace Astral::Renderer
