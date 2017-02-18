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
    vec2 lightTexCoord;
    vec2 diffuseTexCoord;
    vec4 lightScales;
    vec3 worldPos;
    vec3 normal;
} fs_in;

void main(void)
{
    vec4 lightValues = texture(lightmap, fs_in.lightTexCoord);
    vec3 l = vec3(dot(lightValues, fs_in.lightScales)) + uniforms.ambientLight.rgb;

    for (uint i = 0u; i < uniforms.ndlights; ++i)
    {
        vec3 lightPos = uniforms.dlights[i].xyz;
        float radius = uniforms.dlights[i].w;
        vec3 lightRay = lightPos - fs_in.worldPos;
        float dist = length(lightRay);
        float intensity = dot(normalize(lightRay), fs_in.normal);
        float distScale = (radius / 255.0) * 20000 / (dist * dist);
        intensity *= distScale;
        l += clamp(intensity, 0.0, 1.0); // dynamic light can go up to 100% above base
    }

    vec2 uv = fs_in.diffuseTexCoord;
    if ((uniforms.flags & FLAG_TURBULENCE) != 0u)
    {
        uv += vec2(sin(uniforms.globalTime + uv.y*4.0), cos(uniforms.globalTime + uv.x*4.0)) / 8.0;
        l *= 0.5; // offset the * 2.0 later, because turbulence faces don't use lightmaps
    }
    vec4 texColor = texture(diffusemap, uv);
    if (texColor.a == 0.0) // full bright
        l = vec3(1.0);
    else
        l *= 2.0; // lightmaps are 0-200%
    color = vec4(texColor.rgb * l, 1.0);
}
