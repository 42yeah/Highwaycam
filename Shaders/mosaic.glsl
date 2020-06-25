#version 330 core

// name: Mosaic
// weight: 1
// enabled: true
// slider: mosaic

in vec2 uv;

uniform float time;
uniform sampler2D prevPass;
uniform float aspect;
uniform float mosaic;

out vec4 color;

void main() {
    vec2 xy = uv;
    float m = mix(2.0, 100.0, mosaic);
    xy.x = floor(xy.x * m * aspect) / (m * aspect);
    xy.y = floor(xy.y * m) / m;
    color = texture(prevPass, xy);
}
