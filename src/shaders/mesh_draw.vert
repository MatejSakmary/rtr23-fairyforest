#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/scene.inl"

layout(push_constant, scalar) uniform pc { DrawPc data; };

layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Transform { f32mat4x3 trans;  };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Index     { u32 idx;          };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Position  { f32vec3 position; };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer UV        { f32vec2 uv;       };

layout(location = 0) out vec2 out_uv;

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
    const u32 index = gl_VertexIndex;
    const u32 instance = gl_InstanceIndex;

    SceneDescriptor scene_descriptor = SceneDescriptor(data.scene_descriptor);
    MeshDescriptor mesh_descriptor = MeshDescriptor(scene_descriptor.mesh_descriptors_start)[data.mesh_index];

    f32mat4x3 transform = (Transform(scene_descriptor.transforms_start)[mesh_descriptor.transforms_offset + instance]).trans;
    u32 vert_index = (Index(scene_descriptor.indices_start)[mesh_descriptor.indices_offset + index]).idx;

    f32vec3 position = (Position(scene_descriptor.positions_start)[mesh_descriptor.positions_offset + vert_index]).position;
    f32vec2 uv = (UV(scene_descriptor.uvs_start)[mesh_descriptor.uvs_offset + vert_index]).uv;

    out_uv = uv;
    gl_Position = data.view_proj * mat_4x3_to_4x4(transform) * f32vec4(position, 1.0);
}
