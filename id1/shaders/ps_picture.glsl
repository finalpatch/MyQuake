#version 330 core

uniform sampler2D picture;

out vec4 color;

in VS_OUT
{
    vec2 uv;
} fs_in;

void main()
{
    color = texture(picture, fs_in.uv);
}
