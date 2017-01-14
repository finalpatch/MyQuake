#version 330 core

#define MAX_LIGHTSTYLES 64

layout(std140) uniform UniformBlock
{
    mat4 model;
    mat4 view;
    mat4 projection;

    vec4 lightStyles[MAX_LIGHTSTYLES / 4];
    vec4 ambientLight;

    uint flags;
} uniforms;

uniform sampler2D lightmap;
uniform sampler2D diffusemap;

out vec4 color;

in VS_OUT
{
    vec4 color;
    vec2 lightTexCoord;
    vec2 diffuseTexCoord;
    vec4 lightScales;
} fs_in;

void main(void)
{
    vec4 lightValues = texture(lightmap, fs_in.lightTexCoord);
    vec4 l = vec4(vec3(dot(lightValues, fs_in.lightScales)) + uniforms.ambientLight.rgb, 1.0);
    color = texture(diffusemap, fs_in.diffuseTexCoord) * l * 2.0;
}
