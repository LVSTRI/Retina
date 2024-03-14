#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>
#include <Meshlet.glsl>

#define M_GOLDEN_CONJUGATE 0.618033988749895

layout (location = 0) in vec2 i_Uv;

layout (location = 0) out vec4 o_Pixel;

RetinaDeclarePushConstant() {
  uint u_VisbufferMainId;
};

RetinaDeclareSampledImage(utexture2D, STexture2Du);

#define g_VisbufferMain RetinaGetSampledImage(STexture2Du, u_VisbufferMainId)

void main() {
  const uint payload = texelFetch(g_VisbufferMain, ivec2(gl_FragCoord.xy), 0).r;
  if (payload == -1u) {
    o_Pixel = vec4(0.0, 0.0, 0.0, 1.0);
    return;
  }
  const uint meshletInstanceIndex = payload >> MESHLET_VISBUFFER_PRIMITIVE_ID_BITS;
  const uint primitiveId = payload & MESHLET_VISBUFFER_PRIMITIVE_ID_MASK;
  o_Pixel = vec4(HsvToRgb(fract(float(meshletInstanceIndex) * M_GOLDEN_CONJUGATE), 0.75, 1.0), 1.0);
}
