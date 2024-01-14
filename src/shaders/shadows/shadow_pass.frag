#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/shared.inl"
layout(location = 0) in f32vec2 in_uv;
layout(location = 1) in f32 viewspace_depth;
layout(location = 2) in flat u32 albedo_index;

layout(push_constant, scalar) uniform push { ShadowPC pc; };

void main()
{
    gl_FragDepth = viewspace_depth / (ShadowmapCascadeData(pc.cascade_data)[pc.cascade_index]).far_plane;
}