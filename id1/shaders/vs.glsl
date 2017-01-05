#version 330 core

layout(std140) uniform TransformBlock
{
    mat4 model;
    mat4 view;
    mat4 projection;
} transform;

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;

out VS_OUT
{
    vec4 color;
} vs_out;

void main(void)
{
    vec3 l1 = normalize(vec3(-0.2, 0.5, 1));
    vec3 l2 = normalize(vec3(0.3, 0.7, -0.5));
    mat3 r = mat3(transform.model);
    vec3 n = normalize(r * normal);
    float l = max(0, dot(n, l1))*0.7 + max(0, dot(n, l2))*0.5;

    mat4 t = transform.projection * transform.view * transform.model;
    gl_Position = t * position;
    vs_out.color = vec4(l, l, l, 1.0);
}