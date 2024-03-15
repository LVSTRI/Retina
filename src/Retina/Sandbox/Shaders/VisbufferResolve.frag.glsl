#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>
#include <Meshlet.glsl>

#define M_GOLDEN_CONJUGATE 0.618033988749895

layout (location = 0) in vec2 i_Uv;

layout (location = 0) out vec4 o_Pixel;

RetinaDeclarePushConstant() {
  uint u_VisbufferMainId;
  uint u_VisbufferMainDepthId;
  uint u_ViewInfoBufferId;
  uint u_ShadowCascadeBufferId;
};

RetinaDeclareQualifiedBuffer(restrict readonly, SViewInfoBuffer) {
  SViewInfo[] Data;
};
RetinaDeclareQualifiedBuffer(restrict readonly, SShadowCascadeBuffer) {
  SShadowCascadeInfo[] Data;
};
RetinaDeclareBufferPointer(SViewInfoBuffer, g_ViewInfoBuffer, u_ViewInfoBufferId);
RetinaDeclareBufferPointer(SShadowCascadeBuffer, g_ShadowCascadeBuffer, u_ShadowCascadeBufferId);

RetinaDeclareSampledImage(utexture2D, STexture2Du);
RetinaDeclareSampledImage(texture2D, STexture2D);

#define g_VisbufferMain RetinaGetSampledImage(STexture2Du, u_VisbufferMainId)
#define g_VisbufferMainDepth RetinaGetSampledImage(STexture2D, u_VisbufferMainDepthId)

vec3 GetPositionFromDepth(in vec2 uv, in float depth, in mat4 invProjView) {
  const vec4 ndc = vec4(uv * 2.0 - 1.0, depth, 1.0);
  const vec4 worldPosition = invProjView * ndc;
  return worldPosition.xyz / worldPosition.w;
}

uint GetCascadeIndex(in float distance) {
  for (uint i = 0; i < SHADOW_CASCADE_COUNT; ++i) {
    if (distance < g_ShadowCascadeBuffer.Data[i].Offset.w) {
      return i;
    }
  }
  return SHADOW_CASCADE_COUNT - 1u;
}

void main() {
  const uint payload = texelFetch(g_VisbufferMain, ivec2(gl_FragCoord.xy), 0).r;
  if (payload == -1u) {
    o_Pixel = vec4(0.0, 0.0, 0.0, 1.0);
    return;
  }
  const uint meshletInstanceIndex = payload >> MESHLET_VISBUFFER_PRIMITIVE_ID_BITS;
  const uint primitiveId = payload & MESHLET_VISBUFFER_PRIMITIVE_ID_MASK;
  const float depth = texelFetch(g_VisbufferMainDepth, ivec2(gl_FragCoord.xy), 0).r;
  const SViewInfo mainView = g_ViewInfoBuffer.Data[0];
  const vec3 worldPosition = GetPositionFromDepth(i_Uv, depth, inverse(mainView.ProjView));
  const uint cascadeIndex = GetCascadeIndex((mainView.ProjView * vec4(worldPosition, 1.0)).w);
  o_Pixel = vec4(HsvToRgb(fract(float(cascadeIndex) * M_GOLDEN_CONJUGATE), 0.75, 0.75), 1.0);
}
