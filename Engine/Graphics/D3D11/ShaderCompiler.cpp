#include "Engine/Graphics/D3D11/ShaderCompiler.h"

#include <d3dcompiler.h>
#include <wrl/client.h>

#include <iomanip>
#include <sstream>

namespace Astral::Graphics::D3D11 {

ShaderCompileResult CompileShader(std::string_view source, const std::string& name,
    const std::string& entry, const std::string& profile, bool debug) {
    ShaderCompileResult result;
    Microsoft::WRL::ComPtr<ID3DBlob> code, messages;
    const UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS
        | (debug ? D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION : D3DCOMPILE_OPTIMIZATION_LEVEL3);
    const HRESULT hr = D3DCompile(source.data(), source.size(), name.c_str(), nullptr, nullptr,
        entry.c_str(), profile.c_str(), flags, 0, &code, &messages);
    std::ostringstream status;
    status << name << ':' << entry << " [" << profile << "] HRESULT=0x" << std::hex
        << static_cast<unsigned long>(hr);
    if (messages && messages->GetBufferSize()) {
        status << '\n';
        const char* data = static_cast<const char*>(messages->GetBufferPointer());
        std::size_t size = messages->GetBufferSize();
        if (size && data[size - 1] == '\0') --size;
        status.write(data, static_cast<std::streamsize>(size));
    }
    result.diagnostics = status.str();
    if (SUCCEEDED(hr) && code) {
        const auto* first = static_cast<const std::uint8_t*>(code->GetBufferPointer());
        result.bytecode.assign(first, first + code->GetBufferSize());
    }
    return result;
}

} // namespace Astral::Graphics::D3D11
