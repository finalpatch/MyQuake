#version 330 core

#define MAX_LIGHTSTYLES 64
#define MAX_DLIGHTS 32
#define FLAG_TURBULENCE 2u

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

uniform sampler2D lightmap;
uniform sampler2D diffusemap;

out vec4 color;

in VS_OUT
{
    vec4 color;
    vec2 lightTexCoord;
    vec2 diffuseTexCoord;
    vec4 lightScales;
    vec3 worldPos;
    vec3 normal;
} fs_in;

void main(void)
{
    vec4 lightValues = texture(lightmap, fs_in.lightTexCoord);
    vec4 l = vec4(vec3(dot(lightValues, fs_in.lightScales)) + uniforms.ambientLight.rgb, 1.0);

    for (uint i = 0u; i < uniforms.ndlights; ++i)
    {
        vec3 lightPos = uniforms.dlights[i].xyz;
        float radius = uniforms.dlights[i].w;
        vec3 lightRay = lightPos - fs_in.worldPos;
        float dist = length(lightRay);
        {
            float intensity = dot(normalize(lightRay), fs_in.normal);
            intensity = intensity * (radius / 432.0) * 10000 / (dist * dist);
            l += clamp(intensity, 0.0, 1.0);
        }
    }

    vec2 uv = fs_in.diffuseTexCoord;
    if ((uniforms.flags & FLAG_TURBULENCE) != 0u)
        uv += vec2(sin(uniforms.globalTime + uv.y*2.0), cos(uniforms.globalTime + uv.x*2.0)) / 8.0;
    color = texture(diffusemap, uv) * l * 2.0;
}
