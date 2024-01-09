#ifdef __cplusplus
#pragma once
#endif //__cplusplus
#include "../backend/backend.inl"

#define SSAO_X_TILE_SIZE 16
#define SSAO_Y_TILE_SIZE 16

#define SSAO_KERNEL_SAMPLE_COUNT 64

BUFFER_REF(4)
SSAOKernel
{
    f32vec3 sample_pos;
};

struct SSAOPC
{
    VkDeviceAddress SSAO_kernel;
    u32 ss_normals_index;
    u32 depth_index;
    u32 ambient_occlusion_index;
    i32vec2 extent;
};