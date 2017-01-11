#version 330 core

#define MAX_LIGHTSTYLES 64

layout(std140) uniform UniformBlock
{
    mat4 model;
    mat4 view;
    mat4 projection;

    vec4 lightStyles[MAX_LIGHTSTYLES / 4];
    vec4 ambientLight;
} uniforms;

uniform sampler2D lightmap0;
uniform sampler2D diffusemap;

out vec4 color;

in VS_OUT
{
    vec4 color;
    vec2 texcoord;
    vec2 texcoord2;
    vec4 lightScales;
} fs_in;

void main(void)
{
    vec4 lightValues = texture(lightmap0, fs_in.texcoord);
    vec4 l = vec4(vec3(dot(lightValues, fs_in.lightScales)) + uniforms.ambientLight.rgb, 1.0);
    color = texture(diffusemap, fs_in.texcoord2) * l * 2.0;
    //color = l;
}
