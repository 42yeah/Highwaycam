#version 330 core

// name: Noise pass
// weight: 2
// slider: noisiness

in vec2 uv;

uniform float time;
uniform sampler2D prevPass;
uniform float noisiness;

out vec4 color;

vec3 rand3(vec2 i) {
    return fract(sin(vec3(dot(i, vec2(41.515, 67.89)),
                          dot(i, vec2(61.213, 21.314)),
                          dot(i, vec2(90.125, 31.5))
                          )) * (12345.67 + time));
}

void main() {
    vec3 tex = texture(prevPass, uv).rgb;
    vec3 noise = rand3(uv) * 2.0 - 1.0;
    color = vec4(mix(tex, noise, noisiness), 1.0);
}
