#ifndef CAMERA_GLSL
#define CAMERA_GLSL

#include "../common/random.glsl"

struct ViewRay {
    vec3 origin;
    vec3 direction;
};

ViewRay generateViewRay(vec2 launch_id, vec2 launch_size, mat4 inv_view, mat4 inv_proj, inout uvec4 rng_state) {
    const vec2 pixelCenter = launch_id + vec2(stepAndOutputRNGFloat(rng_state), stepAndOutputRNGFloat(rng_state));
    const vec2 inUV = pixelCenter / launch_size;
    vec2 d = inUV * 2.0 - 1.0;

    vec3 origin = (inv_view * vec4(0,0,0,1)).xyz;
    vec4 target = inv_proj * vec4(d.x, d.y, 1, 1);
    vec3 direction = (inv_view * vec4(normalize(target.xyz), 0)).xyz;

    float focus_dist = 10;
    float lens_radius = 0;

    if (lens_radius != 0) {
        vec3 convergence_point = origin + focus_dist * direction;

        vec2 rd = lens_radius * sampleUniformDiskPolar(rng_state);
        vec3 right = normalize((inv_view * vec4(1,0,0,0)).xyz);
        vec3 up    = normalize((inv_view * vec4(0,1,0,0)).xyz);
        vec3 offset = rd.x * right + rd.y * up;

        origin += offset;
        direction = normalize(convergence_point - origin);
    }

    ViewRay ray = {origin, direction};
    return ray;
}

#endif