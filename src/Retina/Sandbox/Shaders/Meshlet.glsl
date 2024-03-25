#ifndef RETINA_SHADER_MESHLET_HEADER
#define RETINA_SHADER_MESHLET_HEADER

#define MESHLET_INDEX_COUNT 64
#define MESHLET_PRIMITIVE_COUNT 124

#define MESHLET_VISBUFFER_MESHLET_INDEX_BITS 25
#define MESHLET_VISBUFFER_PRIMITIVE_ID_BITS 7
#define MESHLET_VISBUFFER_MESHLET_ID_MASK ((1 << MESHLET_VISBUFFER_MESHLET_INDEX_BITS) - 1)
#define MESHLET_VISBUFFER_PRIMITIVE_ID_MASK ((1 << MESHLET_VISBUFFER_PRIMITIVE_ID_BITS) - 1)

#define SHADOW_CASCADE_COUNT 16

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
  uint MaterialIndex;
};

struct SMaterial {
  vec3 BaseColorFactor;
  uint BaseColorTexture;
  uint NormalTexture;
};

struct SMeshletVertex {
  vec3 Normal;
  vec2 Uv;
  vec4 Tangent;
};

struct SViewInfo {
  mat4 Projection;
  mat4 PrevProjection;
  mat4 JitterProj;
  mat4 PrevJitterProj;
  mat4 View;
  mat4 PrevView;
  mat4 ProjView;
  mat4 PrevProjView;
  vec4 Position;
};

#endif
