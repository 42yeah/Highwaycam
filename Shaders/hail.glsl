#version 330 core

// name: Useless pass
// weight: -1
// warning: Hello Zambon21

in vec2 uv;

uniform float time;
uniform sampler2D prevPass;

out vec4 color;

void main() {
    color = texture(prevPass, uv);
}
