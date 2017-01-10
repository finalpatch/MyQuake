#version 330 core

layout(std140) uniform UniformBlock
{
    mat4 model;
    mat4 view;
    mat4 projection;

    vec4 lightmapMask;
    vec4 ambientLight;
} uniforms;

uniform sampler2D lightmap0;

out vec4 color;

in VS_OUT
{
    vec4 color;
    vec2 texcoord;
} fs_in;

void main(void)
{
    vec4 lightValues = texture(lightmap0, fs_in.texcoord);
    float l = dot(lightValues, uniforms.lightmapMask);
    color = vec4(l, l, l, 1.0);
}
