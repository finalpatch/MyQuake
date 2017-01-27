#version 330 core

#define MAX_LIGHTSTYLES 64
#define MAX_DLIGHTS 32
#define FLAG_BACKSIDE 1u
#define VFLAG_ONSEAM 1u

layout(std140) uniform UniformBlock
{
    mat4 model;
    mat4 view;
    mat4 projection;

    vec4 lightStyles[MAX_LIGHTSTYLES / 4];
    vec4 ambientLight;

    vec4 dlights[MAX_DLIGHTS];
    uint ndlights;

    uint flags;
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
    vec4 color;
    vec2 lightTexCoord;
    vec2 diffuseTexCoord;
    vec4 lightScales;
    vec3 worldPos;
    vec3 normal;
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
    vs_out.lightTexCoord = lightTexCoord;
    if (((uniforms.flags & FLAG_BACKSIDE) != 0u) && ((vtxflags & VFLAG_ONSEAM) != 0u))
        vs_out.diffuseTexCoord = diffuseTexCoord + vec2(0.5, 0.0);
    else
        vs_out.diffuseTexCoord = diffuseTexCoord;

    uvec4 ustyles = uvec4(styles * 255);
    uvec4 style_h  = (ustyles / 4u) % 16u;
    uvec4 style_l  = ustyles % 4u;
    vec4 mask = sign(1.0 - styles);
    vs_out.lightScales = mask * vec4(
        uniforms.lightStyles[style_h.r][style_l.r],
        uniforms.lightStyles[style_h.g][style_l.g],
        uniforms.lightStyles[style_h.b][style_l.b],
        uniforms.lightStyles[style_h.a][style_l.a]);
    vs_out.worldPos = (uniforms.model * position).xyz;
    vs_out.normal = normal;
}
