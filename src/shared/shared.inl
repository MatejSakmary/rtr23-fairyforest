#ifdef __cplusplus
#pragma once
#endif //__cplusplus
#include "../backend/backend.inl"

#define SKY_COLOR f32vec3(0.0015, 0.0015, 0.0075)
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
    f32mat4x4 view;
    f32mat4x4 inverse_view;
    f32mat4x4 projection;
    f32mat4x4 inverse_projection;
    f32mat4x4 view_projection;
    f32mat4x4 inverse_view_projection;
};

struct DrawPc
{
    VkDeviceAddress scene_descriptor;
    VkDeviceAddress camera_info;
    u32 ss_normals_index;
    u32 ssao_index;
    u32 fif_index;
    u32 mesh_index;
    u32 sampler_id;
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