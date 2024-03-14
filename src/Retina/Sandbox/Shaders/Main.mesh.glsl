#extension GL_EXT_mesh_shader : require

#include <Retina/Retina.glsl>

const vec3[] trianglePositions = vec3[](
  vec3( 0.0, -0.5, 0.0),
  vec3(-0.5,  0.5, 0.0),
  vec3( 0.5,  0.5, 0.0)
);

const vec3[] triangleColors = vec3[](
  vec3(1.0, 0.0, 0.0),
  vec3(0.0, 1.0, 0.0),
  vec3(0.0, 0.0, 1.0)
);

layout (location = 0) out SVertexData {
  vec3 Color;
} o_VertexData[];

RetinaDeclarePushConstant() {
  uint u_ViewBufferIndex;
};

RetinaDeclareQualifiedBuffer(restrict readonly, SViewInfoBuffer) {
  mat4 Projection;
  mat4 View;
  mat4 ProjView;
};
RetinaDeclareBufferPointer(SViewInfoBuffer, g_ViewInfoBuffer, u_ViewBufferIndex);

layout (local_size_x = 1) in;
layout (triangles, max_vertices = 3, max_primitives = 1) out;
void main() {
  SetMeshOutputsEXT(3, 1);
  for (uint i = 0; i < 3; i++) {
    o_VertexData[i].Color = triangleColors[i];
    gl_MeshVerticesEXT[i].gl_Position = g_ViewInfoBuffer.ProjView * vec4(trianglePositions[i], 1.0);
  }

  gl_PrimitiveTriangleIndicesEXT[0] = uvec3(0, 1, 2);
  gl_MeshPrimitivesEXT[0].gl_PrimitiveID = 0;
}
