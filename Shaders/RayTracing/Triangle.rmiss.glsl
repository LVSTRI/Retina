#version 460
#include <RayTracing/Common.glsl>

layout(location = 0) rayPayloadInEXT vec3 p_color;

void main() {
    p_color = vec3(0.0);
}