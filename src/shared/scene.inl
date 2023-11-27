#include "../backend/backend.inl"
#pragma once
BUFFER_REF(4)
SceneDescriptor
{
    VkDeviceAddress mesh_descriptors_start;
    VkDeviceAddress material_descriptors_start;
    VkDeviceAddress transforms_start;
    VkDeviceAddress positions_start;
    VkDeviceAddress uvs_start;
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
    u32 indices_offset;
    u32 material_index;
};

struct DrawPc
{
    VkDeviceAddress scene_descriptor;
    f32mat4x4 view_proj;
    u32 mesh_index;
};