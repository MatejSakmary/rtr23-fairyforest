#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/shared.inl"

layout(push_constant, scalar) uniform pc { DrawPc data; };

layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Transform { f32mat4x3 trans;  };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Index     { u32 idx;          };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Position  { f32vec3 position; };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer UV        { f32vec2 uv;       };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Tangent   { f32vec4 tangent;  };
layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Normal    { f32vec3 normal;  };

layout(location = 0) out f32vec2 out_uv;
layout(location = 1) out f32vec4 out_tangent;
layout(location = 2) out f32vec3 out_normal;
layout(location = 3) out flat u32 albedo_index;
layout(location = 4) out flat u32 normals_index;

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
    const f32vec4 tangent = (Tangent(scene_descriptor.tangents_start)[mesh_descriptor.tangents_offset + vert_index]).tangent;
    const f32vec3 normal = (Normal(scene_descriptor.normals_start)[mesh_descriptor.normals_offset + vert_index]).normal;

    normals_index = material_descriptor.normal_index;
    albedo_index = material_descriptor.albedo_index;
    out_uv = uv;
    out_tangent = f32vec4(normalize((mat_4x3_to_4x4(transform) * f32vec4(tangent.xyz, 0.0)).xyz), tangent.w);
    out_normal = normalize((mat_4x3_to_4x4(transform) * f32vec4(normal.xyz, 0.0)).xyz);

    const f32mat4x4 jittered_view_proj = (CameraInfoBuf(data.camera_info)[data.fif_index]).jittered_view_projection;
    gl_Position = jittered_view_proj * mat_4x3_to_4x4(transform) * f32vec4(position, 1.0);
}