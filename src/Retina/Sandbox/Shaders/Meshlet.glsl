#ifndef RETINA_SHADER_MESHLET_HEADER
#define RETINA_SHADER_MESHLET_HEADER

#define MESHLET_INDEX_COUNT 64
#define MESHLET_PRIMITIVE_COUNT 124

#define MESHLET_VISBUFFER_MESHLET_INDEX_BITS 25
#define MESHLET_VISBUFFER_PRIMITIVE_ID_BITS 7
#define MESHLET_VISBUFFER_MESHLET_ID_MASK ((1 << MESHLET_VISBUFFER_MESHLET_INDEX_BITS) - 1)
#define MESHLET_VISBUFFER_PRIMITIVE_ID_MASK ((1 << MESHLET_VISBUFFER_PRIMITIVE_ID_BITS) - 1)

#define SHADOW_CASCADE_COUNT 24

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

struct SMeshletVertex {
  vec3 Normal;
  vec2 Uv;
  vec4 Tangent;
};

struct SViewInfo {
  mat4 Projection;
  mat4 FiniteProjection;
  mat4 View;
  mat4 ProjView;
};

struct SShadowCascadeInfo {
  mat4 Projection;
  mat4 View;
  mat4 ProjView;
  mat4 Global;
  vec4 Scale;
  vec4 Offset;
};

#endif