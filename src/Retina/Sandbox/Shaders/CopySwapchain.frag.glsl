#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>

layout (location = 0) in vec2 i_Uv;

layout (location = 0) out vec4 o_Pixel;

RetinaDeclarePushConstant() {
  uint u_TonemapImageId;
  uint u_NearestSamplerId;
};

#define g_TonemapImage RetinaGetSampledImage(Texture2D, u_TonemapImageId)
#define g_NearestSampler RetinaGetSampler(u_NearestSamplerId)

void main() {
  const vec3 color = texture(sampler2D(g_TonemapImage, g_NearestSampler), i_Uv).rgb;
  o_Pixel = vec4(RetinaAsNonLinearColor(color), 1.0);
}