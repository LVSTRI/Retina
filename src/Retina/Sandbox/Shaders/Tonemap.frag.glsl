#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>
#include <Meshlet.glsl>

#define WHITE_POINT 1.25

layout (location = 0) in vec2 i_Uv;

layout (location = 0) out vec4 o_Pixel;

RetinaDeclarePushConstant() {
  uint u_VisbufferResolveImageId;
};

#define g_VisbufferResolveImage RetinaGetSampledImage(Texture2D, u_VisbufferResolveImageId)

vec3 ExtendedReinhardTonemap(in vec3 color) {
  const float oldLum = RetinaGetLuminance(color);
  const float numerator = oldLum * (1.0 + (oldLum / (WHITE_POINT * WHITE_POINT)));
  const float denominator = 1.0 + oldLum;
  const float newLum = numerator / denominator;
  return RetinaChangeLuminance(color, newLum);
}

void main() {
  const vec4 color = texelFetch(g_VisbufferResolveImage, ivec2(gl_FragCoord.xy), 0);
  o_Pixel = vec4(ExtendedReinhardTonemap(color.rgb), 1.0);
}
