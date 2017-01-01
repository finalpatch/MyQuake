#version 330 core

out vec4 color;

in VS_OUT
{
    vec4 color;
} fs_in;

void main(void)
{
    //color = vec4(0.0, 0.8, 1.0, 1.0);
    color = fs_in.color;
}
