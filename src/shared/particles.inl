#ifdef __cplusplus
#pragma once
#endif //__cplusplus
#include "../backend/backend.inl"

#define PARTICLES_X_TILE_SIZE 16
#define PARTICLES_Y_TILE_SIZE 16
#define PARTICLES_Z_TILE_SIZE 8

#define PARTICLES_COUNT 256


BUFFER_REF(4)
Particle
{
    f32vec3 pos;    // Particle position
    f32vec3 vel;    // Particle velocity
    // float ttl;      // Time to live
};

struct ParticlesPC
{
    VkDeviceAddress particles_in;
    VkDeviceAddress particles_out;
    
    // Should I just use DrawPC?
    f32mat4x4 view;
    u32 sampler_id;

    // VkDeviceAddress atomic_count; // TODO(lgress) : See how to make a proper atomic counter with Vulkan
    // VkDeviceAddress last_count; 
};