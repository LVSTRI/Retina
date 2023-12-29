#ifndef RETINA_SHADERS_BINDINGS_HEADER
#define RETINA_SHADERS_BINDINGS_HEADER

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
#extension GL_EXT_samplerless_texture_functions : require

// Private
#define RETINA_SAMPLER_DESCRIPTOR_BINDING 0
#define RETINA_SAMPLED_IMAGE_DESCRIPTOR_BINDING 1
#define RETINA_STORAGE_IMAGE_DESCRIPTOR_BINDING 2
#define RETINA_UNIFORM_BUFFER_DESCRIPTOR_BINDING 3
#define RETINA_STORAGE_BUFFER_DESCRIPTOR_BINDING 4

#define RETINA_SAMPLER_LAYOUT layout (set = 0, binding = RETINA_SAMPLER_DESCRIPTOR_BINDING)
#define RETINA_SAMPLED_IMAGE_LAYOUT layout (set = 0, binding = RETINA_SAMPLED_IMAGE_DESCRIPTOR_BINDING)
#define RETINA_STORAGE_IMAGE_LAYOUT layout (set = 0, binding = RETINA_STORAGE_IMAGE_DESCRIPTOR_BINDING)
#define RETINA_UNIFORM_BUFFER_LAYOUT layout (set = 0, binding = RETINA_UNIFORM_BUFFER_DESCRIPTOR_BINDING)
#define RETINA_STORAGE_BUFFER_LAYOUT layout (set = 0, binding = RETINA_STORAGE_BUFFER_DESCRIPTOR_BINDING)

#define RETINA_DECLARE_SAMPLER_DESCRIPTOR()                             \
    RETINA_SAMPLER_LAYOUT uniform sampler[] u_samplerTable;             \
    RETINA_SAMPLER_LAYOUT uniform samplerShadow[] u_samplerShadowTable

#define RETINA_DECLARE_SAMPLED_IMAGE_DESCRIPTOR(type, name) \
    RETINA_SAMPLED_IMAGE_LAYOUT uniform type[] u_sampledImageTable_##name

#define RETINA_DECLARE_STORAGE_IMAGE_DESCRIPTOR(type, name) \
    RETINA_STORAGE_IMAGE_LAYOUT uniform type[] u_storageImageTable_##name

#define RETINA_DECLARE_UNIFORM_BUFFER_DESCRIPTOR(type)    \
    RETINA_UNIFORM_BUFFER_LAYOUT uniform U##type {        \
        type Data;                                        \
    } u_uniformBufferTable_##type[]

#define RETINA_DECLARE_STORAGE_BUFFER_DESCRIPTOR(qualifier, name, block)   \
    RETINA_STORAGE_BUFFER_LAYOUT qualifier buffer name block b_storageBufferTable_##name[]

RETINA_DECLARE_SAMPLER_DESCRIPTOR();

// Public
#define RetinaGetSampler(id) u_samplerTable[id]
#define RetinaGetSamplerShadow(id) u_samplerShadowTable[id]

#define RetinaDeclareSampledImage(type, name) RETINA_DECLARE_SAMPLED_IMAGE_DESCRIPTOR(type, name)
#define RetinaGetSampledImage(name, id) u_sampledImageTable_##name[id]

#define RetinaDeclareStorageImage(type, name) RETINA_DECLARE_STORAGE_IMAGE_DESCRIPTOR(type, name)
#define RetinaGetStorageImage(name, id) u_storageImageTable_##name[id]

#define RetinaDeclareUniformBuffer(type) RETINA_DECLARE_UNIFORM_BUFFER_DESCRIPTOR(type)
#define RetinaGetUniformBuffer(type, id) u_uniformBufferTable_##type[id]
#define RetinaGetUniformBufferData(type, id) RetinaGetUniformBuffer(type, id).Data

#define RetinaDeclareStorageBuffer(qualifier, name, block) RETINA_DECLARE_STORAGE_BUFFER_DESCRIPTOR(qualifier, name, block)
#define RetinaGetStorageBuffer(name, id) b_storageBufferTable_##name[id]


#endif
