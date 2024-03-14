#ifndef RETINA_SHADER_MESHLET_HEADER
#define RETINA_SHADER_MESHLET_HEADER

#define MESHLET_INDEX_COUNT 64
#define MESHLET_PRIMITIVE_COUNT 124

#define MESHLET_VISBUFFER_MESHLET_INDEX_BITS 25
#define MESHLET_VISBUFFER_PRIMITIVE_ID_BITS 7
#define MESHLET_VISBUFFER_MESHLET_ID_MASK ((1 << MESHLET_VISBUFFER_MESHLET_INDEX_BITS) - 1)
#define MESHLET_VISBUFFER_PRIMITIVE_ID_MASK ((1 << MESHLET_VISBUFFER_PRIMITIVE_ID_BITS) - 1)

struct SMeshlet {
  uint VertexOffset;
  uint IndexOffset;
  uint IndexCount;
  uint PrimitiveOffset;
  uint PrimitiveCount;
};

struct SMeshletInstance {
  uint MeshletIndex;
  uint TransformIndex;
};

#endif
