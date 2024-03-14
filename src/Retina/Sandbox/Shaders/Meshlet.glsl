#ifndef RETINA_SHADER_MESHLET_HEADER
#define RETINA_SHADER_MESHLET_HEADER

#define MESHLET_INDEX_COUNT 64
#define MESHLET_PRIMITIVE_COUNT 124

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
