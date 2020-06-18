#version 330 core

// name: Spiral pass
// weight: -1
// enabled: true
// slider: spiralness

in vec2 uv;

uniform float time;
uniform sampler2D prevPass;
uniform float spiralness;

out vec4 color;

vec2 rot2d(vec2 uv, float deg) {
    return vec2(cos(deg) * uv.x - sin(deg) * uv.y,
                sin(deg) * uv.x + cos(deg) * uv.y);
}

void main() {
    vec2 xy = uv * 2.0 - 1.0;
    xy = rot2d(xy, time * spiralness);
    xy = rot2d(xy, time * length(xy) * spiralness);
    color = texture(prevPass, xy);
}
