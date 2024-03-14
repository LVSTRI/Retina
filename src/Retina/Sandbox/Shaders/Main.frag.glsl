#include <Retina/Retina.glsl>

#define M_GOLDEN_CONJUGATE 0.618033988749895

layout (location = 0) in SVertexData {
  flat uint MeshletInstanceIndex;
} i_VertexData;

layout (location = 0) out vec4 o_Pixel;

vec3 HsvToRgb(in vec3 hsv) {
  const vec3 rgb = clamp(abs(mod(hsv.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
  return hsv.z * mix(vec3(1.0), rgb, hsv.y);
}

void main() {
  const float hueIndex = fract(float(i_VertexData.MeshletInstanceIndex) * M_GOLDEN_CONJUGATE);
  o_Pixel = vec4(HsvToRgb(vec3(hueIndex, 0.9, 1.0)), 1.0);
}
