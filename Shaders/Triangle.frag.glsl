#version 460

#include <Macros.glsl>

layout (location = 0) in vec4 i_color;

layout (location = 0) out vec4 o_pixel;

void main() {
    o_pixel = i_color;
}