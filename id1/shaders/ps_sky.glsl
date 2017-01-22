#version 330 core

layout(std140) uniform UniformBlock
{
    mat4 view;
    mat4 projection;
    float globalTime;
} uniforms;

uniform sampler2D skytexture0;
uniform sampler2D skytexture1;

out vec4 color;

in VS_OUT
{
    vec3 position;
} fs_in;

void main(void)
{
    vec4 fg = texture(skytexture0, fs_in.position.xz / 128.0 + uniforms.globalTime);
    vec4 bg = texture(skytexture1, fs_in.position.xz / 128.0 + uniforms.globalTime * 0.5);
    color = vec4(mix(bg.rgb, fg.rgb, fg.a), 1.0);
}
