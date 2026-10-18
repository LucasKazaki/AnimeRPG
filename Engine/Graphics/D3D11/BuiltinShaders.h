#pragma once

namespace Astral::Graphics::D3D11::Detail {

inline constexpr char kOpaqueUnlitShader[] = R"hlsl(
cbuffer DrawConstants : register(b0) {
    row_major float4x4 clipFromLocal;
    float4 baseTint;
    float4 emissive;
    float4 uvAndTextureSize;
    uint4 flags; // vertex color, texture, object ID, unused
};
Texture2D<float4> baseTexture : register(t0);
SamplerState textureSampler : register(s0);
struct Input {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};
struct Interpolated {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};
Interpolated VSMain(Input input) {
    Interpolated output;
    output.position = mul(clipFromLocal, float4(input.position, 1));
    output.uv = input.uv * uvAndTextureSize.xy;
    output.color = input.color;
    return output;
}
struct Pixel {
    float4 hdr : SV_Target0;
    uint objectId : SV_Target1;
};
Pixel PSMain(Interpolated input) {
    float3 albedo = baseTint.rgb;
    if (flags.x != 0) albedo *= input.color.rgb;
    if (flags.y != 0) {
        float2 dx = ddx(input.uv) * uvAndTextureSize.zw;
        float2 dy = ddy(input.uv) * uvAndTextureSize.zw;
        float rho = max(dot(dx, dx), dot(dy, dy));
        float lod = rho > 1 ? 0.5 * log2(rho) : 0;
        // CPU Texture2D::Sample elides interpolation for a fraction below .01.
        if (frac(lod) < 0.01) lod = floor(lod);
        albedo *= baseTexture.SampleLevel(textureSampler, input.uv, lod).rgb;
    }
    Pixel output;
    output.hdr = float4(albedo + emissive.rgb, 1);
    output.objectId = flags.z;
    return output;
}
)hlsl";

} // namespace Astral::Graphics::D3D11::Detail
