#version 330 core

uniform sampler2D lightmap0;

out vec4 color;

in VS_OUT
{
    vec4 color;
    vec2 texcoord;
} fs_in;

void main(void)
{
    //color = vec4(0.0, 0.8, 1.0, 1.0);
    //color = fs_in.color;
    color = vec4(texture(lightmap0, fs_in.texcoord).rrr, 1.0);
}
