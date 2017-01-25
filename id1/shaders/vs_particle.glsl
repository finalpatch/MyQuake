#version 330 core

layout(std140) uniform UniformBlock
{
    mat4 view;
    mat4 projection;
    vec4 origin;
} uniforms;

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;

out VS_OUT
{
    vec4 color;
} vs_out;

void main(void)
{
    mat4 t = uniforms.projection * uniforms.view;
    gl_Position = t * position;
    vs_out.color = color;

    float dist = distance(uniforms.origin.xyz, position.xyz);
    gl_PointSize = 1024.0 / dist;
}
