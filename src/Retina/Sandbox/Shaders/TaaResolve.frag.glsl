#include <Retina/Retina.glsl>

layout (location = 0) in vec2 i_Uv;

layout (location = 0) out vec4 o_Pixel;

RetinaDeclarePushConstant() {
  uint u_ColorImageId;
  uint u_HistoryImageId;
  uint u_VelocityImageId;
  uint u_LinearSampler;
  uint u_NearestSampler;
  uint u_ShouldReset;
  float u_ModulationFactor;
};

#define g_LinearSampler RetinaGetSampler(u_LinearSampler)
#define g_NearestSampler RetinaGetSampler(u_NearestSampler)

#define g_ColorImage RetinaGetSampledImage(Texture2D, u_ColorImageId)
#define g_HistoryImage RetinaGetSampledImage(Texture2D, u_HistoryImageId)
#define g_VelocityImage RetinaGetSampledImage(Texture2D, u_VelocityImageId)

void main() {
  const vec2 velocity = textureLod(sampler2D(g_VelocityImage, g_NearestSampler), i_Uv, 0).xy;
  const vec2 previousUv = i_Uv - velocity;
  const vec3 currentColor = textureLod(sampler2D(g_ColorImage, g_NearestSampler), i_Uv, 0).rgb;
  if (bool(u_ShouldReset)) {
    o_Pixel = vec4(currentColor, 1.0);
    return;
  }

  const vec3 nearColor0 = textureLodOffset(sampler2D(g_ColorImage, g_NearestSampler), i_Uv, 0, ivec2(1, 0)).rgb;
  const vec3 nearColor1 = textureLodOffset(sampler2D(g_ColorImage, g_NearestSampler), i_Uv, 0, ivec2(-1, 0)).rgb;
  const vec3 nearColor2 = textureLodOffset(sampler2D(g_ColorImage, g_NearestSampler), i_Uv, 0, ivec2(0, 1)).rgb;
  const vec3 nearColor3 = textureLodOffset(sampler2D(g_ColorImage, g_NearestSampler), i_Uv, 0, ivec2(0, -1)).rgb;

  const vec3 minColor = min(currentColor, min(nearColor0, min(nearColor1, min(nearColor2, nearColor3))));
  const vec3 maxColor = max(currentColor, max(nearColor0, max(nearColor1, max(nearColor2, nearColor3))));

  const vec3 previousColor = textureLod(sampler2D(g_HistoryImage, g_LinearSampler), previousUv, 0).rgb;
  const vec3 clampedColor = clamp(previousColor, minColor, maxColor);

  const vec3 outputColor = mix(currentColor, clampedColor, u_ModulationFactor);
  o_Pixel = vec4(outputColor, 1.0);
}
