#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>
#include <Meshlet.glsl>

#define M_GOLDEN_CONJUGATE 0.618033988749895

layout (location = 0) in vec2 i_Uv;

layout (location = 0) out vec4 o_Pixel;

RetinaDeclarePushConstant() {
  uint u_VisbufferMainId;
  uint u_MeshletBufferId;
  uint u_MeshletInstanceBufferId;
  uint u_TransformBufferId;
  uint u_PositionBufferId;
  uint u_IndexBufferId;
  uint u_PrimitiveBufferId;
  uint u_ViewBufferId;
};

RetinaDeclareQualifiedBuffer(restrict readonly, SMeshletBuffer) {
  SMeshlet[] Data;
};
RetinaDeclareQualifiedBuffer(restrict readonly, SMeshletInstanceBuffer) {
  SMeshletInstance[] Data;
};
RetinaDeclareQualifiedBuffer(restrict readonly, STransformBuffer) {
  mat4[] Data;
};
RetinaDeclareQualifiedBuffer(restrict readonly, SPositionBuffer) {
  vec3[] Data;
};
RetinaDeclareQualifiedBuffer(restrict readonly, SIndexBuffer) {
  uint[] Data;
};
RetinaDeclareQualifiedBuffer(restrict readonly, SPrimitiveBuffer) {
  uint8_t[] Data;
};
RetinaDeclareQualifiedBuffer(restrict readonly, SViewInfoBuffer) {
  SViewInfo[] Data;
};

RetinaDeclareBufferPointer(SMeshletBuffer, g_MeshletBuffer, u_MeshletBufferId);
RetinaDeclareBufferPointer(SMeshletInstanceBuffer, g_MeshletInstanceBuffer, u_MeshletInstanceBufferId);
RetinaDeclareBufferPointer(STransformBuffer, g_TransformBuffer, u_TransformBufferId);
RetinaDeclareBufferPointer(SPositionBuffer, g_PositionBuffer, u_PositionBufferId);
RetinaDeclareBufferPointer(SIndexBuffer, g_IndexBuffer, u_IndexBufferId);
RetinaDeclareBufferPointer(SPrimitiveBuffer, g_PrimitiveBuffer, u_PrimitiveBufferId);
RetinaDeclareBufferPointer(SViewInfoBuffer, g_ViewInfoBuffer, u_ViewBufferId);

#define g_VisbufferMain RetinaGetSampledImage(Texture2DU, u_VisbufferMainId)

struct SPartialDerivatives {
  vec3 lambda;
  vec3 ddx;
  vec3 ddy;
};

precise SPartialDerivatives CalculatePartialDerivatives(in vec4 clip0, in vec4 clip1, in vec4 clip2, in vec2 p) {
  const vec2 screenSize = vec2(textureSize(g_VisbufferMain, 0));
  const vec3 invW = 1.0 / vec3(clip0.w, clip1.w, clip2.w);
  const vec2 ndc0 = clip0.xy * invW.x;
  const vec2 ndc1 = clip1.xy * invW.y;
  const vec2 ndc2 = clip2.xy * invW.z;

  const float invDet = 1.0 / (determinant(mat2(ndc2 - ndc1, ndc0 - ndc1)));

  vec3 ddx = invDet * invW * vec3(ndc1.y - ndc2.y, ndc2.y - ndc0.y, ndc0.y - ndc1.y);
  vec3 ddy = invDet * invW * vec3(ndc2.x - ndc1.x, ndc0.x - ndc2.x, ndc1.x - ndc0.x);
  float ddxSum = dot(ddx, vec3(1.0));
  float ddySum = dot(ddy, vec3(1.0));

  const vec2 deltaP = p - ndc0;
  const float interpInvW = invW.x + deltaP.x * ddxSum + deltaP.y * ddySum;
  const float interpW = 1.0 / interpInvW;

  const vec3 lambda = vec3(
    interpW * (invW.x + deltaP.x * ddx.x + deltaP.y * ddy.x),
    interpW * (0.0 + deltaP.x * ddx.y + deltaP.y * ddy.y),
    interpW * (0.0 + deltaP.x * ddx.z + deltaP.y * ddy.z)
  );

  ddx *= 2.0 / screenSize.x;
  ddy *= 2.0 / screenSize.y;
  ddxSum *= 2.0 / screenSize.x;
  ddySum *= 2.0 / screenSize.y;

  const float ddxInterpW = 1.0 / (interpInvW + ddxSum);
  const float ddyInterpW = 1.0 / (interpInvW + ddySum);

  ddx = ddxInterpW * (lambda * interpInvW + ddx) - lambda;
  ddy = ddyInterpW * (lambda * interpInvW + ddy) - lambda;

  return SPartialDerivatives(lambda, ddx, ddy);
}

void main() {
  const uint payload = texelFetch(g_VisbufferMain, ivec2(gl_FragCoord.xy), 0).r;
  if (payload == -1u) {
    o_Pixel = vec4(0.0, 0.0, 0.0, 1.0);
    return;
  }
  const uint meshletInstanceIndex = payload >> MESHLET_VISBUFFER_PRIMITIVE_ID_BITS;
  const uint meshletPrimitiveId = payload & MESHLET_VISBUFFER_PRIMITIVE_ID_MASK;
  const SViewInfo mainView = g_ViewInfoBuffer.Data[0];
  const SMeshletInstance meshletInstance = g_MeshletInstanceBuffer.Data[meshletInstanceIndex];
  const SMeshlet meshlet = g_MeshletBuffer.Data[meshletInstance.MeshletIndex];
  const mat4 transform = g_TransformBuffer.Data[meshletInstance.TransformIndex];
  const mat4 pvm = mainView.ProjView * transform;
  const uvec3 indices = uvec3(
    meshlet.VertexOffset + g_IndexBuffer.Data[meshlet.IndexOffset + uint(g_PrimitiveBuffer.Data[meshlet.PrimitiveOffset + meshletPrimitiveId * 3 + 0])],
    meshlet.VertexOffset + g_IndexBuffer.Data[meshlet.IndexOffset + uint(g_PrimitiveBuffer.Data[meshlet.PrimitiveOffset + meshletPrimitiveId * 3 + 1])],
    meshlet.VertexOffset + g_IndexBuffer.Data[meshlet.IndexOffset + uint(g_PrimitiveBuffer.Data[meshlet.PrimitiveOffset + meshletPrimitiveId * 3 + 2])]
  );
  const vec4 clipVertex0 = pvm * vec4(g_PositionBuffer.Data[indices.x], 1.0);
  const vec4 clipVertex1 = pvm * vec4(g_PositionBuffer.Data[indices.y], 1.0);
  const vec4 clipVertex2 = pvm * vec4(g_PositionBuffer.Data[indices.z], 1.0);
  const SPartialDerivatives derivatives = CalculatePartialDerivatives(clipVertex0, clipVertex1, clipVertex2, i_Uv * 2.0 - 1.0);

  const vec3 color = RetinaHsvToRgb(fract(float(meshletInstanceIndex) * M_GOLDEN_CONJUGATE), 0.75, 0.75);
  o_Pixel = vec4(derivatives.lambda, 1.0);
}
