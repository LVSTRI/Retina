#extension GL_EXT_mesh_shader : require

#include <Retina/Retina.glsl>
#include <Meshlet.glsl>

#define WORK_GROUP_SIZE 32
#define MAX_INDICES_PER_THREAD ((MESHLET_INDEX_COUNT + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE)
#define MAX_PRIMITIVES_PER_THREAD ((MESHLET_PRIMITIVE_COUNT + WORK_GROUP_SIZE - 1) / WORK_GROUP_SIZE)

layout (location = 0) out SVertexData {
  flat uint MeshletInstanceIndex;
  vec4 ClipPosition;
  vec4 PrevClipPosition;
} o_VertexData[];

RetinaDeclarePushConstant() {
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

shared vec3 sh_ClipVertices[MESHLET_INDEX_COUNT];

layout (local_size_x = WORK_GROUP_SIZE) in;
layout (triangles, max_vertices = MESHLET_INDEX_COUNT, max_primitives = MESHLET_PRIMITIVE_COUNT) out;
void main() {
  const uint meshletInstanceIndex = gl_WorkGroupID.x;
  const SMeshletInstance meshletInstance = g_MeshletInstanceBuffer.Data[meshletInstanceIndex];
  const SMeshlet meshlet = g_MeshletBuffer.Data[meshletInstance.MeshletIndex];
  const SViewInfo mainView = g_ViewInfoBuffer.Data[0];
  const mat4 transform = g_TransformBuffer.Data[meshletInstance.TransformIndex];

  SetMeshOutputsEXT(meshlet.IndexCount, meshlet.PrimitiveCount);
  for (uint i = 0; i < MAX_INDICES_PER_THREAD; i++) {
    const uint id = min(gl_LocalInvocationID.x + i * WORK_GROUP_SIZE, meshlet.IndexCount - 1);
    const uint index = g_IndexBuffer.Data[meshlet.IndexOffset + id];
    const vec3 position = g_PositionBuffer.Data[meshlet.VertexOffset + index];
    const vec4 clipJitter = mainView.JitterProj * mainView.View * transform * vec4(position, 1.0);
    const vec4 clip = mainView.ProjView * transform * vec4(position, 1.0);
    const vec4 prevClip = mainView.PrevProjView * transform * vec4(position, 1.0);
    o_VertexData[id].MeshletInstanceIndex = meshletInstanceIndex;
    o_VertexData[id].ClipPosition = clip;
    o_VertexData[id].PrevClipPosition = prevClip;
    sh_ClipVertices[id] = vec3(clipJitter.xyw);
    gl_MeshVerticesEXT[id].gl_Position = clipJitter;
  }
  barrier();

  for (uint i = 0; i < MAX_PRIMITIVES_PER_THREAD; i++) {
    const uint id = min(gl_LocalInvocationID.x + i * WORK_GROUP_SIZE, meshlet.PrimitiveCount - 1);
    const uvec3 indices = uvec3(
      uint(g_PrimitiveBuffer.Data[meshlet.PrimitiveOffset + id * 3 + 0]),
      uint(g_PrimitiveBuffer.Data[meshlet.PrimitiveOffset + id * 3 + 1]),
      uint(g_PrimitiveBuffer.Data[meshlet.PrimitiveOffset + id * 3 + 2])
    );
    const vec3 v0 = sh_ClipVertices[indices.x];
    const vec3 v1 = sh_ClipVertices[indices.y];
    const vec3 v2 = sh_ClipVertices[indices.z];
    const float det = determinant(mat3(v0, v1, v2));
    gl_PrimitiveTriangleIndicesEXT[id] = indices;
    gl_MeshPrimitivesEXT[id].gl_PrimitiveID = int(id);
    gl_MeshPrimitivesEXT[id].gl_CullPrimitiveEXT = det > 0.0;
  }
}
