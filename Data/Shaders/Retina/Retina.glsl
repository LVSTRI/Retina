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
#extension GL_EXT_samplerless_texture_functions : require

#define RETINA_SAMPLER_DESCRIPTOR_BINDING 0
#define RETINA_SAMPLED_IMAGE_BINDING 1
#define RETINA_STORAGE_IMAGE_BINDING 2


#endif
