#include <Macros.hlsl>

struct SVertexOutput {
    [[Location(0)]] float4 Position : SV_Position;
    [[Location(1)]] float4 Color : Color;
};

struct SCamera {
    float4x4 Projection;
    float4x4 View;
    float4x4 ProjView;
    float4 Position;
};
[[Binding(0, 0)]] StructuredBuffer<SCamera> b_cameraBuffer[];

struct SPushConstant {
    uint cameraId;
};
[[PushConstant()]] ConstantBuffer<SPushConstant> u_pushConstants;

float3 ToSRGB(in float3 color) {
    float3 outColor;
    for (uint i = 0; i < 3; ++i) {
        if (color[i] <= 0.0031308) {
            outColor[i] = 12.92 * color[i];
        } else {
            outColor[i] = 1.055 * pow(color[i], 1.0 / 2.4) - 0.055;
        }
    }
    return outColor;
}

SVertexOutput VSMain(in uint vertexId : SV_VertexID) {
    const float3 trianglePositions[] = {
        float3( 0.5,  0.5, 0.0),
        float3(-0.5,  0.5, 0.0),
        float3( 0.0, -0.5, 0.0)
    };
    const float3 triangleColors[] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };
    const SCamera camera = b_cameraBuffer[NonUniformResourceIndex(u_pushConstants.cameraId)].Load(0);
    SVertexOutput output;
    output.Position = mul(camera.ProjView, float4(trianglePositions[vertexId], 1.0));
    output.Color = float4(triangleColors[vertexId], 1.0);
    return output;
}

struct SPixelOutput {
    [[Location(0)]] float4 Color : SV_Target;
};

SPixelOutput PSMain(in SVertexOutput input) {
    SPixelOutput output;
    output.Color = float4(ToSRGB(input.Color), 1.0);
    return output;
}
