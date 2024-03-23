#include <Retina/Retina.glsl>
#include <Meshlet.glsl>

layout (location = 0) in SVertexData {
  flat uint MeshletInstanceIndex;
  vec4 ClipPosition;
  vec4 PrevClipPosition;
} i_VertexData;

layout (location = 0) out uint o_Pixel;
layout (location = 1) out vec2 o_Velocity;

vec2 GetVelocity() {
  const vec4 ndcPosition = i_VertexData.ClipPosition / i_VertexData.ClipPosition.w;
  const vec4 prevNdcPosition = i_VertexData.PrevClipPosition / i_VertexData.PrevClipPosition.w;
  const vec2 uvPosition = ndcPosition.xy * 0.5 + 0.5;
  const vec2 prevUvPosition = prevNdcPosition.xy * 0.5 + 0.5;
  return prevUvPosition - uvPosition;
}

void main() {
  o_Pixel = i_VertexData.MeshletInstanceIndex << MESHLET_VISBUFFER_PRIMITIVE_ID_BITS | gl_PrimitiveID;
  o_Velocity = GetVelocity();
}
