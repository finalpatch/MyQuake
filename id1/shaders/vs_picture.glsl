#version 330 core

uniform vec4 transform;
uniform vec4 srcrect;

out VS_OUT
{
    vec2 uv;
} vs_out;

const vec2 rectangleUV[4] = vec2[4](
    vec2(0, 0),
    vec2(1, 0),
    vec2(1, 1),
    vec2(0, 1)
);

void main(void)
{
    vec2 scale = transform.zw;
    vec2 offset = transform.xy;
    vec2 uv = rectangleUV[gl_VertexID];
    gl_Position = vec4((uv * scale + offset) * vec2(2.0, -2.0) + vec2(-1.0, 1.0), 0.5, 1);
    vs_out.uv = uv * srcrect.zw + srcrect.xy;
}
