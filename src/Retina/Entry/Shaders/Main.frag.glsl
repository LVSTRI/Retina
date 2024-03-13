#include <Retina/Retina.glsl>

layout (location = 0) in SVertexData {
  vec3 Color;
} i_VertexData;

layout (location = 0) out vec4 o_Pixel;

void main() {
  o_Pixel = vec4(i_VertexData.Color, 1.0);
}