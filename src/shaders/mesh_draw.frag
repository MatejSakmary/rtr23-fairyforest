#version 450
// Make sure to remove this if I ever include an .inl file
#include "src/backend/backend.inl"
layout(location = 0) in f32vec2 in_uv;
layout(location = 0) out f32vec4 out_color;

void main()
{
    out_color = f32vec4(in_uv, 0.0, 1.0);
}