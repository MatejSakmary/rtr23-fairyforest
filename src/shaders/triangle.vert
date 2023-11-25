#version 450

layout (location = 0) out vec4 out_color;

const vec2[] positions = vec2[3] (
    vec2( 0.5,  0.5),
    vec2( 0.0, -0.5),
    vec2(-0.5,  0.5)
);

const vec3[] colors = vec3[3] (
    vec3( 1.0, 0.0, 0.0),
    vec3( 0.0, 1.0, 0.0),
    vec3( 0.0, 0.0, 1.0)
);

void main() 
{
    out_color = vec4(colors[gl_VertexIndex], 1.0);
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
