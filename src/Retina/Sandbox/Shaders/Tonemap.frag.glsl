#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>
#include <Meshlet.glsl>

#define WHITE_POINT 0.95

layout (location = 0) in vec2 i_Uv;

layout (location = 0) out vec4 o_Pixel;

RetinaDeclarePushConstant() {
  uint u_VisbufferResolveImageId;
};

#define g_VisbufferResolveImage RetinaGetSampledImage(Texture2D, u_VisbufferResolveImageId)

float GetLuminance(in vec3 color) {
  return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 ChangeLuminance(in vec3 color, in float outLum) {
  const float inLum = GetLuminance(color);
  return color * (outLum / inLum);
}

vec3 ApplyExtendedReinhardTonemap(in vec3 color) {
  const float oldLum = GetLuminance(color);
  const float numerator = oldLum * (1.0 + (oldLum / (WHITE_POINT * WHITE_POINT)));
  const float denominator = 1.0 + oldLum;
  const float newLum = numerator / denominator;
  return ChangeLuminance(color, newLum);
}

void main() {
  const vec4 color = texelFetch(g_VisbufferResolveImage, ivec2(gl_FragCoord.xy), 0);
  o_Pixel = vec4(ApplyExtendedReinhardTonemap(color.rgb), 1.0);
}
