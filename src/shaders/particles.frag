#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/scene.inl"

layout(location = 0) in f32vec2 in_uv;

layout(push_constant, scalar) uniform pc {ParticlesPC data;};

layout(location = 1) out f32vec4 FragColor;

void main()
{
    // TODO: Understand how to put the correct texture in 
    
    // f32vec2 uv = in_uv.xy
    // f32vec3 tex = texture(sampler2D(texture2DTable[albedo_index], samplerTable[data.sampler_id]), uv);
    // FragColor = f32vec4(tex, 1.0);
    FragColor = f32vec3(1.f, 0.f, 0.f);
}
