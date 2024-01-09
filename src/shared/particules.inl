#ifdef __cplusplus
#pragma once
#endif //__cplusplus
#include "../backend/backend.inl"

#define PARTICULES_COUNT 64

BUFFER_REF(4)
Particule
{
    f32vec3 pos;    // Particule position
    f32vec3 vel;    // Particule velocity
    // color
};

struct ParticulesPC
{
    VkDeviceAddress particule_in;
    VkDeviceAddress particule_out;
};