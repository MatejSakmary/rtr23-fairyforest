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
    //f32vec3 vel;    // Particle velocity
    // color
};

struct ParticlesPC
{
    VkDeviceAddress particles_in;
    VkDeviceAddress particles_out;
};