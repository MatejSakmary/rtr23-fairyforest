#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/shared.inl"
#include "src/shaders/util/normals_compress.glsl"


layout(location = 0) in f32vec2 in_uv;
layout(location = 1) in flat u32 albedo_index;
layout(location = 2) in f32vec3 world_position;
layout(location = 3) in f32vec2 in_motion_vector;
layout(location = 4) in f32 view_space_depth;

layout(location = 0) out f32vec4 out_color;
layout(location = 1) out f32vec4 out_motion_vector;

layout(push_constant, scalar) uniform push { DrawPc pc; };

f32vec3 point_lights_contribution(f32vec3 normal, f32vec3 world_position, f32vec3 view_direction)
{
    f32vec3 total_contribution = f32vec3(0.0);
    for(i32 light_index = 0; light_index < pc.curr_num_lights; light_index++)
    {
        LightInfo light = LightInfo(pc.lights_info)[light_index];
        const f32vec3 position_to_light = normalize(light.position - world_position);
        const f32 diffuse = max(dot(normal, position_to_light), 0.0);

        const f32 to_light_dist = length(light.position - world_position);
        const f32 falloff_factor = 
            light.constant_falloff + 
            light.linear_falloff * to_light_dist + 
            light.quadratic_falloff + (to_light_dist * to_light_dist);

        const f32 attenuation = 1.0 / falloff_factor;
        total_contribution += light.color * diffuse * attenuation * light.intensity;
    }
    return total_contribution;
}

void main()
{
    f32vec4 albedo = f32vec4(1.0);
    if (albedo_index != -1)
    {
        albedo = texture(sampler2D(texture2DTable[albedo_index], samplerTable[pc.sampler_id]), in_uv);
    }

    out_motion_vector = f32vec4(in_motion_vector, 0.0, 0.0);
    const f32 depth = gl_FragCoord.z;

    // Get the cascade index of the current fragment
    u32 cascade_idx = 0;
    for(cascade_idx; cascade_idx < NUM_CASCADES; cascade_idx++)
    {
        if(view_space_depth < (ShadowmapCascadeData(pc.cascade_data)[cascade_idx]).cascade_far_depth)
        {
            break;
        }
    }
    // Due to accuracy issues when reprojecting some samples may think
    // they are behind the last far depth - clip these to still be in the last cascade
    // to avoid out of bounds indexing
    cascade_idx = min(cascade_idx, NUM_CASCADES - 1);

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

    const f32 threshold = 0.05;

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
    }

    const u32 world_normal_compressed = texelFetch(utexture2DTable[pc.ss_normals_index], i32vec2(gl_FragCoord.xy), 0).r;
    const f32vec3 world_normal = u16_to_nrm(world_normal_compressed);
    const f32 sun_norm_dot = clamp(dot(world_normal, pc.sun_direction), 0.0, 1.0);
    const f32 ambient_occlusion = (pc.enable_ao == 1) ?  texelFetch(texture2DTable[pc.ssao_index], i32vec2(gl_FragCoord.xy), 0).r : 1.0;
    const f32 weighed_ambient_occlusion = pow(ambient_occlusion, 2.0);
    const f32 sun_intensity = 0.5;

    const f32 ambient_factor = (dot(f32vec3(0.0, 0.0, 1.0), world_normal) * 0.4 + 0.6) * 0.5;
    const f32 occlusion_ambient_factor = weighed_ambient_occlusion * ambient_factor;
    const f32 indirect = clamp(dot(world_normal, normalize(pc.sun_direction * f32vec3(-1.0, -1.0, 0.0))), 0.0, 1.0);

    f32vec3 diffuse = sun_norm_dot * SUN_COLOR * sun_intensity * clamp(pow(shadow, 4), 0.0, 1.0);

    const f32vec3 camera_position = (CameraInfoBuf(pc.camera_info)[pc.fif_index]).position;
    const f32vec3 camera_to_point = normalize(world_position - camera_position);
    diffuse += point_lights_contribution(world_normal, world_position, camera_to_point) * weighed_ambient_occlusion;
    diffuse += occlusion_ambient_factor * (SKY_COLOR * 100);
    diffuse += weighed_ambient_occlusion * indirect * f32vec3(0.82, 0.910, 0.976) * 0.02;

    out_color = f32vec4(diffuse * (albedo.rgb), 1.0);
    // out_color.xyz = f32vec3(shadow) * sun_norm_dot;
    // out_color.xyz = diffuse;
}