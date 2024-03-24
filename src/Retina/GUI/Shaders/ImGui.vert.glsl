#include <Retina/Retina.glsl>
#include <Retina/Utility.glsl>

struct SVertexFormat {
  vec2 Position;
  vec2 Uv;
  uint Color;
};

layout (location = 0) out vec4 o_Color;
layout (location = 1) out vec2 o_Uv;

RetinaDeclarePushConstant() {
  uint u_VertexBufferId;
  uint u_SamplerId;
  uint u_TextureId;
  vec2 u_Scale;
  vec2 u_Translate;
};

RetinaDeclareQualifiedBuffer(restrict readonly, SVertexBuffer) {
  SVertexFormat[] Data;
};
RetinaDeclareBufferPointer(SVertexBuffer, g_VertexBuffer, u_VertexBufferId);

void main() {
  const SVertexFormat vertex = g_VertexBuffer.Data[gl_VertexIndex];
  o_Uv = vertex.Uv;
  o_Color = unpackUnorm4x8(vertex.Color);
  gl_Position = vec4(vertex.Position * u_Scale + u_Translate, 0.0, 1.0);
}
