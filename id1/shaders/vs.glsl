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

layout (location = 0) in vec4 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoord;
layout (location = 3) in vec4 styles;

out VS_OUT
{
    vec4 color;
    vec2 texcoord;
    vec4 lightScales;
} vs_out;

void main(void)
{
    vec3 l1 = normalize(vec3(-0.2, 0.5, 1));
    vec3 l2 = normalize(vec3(0.3, 0.7, -0.5));
    mat3 r = mat3(uniforms.model);
    vec3 n = normalize(r * normal);
    float l = max(0, dot(n, l1))*0.7 + max(0, dot(n, l2))*0.5;

    mat4 t = uniforms.projection * uniforms.view * uniforms.model;
    gl_Position = t * position;
    vs_out.color = vec4(l, l, l, 1.0);
    vs_out.texcoord = texCoord;

    uvec4 ustyles = uvec4(styles * 255);
    uvec4 style_h  = (ustyles / 4u) % 16u;
    uvec4 style_l  = ustyles % 4u;
    vec4 mask = vec4(sign(ustyles - 255u));
    vs_out.lightScales = mask * vec4(
        uniforms.lightStyles[style_h.r][style_l.r],
        uniforms.lightStyles[style_h.g][style_l.g],
        uniforms.lightStyles[style_h.b][style_l.b],
        uniforms.lightStyles[style_h.a][style_l.a]);
}
