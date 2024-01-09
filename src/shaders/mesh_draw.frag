#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/scene.inl"
// #extension GL_EXT_debug_printf : enable
layout(location = 0) in f32vec2 in_uv;
layout(location = 1) in flat u32 albedo_index;

layout(location = 0) out f32vec4 out_color;

layout(push_constant, scalar) uniform pc { DrawPc data; };

void main()
{
    f32vec4 albedo = f32vec4(1.0);
    if (albedo_index != -1)
    {
        albedo = texture(sampler2D(texture2DTable[albedo_index], samplerTable[data.sampler_id]), in_uv);
    }
    const f32vec3 world_normal = texelFetch(texture2DTable[data.ss_normals_index], i32vec2(gl_FragCoord.xy), 0).rgb;
    const f32 tex_sun_norm_dot = clamp(dot(world_normal, data.sun_direction), 0.0, 1.0);

    const f32vec3 sun_color = vec3(1.0, 0.95, 0.6);
    const f32vec3 sky_color = vec3(0.5, 0.7, 1.0);
    const f32 ambient_factor = (dot(f32vec3(0.0, 0.0, 1.0), world_normal) * 0.4 + 0.6) * 0.2;
    f32vec3 diffuse = tex_sun_norm_dot * sun_color;
    diffuse += ambient_factor * sky_color;
    out_color = f32vec4(diffuse * albedo.rgb, 1.0);
}