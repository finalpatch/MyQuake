#version 330 core

layout(std140) uniform UniformBlock
{
    mat4 view;
    mat4 projection;
    float globalTime;
} uniforms;

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 lightTexCoord;
layout (location = 3) in vec2 diffuseTexCoord;
layout (location = 4) in vec4 styles;
layout (location = 5) in uint vtxflags;

out VS_OUT
{
    vec3 position;
} vs_out;

void main(void)
{
    mat4 t = uniforms.projection * uniforms.view;
    gl_Position = t * position;
    vs_out.position = position.xyz;
}
