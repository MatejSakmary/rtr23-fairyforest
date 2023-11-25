#version 450

layout(location = 0) out vec4 out_color;

vec2 const[] positions = vec2[3](
    vec2(0.5, 0.5),
    vec2(0.0, -0.5),
    vec2(-0.5, 0.5));

void main()
{
    out_color = vec4(colors[gl_VertexIndex], 1.0);
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}