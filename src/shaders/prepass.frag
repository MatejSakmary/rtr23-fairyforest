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

// TODO(msakmary) Should load normal maps as UNORM format so I don't have to do this
f32vec4 srgb_to_linear(f32vec4 srgb)
{
    f32vec3 color_srgb = srgb.rgb;
    f32vec3 selector = clamp(ceil(color_srgb - 0.04045), 0.0, 1.0); // 0 if under value, 1 if over
    f32vec3 under = color_srgb / 12.92;
    f32vec3 over = pow((color_srgb + 0.055) / 1.055, f32vec3(2.4, 2.4, 2.4));
    f32vec3 result = mix(under, over, selector);
    return f32vec4(result, srgb.a);
}
f32vec4 linear_to_srgb(f32vec4 linear)
{
    f32vec3 color_linear = linear.rgb;
    f32vec3 selector = clamp(ceil(color_linear - 0.0031308), 0.0, 1.0); // 0 if under value, 1 if over
    f32vec3 under = 12.92 * color_linear;
    f32vec3 over = (1.055) * pow(color_linear, f32vec3(1.0 / 2.4)) - 0.055;
    f32vec3 result = mix(under, over, selector);
    return f32vec4(result, linear.a);
}
void main()
{
    f32vec4 albedo = f32vec4(1.0);
    if (albedo_index != -1)
    {
        albedo = texture(sampler2D(texture2DTable[albedo_index], samplerTable[data.sampler_id]), in_uv);
    }
    if (albedo.a == 0.0)
    {
        discard;
    }
    const f32vec4 srgb_normal = texture(sampler2D(texture2DTable[normals_index], samplerTable[data.sampler_id]), in_uv);
    const f32vec3 linear_normal = linear_to_srgb(srgb_normal).rgb;
    const f32vec3 rescaled_normal = linear_normal * 2.0 - 1.0;
                
    const f32vec3 norm_in_tangent = normalize(in_tangent.xyz);
    const f32vec3 norm_in_normal = normalize(in_normal.xyz);
    // re-orthogonalize tangent with respect to normal
    const f32vec3 ort_tangent = normalize(norm_in_tangent - dot(norm_in_tangent, norm_in_normal) * norm_in_normal);
    const f32vec3 bitangent = normalize(cross(norm_in_normal, ort_tangent) * -in_tangent.w);
    const f32mat3x3 TBN = f32mat3x3(ort_tangent, bitangent, norm_in_normal);
    world_normal = f32vec4(normalize(TBN * rescaled_normal), 1.0);
    // world_normal = f32vec4(in_normal, 1.0);
}