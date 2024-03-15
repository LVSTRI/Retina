#ifndef RETINA_SHADERS_RETINA_HEADER
#define RETINA_SHADERS_RETINA_HEADER

#extension GL_ARB_separate_shader_objects : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_float16 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_float32 : enable
#extension GL_KHR_shader_subgroup_basic : enable
#extension GL_KHR_shader_subgroup_vote : enable
#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_ballot : enable
#extension GL_KHR_shader_subgroup_shuffle : enable
#extension GL_KHR_shader_subgroup_shuffle_relative : enable
#extension GL_KHR_shader_subgroup_clustered : enable
#extension GL_KHR_shader_subgroup_quad : enable
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_image_load_formatted : require
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_shader_image_int64 : require
#extension GL_EXT_shader_atomic_int64 : require
#extension GL_EXT_samplerless_texture_functions : require

#define RETINA_SAMPLER_DESCRIPTOR_BINDING 0
#define RETINA_SAMPLED_IMAGE_BINDING 1
#define RETINA_STORAGE_IMAGE_BINDING 2
#define RETINA_STORAGE_BUFFER_BINDING 3

#define RETINA_SAMPLER_LAYOUT layout (set = 0, binding = RETINA_SAMPLER_DESCRIPTOR_BINDING)
#define RETINA_SAMPLED_IMAGE_LAYOUT layout (set = 0, binding = RETINA_SAMPLED_IMAGE_BINDING)
#define RETINA_STORAGE_IMAGE_LAYOUT layout (set = 0, binding = RETINA_STORAGE_IMAGE_BINDING)
#define RETINA_STORAGE_IMAGE_LAYOUT_WITH_FORMAT(format) layout (format, set = 0, binding = RETINA_STORAGE_IMAGE_BINDING)
#define RETINA_BUFFER_POINTER_LAYOUT layout (scalar, buffer_reference)

#define RETINA_DECLARE_SAMPLER_DESCRIPTOR()                           \
  RETINA_SAMPLER_LAYOUT uniform sampler[] u_SamplerTable;             \
  RETINA_SAMPLER_LAYOUT uniform samplerShadow[] u_SamplerShadowTable

#define RETINA_DECLARE_SAMPLED_IMAGE_DESCRIPTOR(type, name) \
  RETINA_SAMPLED_IMAGE_LAYOUT uniform type[] u_SampledImageTable_##name

#define RETINA_DECLARE_STORAGE_IMAGE_DESCRIPTOR(type, name) \
  RETINA_STORAGE_IMAGE_LAYOUT uniform type[] u_StorageImageTable_##name

#define RETINA_DECLARE_STORAGE_IMAGE_DESCRIPTOR_WITH_FORMAT(format, type, name) \
  RETINA_STORAGE_IMAGE_LAYOUT_WITH_FORMAT(format) uniform type[] u_StorageImageTable_##name

#define RETINA_DECLARE_BUFFER_TYPE(name) \
  RETINA_BUFFER_POINTER_LAYOUT buffer SBufferPointerType_##name

#define RETINA_DECLARE_QUALIFIED_BUFFER_TYPE(qualifier, name) \
  RETINA_BUFFER_POINTER_LAYOUT qualifier buffer SBufferPointerType_##name

RETINA_DECLARE_SAMPLER_DESCRIPTOR();

layout (set = 0, binding = RETINA_STORAGE_BUFFER_BINDING)
restrict readonly buffer SAddressTable {
  uint64_t[] Data;
} b_AddressTable;

#define RetinaGetSampler(id) u_SamplerTable[id]
#define RetinaGetSamplerShadow(id) u_SamplerShadowTable[id]

#define RetinaDeclareSampledImage(type, name) RETINA_DECLARE_SAMPLED_IMAGE_DESCRIPTOR(type, name)
#define RetinaGetSampledImage(name, id) u_SampledImageTable_##name[id]

#define RetinaDeclareStorageImage(type, name) RETINA_DECLARE_STORAGE_IMAGE_DESCRIPTOR(type, name)
#define RetinaDeclareStorageImageWithFormat(format, type, name) RETINA_DECLARE_STORAGE_IMAGE_DESCRIPTOR_WITH_FORMAT(format, type, name)
#define RetinaGetStorageImage(name, id) u_StorageImageTable_##name[id]

#define RetinaDeclareBuffer(name) RETINA_DECLARE_BUFFER_TYPE(name)
#define RetinaDeclareQualifiedBuffer(qualifier, name) RETINA_DECLARE_QUALIFIED_BUFFER_TYPE(qualifier, name)
#define RetinaGetBufferType(name) SBufferPointerType_##name
#define RetinaGetBufferPointer(name, id) RetinaGetBufferType(name)(b_AddressTable.Data[id])
#define RetinaDeclareBufferPointer(type, name, id) RetinaGetBufferType(type) name = RetinaGetBufferPointer(type, id)

#define RetinaDeclarePushConstant() layout (push_constant) readonly uniform SPushConstant

#endif
