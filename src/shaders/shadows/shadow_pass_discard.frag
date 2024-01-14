#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/shared.inl"
layout(location = 0) in f32vec2 in_uv;
layout(location = 1) in f32 viewspace_depth;
layout(location = 2) in flat u32 albedo_index;

layout(push_constant, scalar) uniform push { ShadowPC pc; };

void main()
{
    f32vec4 albedo = f32vec4(1.0);
    if (albedo_index != -1)
    {
        albedo = texture(sampler2D(texture2DTable[albedo_index], samplerTable[pc.sampler_id]), in_uv);
    }
    if (albedo.a <= 0.3)
    {
        discard;
    }
}