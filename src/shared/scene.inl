#ifdef __cplusplus
#pragma once
#endif //__cplusplus
#include "../backend/backend.inl"
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
    u32 fif_index;
    u32 mesh_index;
    u32 sampler_id;
    f32vec3 sun_direction;
};