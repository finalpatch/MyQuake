#version 330 core

layout(std140) uniform UniformBlock
{
    mat4 view;
    mat4 projection;
    vec4 origin;
    float globalTime;
} uniforms;

uniform sampler2D skytexture0;
uniform sampler2D skytexture1;

out vec4 color;

in VS_OUT
{
    vec3 position;
} fs_in;

#define SKY_HEIGHT 5000.0
#define SCROLL_FACTOR (1.0 / 4)

void main(void)
{
    vec3 v = (fs_in.position - uniforms.origin.xyz);
    float scale = SKY_HEIGHT / v.y;
    vec2 uv = (v * scale + uniforms.origin.xyz).xz / SKY_HEIGHT;
    vec2 offset = vec2(uniforms.globalTime, -uniforms.globalTime);
    vec4 fg = texture(skytexture0, uv + offset* SCROLL_FACTOR);
    vec4 bg = texture(skytexture1, uv + offset * SCROLL_FACTOR * 0.5);
    color = vec4(mix(bg.rgb, fg.rgb, fg.a), 1.0) * 2.0;
}
