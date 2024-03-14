#include <Retina/Retina.glsl>
#include <Meshlet.glsl>

layout (location = 0) in SVertexData {
  flat uint MeshletInstanceIndex;
} i_VertexData;

layout (location = 0) out uint o_Pixel;

void main() {
  o_Pixel = i_VertexData.MeshletInstanceIndex << MESHLET_VISBUFFER_PRIMITIVE_ID_BITS | gl_PrimitiveID;
}
