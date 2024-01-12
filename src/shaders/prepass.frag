#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/shared.inl"
layout(location = 0) in f32vec2 in_uv;
layout(location = 1) in f32vec4 in_tangent;
layout(location = 2) in f32vec3 in_normal;
layout(location = 3) in flat u32 albedo_index;
layout(location = 4) in flat u32 normals_index;

layout(location = 0) out f32vec4 world_normal;

layout(push_constant, scalar) uniform pc { DrawPc data; };

void main()
{
    f32vec4 albedo = f32vec4(1.0);
    if (albedo_index != -1)
    {
        albedo = texture(sampler2D(texture2DTable[albedo_index], samplerTable[data.sampler_id]), in_uv);
    }
    if (albedo.a <= 0.25)
    {
        discard;
    }
    const f32vec3 normal = texture(sampler2D(texture2DTable[normals_index], samplerTable[data.sampler_id]), in_uv).rgb;
    const f32vec3 rescaled_normal = normal * 2.0 - 1.0;
                
    const f32vec3 norm_in_tangent = normalize(in_tangent.xyz);
    const f32vec3 norm_in_normal = normalize(in_normal.xyz);
    // re-orthogonalize tangent with respect to normal
    const f32vec3 ort_tangent = normalize(norm_in_tangent - dot(norm_in_tangent, norm_in_normal) * norm_in_normal);
    const f32vec3 bitangent = normalize(cross(norm_in_normal, ort_tangent) * -in_tangent.w);
    const f32mat3x3 TBN = f32mat3x3(ort_tangent, bitangent, norm_in_normal);
    world_normal = f32vec4(normalize(TBN * rescaled_normal), 1.0);
    // world_normal = f32vec4(in_normal, 1.0);
}