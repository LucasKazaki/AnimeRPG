#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Astral::Graphics::D3D11 {

struct ShaderCompileResult {
    std::vector<std::uint8_t> bytecode;
    std::string diagnostics;
    bool Succeeded() const { return !bytecode.empty(); }
};

// No implicit filesystem includes. The diagnostic includes source, entry,
// profile and HRESULT, including the compiler's actual error/warning blob.
ShaderCompileResult CompileShader(std::string_view source, const std::string& name,
    const std::string& entry, const std::string& profile, bool debug = false);

} // namespace Astral::Graphics::D3D11
