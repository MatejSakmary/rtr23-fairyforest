#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/shared.inl"
// #extension GL_EXT_debug_printf : enable
layout(location = 0) in f32vec2 in_uv;
layout(location = 1) in flat u32 albedo_index;
layout(location = 2) in f32vec3 world_position;
layout(location = 3) in f32 viewspace_depth;

layout(location = 0) out f32vec4 out_color;

layout(push_constant, scalar) uniform push { DrawPc pc; };

void main()
{
    f32vec4 albedo = f32vec4(1.0);
    if (albedo_index != -1)
    {
        albedo = texture(sampler2D(texture2DTable[albedo_index], samplerTable[pc.sampler_id]), in_uv);
    }
    const f32 depth = gl_FragDepth;

    // Get the cascade index of the current fragment
    u32 cascade_idx = 0;
    for(cascade_idx; cascade_idx < NUM_CASCADES; cascade_idx++)
    {
        if(viewspace_depth < (ShadowmapCascadeData(pc.cascade_data)[cascade_idx]).cascade_far_depth)
        {
            break;
        }
    }
    // Due to accuracy issues when reprojecting some samples may think
    // they are behind the last far depth - clip these to still be in the last cascade
    // to avoid out of bounds indexing
    cascade_idx = min(cascade_idx, 3);

    const f32mat4x4 shadow_view = (ShadowmapCascadeData(pc.cascade_data)[cascade_idx]).cascade_view_matrix;
    const f32mat4x4 shadow_proj_view = (ShadowmapCascadeData(pc.cascade_data)[cascade_idx]).cascade_proj_matrix * shadow_view;

    // Project the world position by the shadowmap camera
    const f32vec4 shadow_projected_world = shadow_proj_view * f32vec4(world_position, 1.0);
    const f32vec3 shadow_ndc_pos = shadow_projected_world.xyz / shadow_projected_world.w;
    const f32vec3 shadow_map_uv = f32vec3((shadow_ndc_pos.xy + f32vec2(1.0)) / f32vec2(2.0), f32(cascade_idx));
    const f32 distance_in_shadowmap = texture( sampler2DArray( texture2DArrayTable[pc.esm_shadowmap_index], samplerTable[pc.shadow_sampler_id]), shadow_map_uv).r;

    const f32vec4 shadow_view_world_pos = shadow_view * f32vec4(world_position, 1.0);
    const f32 shadow_reprojected_distance = shadow_view_world_pos.z / (ShadowmapCascadeData(pc.cascade_data)[cascade_idx]).far_plane;

    // Equation 3 in ESM paper
    f32 shadow = exp(-ESM_FACTOR * (shadow_reprojected_distance - distance_in_shadowmap));

    const f32 threshold = 0.2;

    // For the cases where we break the shadowmap assumption (see figure 3 in ESM paper)
    // we do manual filtering where we clamp the individual samples before blending them
    if(shadow > 1.0 + threshold)
    {
        const f32vec4 gather = textureGather(sampler2DArray(texture2DArrayTable[pc.esm_shadowmap_index], samplerTable[pc.shadow_sampler_id]), shadow_map_uv, 0);
        // clamp each sample we take individually before blending them together
        const f32vec4 shadow_gathered = clamp(
            exp(-ESM_FACTOR * (shadow_reprojected_distance - gather)),
            f32vec4(0.0, 0.0, 0.0, 0.0),
            f32vec4(1.0, 1.0, 1.0, 1.0)
        );

        // This is needed because textureGather uses a sampler which only has 8bits of precision in
        // the fractional part - this causes a slight imprecision where texture gather already 
        // collects next texel while the fract part is still on the inital texel
        //    - fract will be 0.998 while texture gather will already return the texel coresponding to 1.0
        //      (see https://www.reedbeta.com/blog/texture-gathers-and-coordinate-precision/)
        const f32 offset = 1.0/512.0;
        const f32vec2 shadow_pix_coord = shadow_map_uv.xy * SHADOWMAP_RESOLUTION + (-0.5 + offset);
        const f32vec2 blend_factor = fract(shadow_pix_coord);

        // texel gather component mapping - (00,w);(01,x);(11,y);(10,z) const daxa_f32 tmp0 = mix(shadow_gathered.w, shadow_gathered.z, blend_factor.x);
        const f32 tmp0 = mix(shadow_gathered.w, shadow_gathered.z, blend_factor.x);
        const f32 tmp1 = mix(shadow_gathered.x, shadow_gathered.y, blend_factor.x);
        shadow = mix(tmp0, tmp1, blend_factor.y);
        // out_color = f32vec4(1.0, 0.0, 0.0, 1.0);
        // return;
    }

    const f32vec3 world_normal = texelFetch(texture2DTable[pc.ss_normals_index], i32vec2(gl_FragCoord.xy), 0).rgb;
    const f32 sun_norm_dot = clamp(dot(world_normal, pc.sun_direction), 0.0, 1.0);
    const f32 ambient_occlusion = (pc.enable_ao == 1) ?  texelFetch(texture2DTable[pc.ssao_index], i32vec2(gl_FragCoord.xy), 0).r : 1.0;
    const f32 weighed_ambient_occlusion = pow(ambient_occlusion, 2.0);
    const f32 sun_intensity = 0.2;

    const f32vec3 sun_color = f32vec3(0.82, 0.910, 0.976);

    const f32 ambient_factor = (dot(f32vec3(0.0, 0.0, 1.0), world_normal) * 0.4 + 0.6) * 0.2;
    const f32 occlusion_ambient_factor = weighed_ambient_occlusion * ambient_factor;
    const f32 indirect = clamp(dot(world_normal, normalize(pc.sun_direction * f32vec3(-1.0, -1.0, 0.0))), 0.0, 1.0);

    f32vec3 diffuse = sun_norm_dot * sun_color * sun_intensity * clamp(pow(shadow, 4), 0.0, 1.0);
    diffuse += occlusion_ambient_factor * (SKY_COLOR * 100);
    diffuse += weighed_ambient_occlusion * indirect * f32vec3(0.82, 0.910, 0.976) * 0.02;

    out_color = f32vec4(diffuse * albedo.rgb, 1.0);
    // out_color.xyz = f32vec3(shadow) * sun_norm_dot;
}