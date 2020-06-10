#version 330 core

// name: Test Raymarching scene
// weight: 4
// enabled: true

in vec2 uv;

uniform float time;
uniform float aspect;

uniform sampler2D prevPass;

out vec4 color;

vec3 sunDir = normalize(vec3(0.0, 1.0, 1.0));

// https://www.iquilezles.org/www/articles/smin/smin.htm
float smin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

float ball(vec3 p, vec3 off) {
    p -= off;
    return length(p) - 0.5;
}

float cube(vec3 p, vec3 off) {
    p -= off;
    vec3 c = abs(p) - vec3(0.5);
    return length(max(c, 0.0))
        + min(max(c.x, max(c.y, c.z)), 0.0);
}

float scene(vec3 p) {
    float dist = ball(p, vec3(sin(time * 0.5), 0.5, cos(time * 1.5) * 2.0));
    dist = smin(ball(p, vec3(cos(time * 2.0), sin(time) * 0.5 + 1.0, 1.3)), dist, 0.6);
    dist = smin(ball(p, vec3(0.0, 0.6, 0.0)), dist, 1.0);
    dist = smin(cube(p, vec3(cos(time * 0.1) * 3.0, cos(time * 0.2) * 0.5 + 1.0, -0.5)), dist, 0.6);
    return dist;
}

float sol(vec3 p) {
    return p.y;
}

vec2 map(vec3 p) {
    float closest = 1000.0;
    float id = -1.0;
    
    float dist = sol(p);
    if (dist < closest) { closest = dist; id = 0.5; }

    dist = scene(p);
    if (dist < closest) { closest = dist; id = 1.5; }
    
    return vec2(closest, id);
}

vec2 intersect(vec3 ro, vec3 rd) {
    float depth = 0.0;
    float id = -1.0;
    for (int i = 0; i < 200; i++) {
        vec2 info = map(ro + rd * depth);
        if (info.x <= 0.001) {
            id = info.y;
            break;
        }
        depth += info.x;
    }
    return vec2(depth, id);
}

vec3 getColor(float id, vec3 p, vec3 rd) {
    if (id < -0.5) {
        vec3 sky = vec3(0.6, 0.8, 0.9) * 0.9;
        sky += rd.y * vec3(0.3, 0.3, 1.1) * 2.5;
        return vec3(sky);
    }
    if (id < 1.0) {
        vec3 floorColor = vec3(1.5, 1.5, 1.5);
        p = p * 2.0;
        vec3 f = fract(p);
        vec3 u = mod(floor(p), 2.0);
        float d = 0.6 + clamp(abs(u.x - u.z), 0.0, 1.0);
        return vec3(d * floorColor);
    }
    if (id < 2.0) { return vec3(1.0, 0.6, 0.0); }
    return vec3(1.0, 0.0, 0.0);
}

vec3 getNormal(vec3 p) {
    const float epsilon = 0.001;
    return normalize(vec3(
        map(p).x - map(vec3(p.x - epsilon, p.yz)).x,
        map(p).x - map(vec3(p.x, p.y - epsilon, p.z)).x,
        map(p).x - map(vec3(p.xy, p.z - epsilon)).x
    ));
}

float getShadowIntensity(vec3 p, float k) {
    float depth = 0.001;
    float intensity = 1.0;
    for (int i = 0; i < 25; i++) {
        // If you wonder why we are using a vec2 here,
        // the x component is shortest distance, and the y component is id.
        // The y component was used for texturing, and is useless here. So we simply ignore it.
        vec2 info = map(p + depth * sunDir);
        intensity = min(intensity, k * (info.x / depth));
        if (intensity < 0.001) {
            break;
        }
        depth += info.x;
    }
    return intensity;
}

void main() {
    vec2 xy = uv * 2.0 - 1.0;
    xy.x *= aspect;
    vec3 ro = vec3(3.0 * sin(time * 0.5), 4.0, 3.0 * cos(time * 0.5));
    vec3 center = vec3(0.0, 1.0, 0.0);
    
    vec3 front = normalize(center - ro);
    vec3 right = normalize(cross(front, vec3(0.0, 1.0, 0.0)));
    vec3 up = normalize(cross(right, front));
    
    mat4 lookAt = mat4(
        vec4(right, 0.0),
        vec4(up, 0.0),
        vec4(front, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );
    vec3 rd = normalize(vec3(lookAt * vec4(xy, 2.0, 1.0)));
    vec2 info = intersect(ro, rd);
    vec3 pos = ro + rd * info.x;
    vec3 n = getNormal(pos);
    
    vec3 light = vec3(0.0);
    float ambient = 1.0;
    float diffuse = max(dot(n, sunDir), 0.0);
    float specular = clamp(pow(max(dot(-normalize(reflect(-sunDir, n)), rd), 0.0), 32.0), 0.0, 1.0);
    float dome = 0.2 + 0.8 * clamp(rd.y, 0.0, 1.0);
    float back = max(dot(n, vec3(sunDir.x, 0.0, sunDir.z)), 0.0);
    float sol = 0.2 + 0.8 * clamp(-rd.y, 0.0, 1.0);
    float shadow = getShadowIntensity(pos + n * 0.001, 5.0);
    
    light += ambient * vec3(0.21, 0.2, 0.2);
    light += diffuse * vec3(0.5, 0.55, 0.5) * shadow;
    light += specular * vec3(0.8, 0.8, 0.8) * shadow;
    light += dome * vec3(1.2, 1.11, 1.3) * shadow;
    light += back * vec3(0.3, 0.3, 0.34);
    light += sol * vec3(0.1, 0.1, 0.2);

    if (info.y < -0.5) { light = vec3(1.0); }
    
    vec3 objColor = getColor(info.y, pos, rd) * light;
    objColor = pow(objColor, vec3(0.4545));
    
    color = vec4(mix(objColor, texture(prevPass, uv).rgb, 0.5), 1.0);
}

