#version 450
#extension GL_GOOGLE_include_directive : require
#include "src/shared/shared.inl"

layout(location = 0) out vec4 out_color;

layout(push_constant, scalar) uniform pc { TrianglePC data; };

void main()
{
    out_color = TestBuffer(data.test_buffer_address).color;
}