#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>
#include <Meshlet.glsl>

layout (location = 0) in vec2 i_Uv;

layout (location = 0) out vec4 o_Pixel;

RetinaDeclarePushConstant() {
  uint u_VisbufferResolveImageId;
  float u_WhitePoint;
  uint u_IsPassthrough;
};

#define g_VisbufferResolveImage RetinaGetSampledImage(Texture2D, u_VisbufferResolveImageId)

float GetLuminance(in vec3 color) {
  return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 ChangeLuminance(in vec3 color, in float outLum) {
  const float inLum = GetLuminance(color);
  if (inLum == 0) {
    return color;
  }
  return color * (outLum / inLum);
}

vec3 ApplyExtendedReinhard(in vec3 color) {
  if (bool(u_IsPassthrough)) {
    return color;
  }
  const float oldLum = GetLuminance(color);
  const float numerator = oldLum * (1.0 + (oldLum / (u_WhitePoint * u_WhitePoint)));
  const float denominator = 1.0 + oldLum;
  const float newLum = numerator / denominator;
  return ChangeLuminance(color, newLum);
}

void main() {
  const vec4 color = texelFetch(g_VisbufferResolveImage, ivec2(gl_FragCoord.xy), 0);
  o_Pixel = vec4(ApplyExtendedReinhard(color.rgb), 1.0);
}
