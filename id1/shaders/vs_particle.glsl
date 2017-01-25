#version 330 core

layout(std140) uniform UniformBlock
{
    mat4 view;
    mat4 projection;
    vec4 origin;
} uniforms;

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;

#define MIN_PARTICLE_SIZE 0.0625
#define MAX_PARTICLE_SIZE 1.0
#define MAX_DISTANCE 1000

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
    float scale = 1.0 - dist / MAX_DISTANCE;
    scale = clamp(scale, MIN_PARTICLE_SIZE, MAX_PARTICLE_SIZE);
    gl_PointSize = 16 * scale;
}
