#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/shared.inl"

layout(push_constant, scalar) uniform pc { DrawPc data; };

layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Transform { f32mat4x3 trans;  };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Index     { u32 idx;          };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Position  { f32vec3 position; };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer UV        { f32vec2 uv;       };

layout(location = 0) out f32vec2 out_uv;
layout(location = 1) out flat u32 albedo_index;
layout(location = 2) out f32vec3 world_position;
layout(location = 3) out f32vec2 out_motion_vector;
layout(location = 4) out f32 view_space_depth;

mat4 mat_4x3_to_4x4(mat4x3 in_mat)
{
    return mat4(
        vec4(in_mat[0], 0.0),
        vec4(in_mat[1], 0.0),
        vec4(in_mat[2], 0.0),
        vec4(in_mat[3], 1.0)
    );
}

void main()
{
    const u32 vert_index = gl_VertexIndex;
    const u32 instance = gl_InstanceIndex;

    SceneDescriptor scene_descriptor = SceneDescriptor(data.scene_descriptor);
    MeshDescriptor mesh_descriptor = MeshDescriptor(scene_descriptor.mesh_descriptors_start)[data.mesh_index];
    MaterialDescriptor material_descriptor = MaterialDescriptor(scene_descriptor.material_descriptors_start)[mesh_descriptor.material_index];

    const f32mat4x3 transform = (Transform(scene_descriptor.transforms_start)[mesh_descriptor.transforms_offset + instance]).trans;

    const f32vec3 position = (Position(scene_descriptor.positions_start)[mesh_descriptor.positions_offset + vert_index]).position;
    const f32vec2 uv = (UV(scene_descriptor.uvs_start)[mesh_descriptor.uvs_offset + vert_index]).uv;

    albedo_index = material_descriptor.albedo_index;
    out_uv = uv;
    const f32mat4x4 jittered_view_projection = (CameraInfoBuf(data.camera_info)[data.fif_index]).jittered_view_projection;
    const f32mat4x4 view = (CameraInfoBuf(data.camera_info)[data.fif_index]).view;
    const f32mat4x4 model = mat_4x3_to_4x4(transform);

    world_position = (model * f32vec4(position, 1.0)).xyz;

    view_space_depth = -(view * model * f32vec4(position, 1.0)).z;
    gl_Position = jittered_view_projection * model * f32vec4(position, 1.0);

    const f32mat4x4 view_projection = (CameraInfoBuf(data.camera_info)[data.fif_index]).view_projection;
    const f32mat4x4 prev_view_projection = (CameraInfoBuf(data.camera_info)[data.fif_index]).prev_view_projection;
    const f32vec4 unjittered_pos = view_projection * model * f32vec4(position, 1.0);
    const f32vec4 unjittered_prev_pos = prev_view_projection * model * f32vec4(position, 1.0);
    out_motion_vector = f32vec2(( unjittered_pos.xy / unjittered_pos.w - unjittered_prev_pos.xy / unjittered_prev_pos.w)) * -0.5;
}
