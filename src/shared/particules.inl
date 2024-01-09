#ifdef __cplusplus
#pragma once
#endif //__cplusplus
#include "../backend/backend.inl"

#define PARTICULES_COUNT 64
// Loanie
BUFFER_REF(4)
Particule
{
    f32vec3 pos;    // Particule position
    f32vec3 vel;    // Particule velocity
    // color
};

// Do I need?
struct ParticulesPC
{
    VkDeviceAddress particule_kernel;
    //u32 ss_normals_index;
    //u32 depth_index;
    //u32 ambient_occlusion_index;
    //i32vec2 extent;
};