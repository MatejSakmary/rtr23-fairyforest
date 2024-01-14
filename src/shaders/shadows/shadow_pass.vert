#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/shared.inl"

layout(push_constant, scalar) uniform push { ShadowPC pc; };

layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Transform { f32mat4x3 trans;  };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Index     { u32 idx;          };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Position  { f32vec3 position; };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer UV        { f32vec2 uv;       };

layout(location = 0) out f32vec2 out_uv;
layout(location = 1) out f32 viewspace_depth;
layout(location = 2) out flat u32 albedo_index;

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

    SceneDescriptor scene_descriptor = SceneDescriptor(pc.scene_descriptor);
    MeshDescriptor mesh_descriptor = MeshDescriptor(scene_descriptor.mesh_descriptors_start)[pc.mesh_index];
    MaterialDescriptor material_descriptor = MaterialDescriptor(scene_descriptor.material_descriptors_start)[mesh_descriptor.material_index];

    const f32mat4x3 transform = (Transform(scene_descriptor.transforms_start)[mesh_descriptor.transforms_offset + instance]).trans;

    const f32vec3 position = (Position(scene_descriptor.positions_start)[mesh_descriptor.positions_offset + vert_index]).position;
    const f32vec2 uv = (UV(scene_descriptor.uvs_start)[mesh_descriptor.uvs_offset + vert_index]).uv;

    albedo_index = material_descriptor.albedo_index;
    out_uv = uv;

    const f32mat4x4 view = (ShadowmapCascadeData(pc.cascade_data)[pc.cascade_index]).cascade_view_matrix;
    const f32mat4x4 projection = (ShadowmapCascadeData(pc.cascade_data)[pc.cascade_index]).cascade_proj_matrix;
    const f32mat4x4 model = mat_4x3_to_4x4(transform);
    viewspace_depth = (view * model * f32vec4(position, 1.0)).z;
    gl_Position = projection * view * model * f32vec4(position, 1.0);
}