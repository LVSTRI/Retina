#version 460
#include <RayTracing/Common.glsl>

layout(location = 0) rayPayloadInEXT vec3 p_color;

hitAttributeEXT vec2 g_barycentrics;

void main() {
    const vec3 barycentrics = vec3(1.0 - g_barycentrics.x - g_barycentrics.y, g_barycentrics.x, g_barycentrics.y);
    p_color = barycentrics;
}
