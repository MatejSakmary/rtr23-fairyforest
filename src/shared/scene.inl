#pragma once

#include "../backend/backend.inl"
BUFFER_REF(4)
SceneDescriptor
{
    VkDeviceAddress mesh_descriptors_start;
    VkDeviceAddress material_descriptors_start;
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
    VkDeviceAddress transforms_start;
    VkDeviceAddress positions_start;
    VkDeviceAddress uvs_start;
    VkDeviceAddress indices_start;
    u32 material_index;
};