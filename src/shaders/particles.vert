#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/scene.inl"

layout(push_constant, scalar) uniform pc {ParticlesPC data;};

layout(buffer_reference, scalar, buffer_reference_align = 4) buffer Part {f32vec3 pos; f32vec3 vel;};


void main()
{
    // Do I have to use DrawPC and setup all the scene/mesh_descriptor etc?
    //
    // const u32 index = gl_VertexIndex;
    // const u32 instance = gl_InstanceIndex;

    // const u32 vert_index = (Index(scene_descriptor.indices_start)[mesh_descriptor.indices_offset + index]).idx;
    // const f32vec3 position = (Part(scene_descriptor.positions_start)[mesh_descriptor.positions_offset + vert_index]).position;
    
    gl_PointSize = 10.f;
    gl_Position = data.view  * f32vec4(position, 1.0);
}
