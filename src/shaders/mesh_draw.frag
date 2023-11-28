#version 450
#include "src/shared/scene.inl"
// #extension GL_EXT_debug_printf : enable
layout(location = 0) in f32vec2 in_uv;
layout(location = 1) in flat u32 albedo_index;
layout(location = 2) in flat u32 normals_index;
layout(location = 0) out f32vec4 out_color;

layout(push_constant, scalar) uniform pc { DrawPc data; };

void main()
{
    f32vec4 albedo = texture(sampler2D(texture2DTable[albedo_index], samplerTable[data.sampler_id]), in_uv);
    f32vec4 normal = texture(sampler2D(texture2DTable[normals_index], samplerTable[data.sampler_id]), in_uv);
    if(albedo.a == 0.0) 
    {
        discard;
    }
    out_color = f32vec4(normal.rgb, 1.0);
}