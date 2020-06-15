#version 330 core

// name: Default camera
// enabled: true
// weight: 10

in vec2 uv;

out vec4 color;

uniform sampler2D rawCamera;
uniform vec2 resolution;


void main() {
    vec2 xy = vec2(uv.x, 1.0 - uv.y);
    color = texture(rawCamera, xy);
}
