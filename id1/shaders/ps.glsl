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

out vec4 color;

in VS_OUT
{
    vec4 color;
    vec2 texcoord;
    flat vec4 styles;
} fs_in;

void main(void)
{
    uvec4 styles = uvec4(fs_in.styles * 255);
    uvec4 style_h  = (styles / 4u) % 16u;
    uvec4 style_l  = styles % 4u;

    vec4 mask = vec4(sign(styles - 255u));

    vec4 lightScales = vec4(
        uniforms.lightStyles[style_h.r][style_l.r],
        uniforms.lightStyles[style_h.g][style_l.g],
        uniforms.lightStyles[style_h.b][style_l.b],
        uniforms.lightStyles[style_h.a][style_l.a]) * mask;
    vec4 lightValues = texture(lightmap0, fs_in.texcoord);
    float l = dot(lightValues, lightScales);
    color = vec4(l, l, l, 1.0);
}
