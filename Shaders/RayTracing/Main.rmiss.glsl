#version 460
#include <RayTracing/Common.glsl>

struct SRayPayload {
    bool Miss;
    float RayT;
    vec3 Normal;
    vec3 Color;
};

layout (location = 0) rayPayloadInEXT SRayPayload p_rayPayload;

void main() {
    p_rayPayload.Miss = true;
    p_rayPayload.RayT = 0.0;
    p_rayPayload.Normal = vec3(0.0);
    p_rayPayload.Color = vec3(0.0);
}