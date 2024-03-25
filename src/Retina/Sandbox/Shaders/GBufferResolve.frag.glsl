#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>
#include <Meshlet.glsl>

layout (location = 0) in vec2 i_Uv;

layout (location = 0) precise out vec4 o_Pixel;

RetinaDeclarePushConstant() {
  uint u_GBufferAlbedoImageId;
  uint u_GBufferNormalImageId;
  uint u_GBufferShaderMaterialImageId;
  uint u_PointSamplerId;
  uint u_PointSamplerIntId;
};

#define g_GBufferAlbedoImage RetinaGetSampledImage(Texture2D, u_GBufferAlbedoImageId)
#define g_GBufferNormalImage RetinaGetSampledImage(Texture2D, u_GBufferNormalImageId)
#define g_GBufferShaderMaterialImage RetinaGetSampledImage(Texture2DU, u_GBufferShaderMaterialImageId)
#define g_PointSampler RetinaGetSampler(u_PointSamplerId)
#define g_PointSamplerInt RetinaGetSampler(u_PointSamplerIntId)

vec3 DecodeNormalOctahedral(in vec2 encoded) {
  if (encoded == vec2(0.0)) {
    return vec3(0.0);
  }
  encoded = encoded * 2.0 - 1.0;
  vec3 normal = vec3(encoded.xy, 1.0 - abs(encoded.x) - abs(encoded.y));
  const float t = RetinaSaturate(-normal.z);
  normal.x += normal.x >= 0.0 ? -t : t;
  normal.y += normal.y >= 0.0 ? -t : t;
  return normalize(normal);
}

void main() {
  const vec3 albedo = texture(sampler2D(g_GBufferAlbedoImage, g_PointSampler), i_Uv).rgb;
  const vec3 normal = DecodeNormalOctahedral(texture(sampler2D(g_GBufferNormalImage, g_PointSampler), i_Uv).rg);
  const uint shaderMaterialId = texture(usampler2D(g_GBufferShaderMaterialImage, g_PointSamplerInt), i_Uv).r;
  if (shaderMaterialId == -1) {
    discard;
  }
  const uint shaderId = shaderMaterialId >> 16;
  const uint materialId = shaderMaterialId & 0xffffu;
  o_Pixel = vec4(albedo, 1.0);
}
