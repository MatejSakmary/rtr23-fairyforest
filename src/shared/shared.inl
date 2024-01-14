#ifdef __cplusplus
#pragma once
#endif //__cplusplus
#include "../backend/backend.inl"

#define SKY_COLOR f32vec3(0.0015 * 0.1, 0.0015 * 0.1, 0.0075 * 0.1)

BUFFER_REF(4)
SceneDescriptor
{
    VkDeviceAddress mesh_descriptors_start;
    VkDeviceAddress material_descriptors_start;
    VkDeviceAddress transforms_start;
    VkDeviceAddress positions_start;
    VkDeviceAddress uvs_start;
    VkDeviceAddress normals_start;
    VkDeviceAddress tangents_start;
    VkDeviceAddress indices_start;
};

BUFFER_REF(4)
MaterialDescriptor
{
    f32vec4 base_color;
    u32 albedo_index;
    u32 normal_index;
};

BUFFER_REF(4)
MeshDescriptor
{
    u32 transforms_offset;
    u32 positions_offset;
    u32 uvs_offset;
    u32 tangents_offset;
    u32 normals_offset;
    u32 indices_offset;
    u32 material_index;
};

BUFFER_REF(4)
CameraInfoBuf
{
    f32vec3 position;
    f32vec3 frust_right_offset;
    f32vec3 frust_top_offset;
    f32vec3 frust_front;
    f32mat4x4 view;
    f32mat4x4 inverse_view;
    f32mat4x4 projection;
    f32mat4x4 inverse_projection;
    f32mat4x4 view_projection;
    f32mat4x4 inverse_view_projection;
};

BUFFER_REF(4)
ShadowmapCascadeData
{
    f32mat4x4 cascade_view_matrix;
    f32mat4x4 cascade_proj_matrix;
    f32 cascade_far_depth;
    f32 far_plane;
};

struct DrawPc
{
    VkDeviceAddress scene_descriptor;
    VkDeviceAddress camera_info;
    VkDeviceAddress cascade_data;
    u32 ss_normals_index;
    u32 ssao_index;
    u32 esm_shadowmap_index;
    u32 fif_index;
    u32 mesh_index;
    u32 sampler_id;
    u32 shadow_sampler_id;
    f32vec3 sun_direction;
    u32 enable_ao;
};

// SSAO 
#define SSAO_X_TILE_SIZE 16
#define SSAO_Y_TILE_SIZE 16
#define SSAO_KERNEL_NOISE_SIZE 4

#define SSAO_KERNEL_SAMPLE_COUNT 32

BUFFER_REF(4)
SSAOKernel
{
    f32vec3 sample_pos;
};

struct SSAOPC
{
    VkDeviceAddress SSAO_kernel;
    VkDeviceAddress camera_info;
    u32 fif_index;
    u32 ss_normals_index;
    u32 kernel_noise_index;
    u32 depth_index;
    u32 ambient_occlusion_index;
    i32vec2 extent;
    bool use_shared;
};

// Shadows
#define NUM_CASCADES 2
#define SHADOWMAP_RESOLUTION 2048
#define DEPTH_PASS_TILE_SIZE 24
#define DEPTH_PASS_WG_SIZE (u32vec2(DEPTH_PASS_TILE_SIZE))
#define DEPTH_PASS_THREAD_READ_COUNT (u32vec2(2))
#define DEPTH_PASS_WG_READS_PER_AXIS (DEPTH_PASS_WG_SIZE * DEPTH_PASS_THREAD_READ_COUNT)

#define ESM_BLUR_WORKGROUP_SIZE 64
#define ESM_FACTOR 100.0

#define LAMBDA 0.76
BUFFER_REF(4)
DepthLimits
{
    f32vec2 limits;
};

struct AnalyzeDepthPC
{
    VkDeviceAddress depth_limits;
    u32vec2 depth_dimensions;
    u32 sampler_id;
    u32 prev_thread_count;
    u32 depth_index;
};

struct WriteShadowMatricesPC
{
    VkDeviceAddress camera_info;
    u32 fif_index;
    VkDeviceAddress depth_limits;
    VkDeviceAddress cascade_data;
    f32vec3 sun_direction;
};

struct ShadowPC
{
    VkDeviceAddress scene_descriptor;
    VkDeviceAddress cascade_data;
    u32 mesh_index;
    u32 sampler_id;
    u32 cascade_index;
};

struct ESMShadowPC
{
    u32 tmp_esm_index;
    u32 esm_index;
    u32 shadowmap_index;
    u32 cascade_index;
    u32vec2 offset;
};
