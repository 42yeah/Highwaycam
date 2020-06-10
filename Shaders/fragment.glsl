#version 330 core

// name: Default camera
// enabled: true
// weight: 5

in vec2 uv;

uniform float time;
uniform float aspect;

out vec4 color;

void main() {
    vec2 xy = uv;
    xy.x *= aspect;
    color = vec4(xy, 0.5 + sin(time), 1.0);
}
