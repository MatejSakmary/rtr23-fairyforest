#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/shared.inl"
#include "src/shaders/util/normals_compress.glsl"

layout(location = 0) in f32vec2 in_uv;
layout(location = 1) in f32vec4 in_tangent;
layout(location = 2) in f32vec3 in_normal;
layout(location = 3) in flat u32 albedo_index;
layout(location = 4) in flat u32 normals_index;

layout(location = 0) out u32vec4 compressed_normal;

layout(push_constant, scalar) uniform push { DrawPc pc; };

void main()
{
    const f32vec3 normal = texture(sampler2D(texture2DTable[normals_index], samplerTable[pc.sampler_id]), in_uv).rgb;
    const f32vec3 rescaled_normal = normal * 2.0 - 1.0;
                
    const f32vec3 norm_in_tangent = normalize(in_tangent.xyz);
    const f32vec3 norm_in_normal = normalize(in_normal.xyz);
    // re-orthogonalize tangent with respect to normal
    const f32vec3 ort_tangent = normalize(norm_in_tangent - dot(norm_in_tangent, norm_in_normal) * norm_in_normal);
    const f32vec3 bitangent = normalize(cross(norm_in_normal, ort_tangent) * -in_tangent.w);
    const f32mat3x3 TBN = f32mat3x3(ort_tangent, bitangent, norm_in_normal);
    const f32vec3 world_normal = (pc.no_normal_maps == 1) ? in_normal : normalize(TBN * rescaled_normal);
    compressed_normal = u32vec4(nrm_to_u16(world_normal));
    // compressed_normal = u32vec4(nrm_to_u16(in_normal));
}