#version 330 core

in vec2 uv;

uniform float time;
uniform sampler2D prevPass;

out vec4 color;

void main() {
    color = texture(prevPass, uv);
}

