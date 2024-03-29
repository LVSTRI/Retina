#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>
#include <Meshlet.glsl>

#define M_GOLDEN_CONJUGATE 0.618033988749895

layout (location = 0) in vec2 i_Uv;

layout (location = 0) precise out vec4 o_Albedo;
layout (location = 1) precise out vec2 o_Normal;
layout (location = 2) precise out uint o_ShaderMaterialId;

RetinaDeclarePushConstant() {
  uint u_VisbufferMainId;
  uint u_MeshletBufferId;
  uint u_MeshletInstanceBufferId;
  uint u_TransformBufferId;
  uint u_VertexBufferId;
  uint u_PositionBufferId;
  uint u_IndexBufferId;
  uint u_PrimitiveBufferId;
  uint u_MaterialBufferId;
  uint u_LinearSamplerId;
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
RetinaDeclareQualifiedBuffer(restrict readonly, SVertexBuffer) {
  SMeshletVertex[] Data;
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
RetinaDeclareQualifiedBuffer(restrict readonly, SMaterialBuffer) {
  SMaterial[] Data;
};
RetinaDeclareQualifiedBuffer(restrict readonly, SViewInfoBuffer) {
  SViewInfo[] Data;
};

RetinaDeclareBufferPointer(SMeshletBuffer, g_MeshletBuffer, u_MeshletBufferId);
RetinaDeclareBufferPointer(SMeshletInstanceBuffer, g_MeshletInstanceBuffer, u_MeshletInstanceBufferId);
RetinaDeclareBufferPointer(STransformBuffer, g_TransformBuffer, u_TransformBufferId);
RetinaDeclareBufferPointer(SVertexBuffer, g_VertexBuffer, u_VertexBufferId);
RetinaDeclareBufferPointer(SPositionBuffer, g_PositionBuffer, u_PositionBufferId);
RetinaDeclareBufferPointer(SIndexBuffer, g_IndexBuffer, u_IndexBufferId);
RetinaDeclareBufferPointer(SPrimitiveBuffer, g_PrimitiveBuffer, u_PrimitiveBufferId);
RetinaDeclareBufferPointer(SMaterialBuffer, g_MaterialBuffer, u_MaterialBufferId);
RetinaDeclareBufferPointer(SViewInfoBuffer, g_ViewInfoBuffer, u_ViewBufferId);

#define g_VisbufferMain RetinaGetSampledImage(Texture2DU, u_VisbufferMainId)

#define g_LinearSampler RetinaGetSampler(u_LinearSamplerId)

struct SPartialDerivatives {
  vec3 lambda;
  vec3 ddx;
  vec3 ddy;
};

struct SGradientVec2 {
  vec2 lambda;
  vec2 ddx;
  vec2 ddy;
};

struct SGradientVec3 {
  vec3 lambda;
  vec3 ddx;
  vec3 ddy;
};

SPartialDerivatives CalculatePartialDerivatives(in vec4 clip0, in vec4 clip1, in vec4 clip2, in vec2 p) {
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
  ddy *= -2.0 / screenSize.y;
  ddxSum *= 2.0 / screenSize.x;
  ddySum *= -2.0 / screenSize.y;

  const float ddxInterpW = 1.0 / (interpInvW + ddxSum);
  const float ddyInterpW = 1.0 / (interpInvW + ddySum);

  ddx = ddxInterpW * (lambda * interpInvW + ddx) - lambda;
  ddy = ddyInterpW * (lambda * interpInvW + ddy) - lambda;

  return SPartialDerivatives(lambda, ddx, ddy);
}

vec3 InterpolateWithDerivatives(in SPartialDerivatives derivatives, in vec3 values) {
  return vec3(
    dot(derivatives.lambda, values),
    dot(derivatives.ddx, values),
    dot(derivatives.ddy, values)
  );
}

vec3 Interpolate(in SPartialDerivatives derivatives, in vec3[3] values) {
  return
    derivatives.lambda.x * values[0] +
    derivatives.lambda.y * values[1] +
    derivatives.lambda.z * values[2];
}

vec4 Interpolate(in SPartialDerivatives derivatives, in vec4[3] values) {
  return
    derivatives.lambda.x * values[0] +
    derivatives.lambda.y * values[1] +
    derivatives.lambda.z * values[2];
}

SGradientVec2 MakeGradient(in SPartialDerivatives derivatives, in vec2[3] values) {
  const vec3 v0 = InterpolateWithDerivatives(derivatives, vec3(values[0].x, values[1].x, values[2].x));
  const vec3 v1 = InterpolateWithDerivatives(derivatives, vec3(values[0].y, values[1].y, values[2].y));
  return SGradientVec2(
    vec2(v0.x, v1.x),
    vec2(v0.y, v1.y),
    vec2(v0.z, v1.z)
  );
}

SGradientVec3 MakeGradient(in SPartialDerivatives derivatives, in vec3[3] values) {
  const vec3 v0 = InterpolateWithDerivatives(derivatives, vec3(values[0].x, values[1].x, values[2].x));
  const vec3 v1 = InterpolateWithDerivatives(derivatives, vec3(values[0].y, values[1].y, values[2].y));
  const vec3 v2 = InterpolateWithDerivatives(derivatives, vec3(values[0].z, values[1].z, values[2].z));
  return SGradientVec3(
    vec3(v0.x, v1.x, v2.x),
    vec3(v0.y, v1.y, v2.y),
    vec3(v0.z, v1.z, v2.z)
  );
}

vec3 SampleBaseColor(in SGradientVec2 uv, in uint baseColorTexture) {
  if (baseColorTexture == uint(-1)) {
    return vec3(1.0);
  }
  return textureGrad(
    RetinaNonUniform(
      sampler2D(
        RetinaGetSampledImage(Texture2D, baseColorTexture),
        g_LinearSampler
      )
    ),
    uv.lambda,
    uv.ddx,
    uv.ddy
  ).rgb;
}

vec3 SampleNormal(in SGradientVec2 uv, in uint normalTexture) {
  if (normalTexture == uint(-1)) {
    return vec3(0.0, 0.0, 0.0);
  }
  const vec2 sampledNormal = textureGrad(
    RetinaNonUniform(
      sampler2D(
        RetinaGetSampledImage(Texture2D, normalTexture),
        g_LinearSampler
      )
    ),
    uv.lambda,
    uv.ddx,
    uv.ddy
  ).rg;
  const float z = sqrt(max(1.0 - dot(sampledNormal, sampledNormal), 0.0));
  return vec3(sampledNormal, z);
}

vec2 EncodeNormalOctahedral(in vec3 normal) {
  const vec2 wrapped = RetinaOctahedralWrap(normal.xy);
  normal /= abs(normal.x) + abs(normal.y) + abs(normal.z);
  normal.xy = normal.z >= 0.0 ? normal.xy : wrapped;
  return normal.xy * 0.5 + 0.5;
}

void main() {
  const uint payload = texelFetch(g_VisbufferMain, ivec2(gl_FragCoord.xy), 0).r;
  const uint shaderId = 0; // TODO: implement
  if (payload == -1u) {
    o_Albedo = vec4(0.0, 0.0, 0.0, 1.0);
    o_Normal = vec2(0.0);
    o_ShaderMaterialId = -1;
    return;
  }
  const uint meshletInstanceIndex = payload >> MESHLET_VISBUFFER_PRIMITIVE_ID_BITS;
  const uint meshletPrimitiveId = payload & MESHLET_VISBUFFER_PRIMITIVE_ID_MASK;
  const SViewInfo mainView = g_ViewInfoBuffer.Data[0];
  const SMeshletInstance meshletInstance = g_MeshletInstanceBuffer.Data[meshletInstanceIndex];
  const SMeshlet meshlet = g_MeshletBuffer.Data[meshletInstance.MeshletIndex];
  const mat4 transform = g_TransformBuffer.Data[meshletInstance.TransformIndex];
  const mat4 pvm = mainView.JitterProj * mainView.View * transform;
  const uvec3 indices = uvec3(
    meshlet.VertexOffset + g_IndexBuffer.Data[meshlet.IndexOffset + uint(g_PrimitiveBuffer.Data[meshlet.PrimitiveOffset + meshletPrimitiveId * 3 + 0])],
    meshlet.VertexOffset + g_IndexBuffer.Data[meshlet.IndexOffset + uint(g_PrimitiveBuffer.Data[meshlet.PrimitiveOffset + meshletPrimitiveId * 3 + 1])],
    meshlet.VertexOffset + g_IndexBuffer.Data[meshlet.IndexOffset + uint(g_PrimitiveBuffer.Data[meshlet.PrimitiveOffset + meshletPrimitiveId * 3 + 2])]
  );
  const vec4 clipVertex0 = pvm * vec4(g_PositionBuffer.Data[indices.x], 1.0);
  const vec4 clipVertex1 = pvm * vec4(g_PositionBuffer.Data[indices.y], 1.0);
  const vec4 clipVertex2 = pvm * vec4(g_PositionBuffer.Data[indices.z], 1.0);
  const SPartialDerivatives derivatives = CalculatePartialDerivatives(clipVertex0, clipVertex1, clipVertex2, i_Uv * 2.0 - 1.0);

  const SMeshletVertex[3] vertexData = SMeshletVertex[](
    g_VertexBuffer.Data[indices.x],
    g_VertexBuffer.Data[indices.y],
    g_VertexBuffer.Data[indices.z]
  );
  const SGradientVec2 uv = MakeGradient(derivatives, vec2[](
    vertexData[0].Uv,
    vertexData[1].Uv,
    vertexData[2].Uv
  ));
  const vec3 normal = Interpolate(derivatives, vec3[](
    vertexData[0].Normal,
    vertexData[1].Normal,
    vertexData[2].Normal
  ));
  const vec4 tangent = Interpolate(derivatives, vec4[](
    vertexData[0].Tangent,
    vertexData[1].Tangent,
    vertexData[2].Tangent
  ));
  const vec3 bitangent = cross(normal, tangent.xyz) * tangent.w;

  const mat3 normalTransform = transpose(inverse(mat3(transform)));
  vec3 albedo = vec3(0.0);
  vec2 encodedNormal = vec2(0.0);
  if (meshletInstance.MaterialIndex != uint(-1)) {
    const SMaterial material = g_MaterialBuffer.Data[meshletInstance.MaterialIndex];
    const vec3 baseColorFactor = material.BaseColorFactor;
    const vec3 sampledBaseColor = SampleBaseColor(uv, material.BaseColorTexture);
    const vec3 sampledBaseNormal = SampleNormal(uv, material.NormalTexture);
    const mat3 TBN = mat3(
      normalize(normalTransform * tangent.xyz),
      normalize(normalTransform * bitangent),
      normalize(normalTransform * normal)
    );
    const vec3 worldNormal = material.NormalTexture == uint(-1)
      ? TBN[2]
      : normalize(TBN * (sampledBaseNormal * 2.0 - 1.0));
    albedo = baseColorFactor * sampledBaseColor;
    encodedNormal = EncodeNormalOctahedral(worldNormal);
  }

  o_Albedo = vec4(albedo, 1.0);
  o_Normal = encodedNormal;
  o_ShaderMaterialId = shaderId << 16 | meshletInstance.MaterialIndex;
}
