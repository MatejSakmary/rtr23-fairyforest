#ifndef __cplusplus
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
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

/// NOTE: Must match the ones in gpu_resource_table.cpp
#define BUFFER_BINDING 0
#define STORAGE_IMAGE_BINDING 1
#define SAMPLED_IMAGE_BINDING 2
#define SAMPLER_BINDING 3

#define f32 float
#define f32vec2 vec2
#define f32mat2x2 mat2x2
#define f32mat2x3 mat2x3
#define f32mat2x4 mat2x4
#define f32vec3 vec3
#define f32mat3x2 mat3x2
#define f32mat3x3 mat3x3
#define f32mat3x4 mat3x4
#define f32vec4 vec4
#define f32mat4x2 mat4x2
#define f32mat4x3 mat4x3
#define f32mat4x4 mat4x4
#define i32 int
#define i32vec2 ivec2
#define i32vec3 ivec3
#define i32vec4 ivec4
#define u32 uint
#define u32vec2 uvec2
#define u32vec3 uvec3
#define u32vec4 uvec4
#define i64 int64_t
#define i64vec2 i64vec2
#define i64vec3 i64vec3
#define i64vec4 i64vec4
#define u64 uint64_t
#define u64vec2 u64vec2
#define u64vec3 u64vec3
#define u64vec4 u64vec4
#define b32 bool
#define b32vec2 bvec2
#define b32vec3 bvec3
#define b32vec4 bvec4

#define VkDeviceAddress u64

#define BUFFER_REF(ALIGN) layout(buffer_reference, scalar, buffer_reference_align = ALIGN) buffer
#else
#pragma once
#include "backend.hpp"
using namespace ff::types;
#define BUFFER_REF(ALIGN) struct
#endif