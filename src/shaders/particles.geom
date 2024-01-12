#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/scene.inl"

layout(points) in;
layout(triangle_strip) out;
layout(max_vertices = 4) out;

layout(push_constant, scalar) uniform pc {ParticlesPC data;};

layout(location = 0) out f32vec2 out_uv;

void main()
{
    f32vec4 = gl_in[0].gl_Position;

    // a: Left bottom
    f32vec2 va : P.xy + f32vec2(-0.5, -0.5) * gl_in[0].gl_PointSize;
    gl_Position = data.view * f32vec4(va, P.zw);
    out_uv = vec2(0.0, 0.0);
    EmitVertex();

    // b: Left op
    f32vec2 vb : P.xy + f32vec2(-0.5, 0.5) * gl_in[0].gl_PointSize;
    gl_Position = data.view * f32vec4(vb, P.zw);
    out_uv = vec2(0.0, 1.0);
    EmitVertex();

    // c: Right bottom
    f32vec2 vc : P.xy + f32vec2(0.5, -0.5) * gl_in[0].gl_PointSize;
    gl_Position = data.view * f32vec4(vc, P.zw);
    out_uv = vec2(1.0, 0.0);
    EmitVertex();

    // d: Right top
    f32vec2 vd : P.xy + f32vec2(0.5, 0.5) * gl_in[0].gl_PointSize;
    gl_Position = data.view * f32vec4(vd, P.zw);
    out_uv = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
