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
};

RetinaDeclareQualifiedBuffer(restrict readonly, SViewInfoBuffer) {
  SViewInfo[] Data;
};

RetinaDeclareBufferPointer(SViewInfoBuffer, g_ViewInfoBuffer, u_ViewInfoBufferId);

RetinaDeclareSampledImage(utexture2D, STexture2DU);
RetinaDeclareSampledImage(texture2D, STexture2D);
#define g_VisbufferMain RetinaGetSampledImage(STexture2DU, u_VisbufferMainId)
#define g_VisbufferMainDepth RetinaGetSampledImage(STexture2D, u_VisbufferMainDepthId)

RetinaDeclareStorageImage(restrict readonly uimage2DArray, SROImage2DArrayU);
#define g_VirtualPageTable RetinaGetStorageImage(SROImage2DArrayU, u_VirtualPageTableId)

RetinaDeclareStorageImage(restrict readonly uimage2D, SROImage2DU);
#define g_VirtualShadowMap RetinaGetStorageImage(SROImage2DU, u_VirtualShadowMapId)

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
  const vec3 worldPosition = RetinaUnprojectWorldPosition(i_Uv, depth, inverse(mainView.ProjView));
  const vec3 color = RetinaHsvToRgb(fract(float(meshletInstanceIndex) * M_GOLDEN_CONJUGATE), 0.75, 0.75);
  o_Pixel = vec4(color, 1.0);
}
